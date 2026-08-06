# 火球术 Montage 启动顺序与同步失败生命周期修复报告

*创建日期：2026-07-07*
*修复者：Claude*
*参考：`Agent/02_SkillSystem/Skill_Runtime_Risk_Checklist.md`*

## 解决的问题

### 问题 1：`PlayCastMontageIfConfigured()` 返回值语义不完整

**旧代码**：`CreatePlayMontageAndWaitProxy` + `ReadyForActivation` 在同一个函数内完成，返回 `true` 只代表任务已创建并提交，不代表 Montage 真正播放成功。播放失败通过 `OnCancelled` 同步回调暴露，但调用方不知道如何区分"启动失败"和"战斗中断"。

### 问题 2：服务端先播放 Montage 再注册 `WaitGameplayEvent`

如果 AnimNotify 在 Montage 前几帧就触发，`WaitGameplayEvent` 还没注册 → 事件被漏掉 → 火球只能等到 Montage 完成 fallback 才生成。

## 修复方案

### 1. 拆分 Montage 逻辑：Prepare → Start

| 旧 API | 新 API |
|--------|--------|
| `bool PlayCastMontageIfConfigured()` | `UAbilityTask_PlayMontageAndWait* PrepareCastMontageTaskIfConfigured()` |
| （同上，一体完成） | `void StartPreparedCastMontageTask()` |

- **`PrepareCastMontageTaskIfConfigured()`**：加载 Montage、创建 `PlayMontageAndWait` 任务、绑定回调。**不调用 `ReadyForActivation()`**。返回任务指针或 nullptr。
- **`StartPreparedCastMontageTask()`**：仅调用 `ActiveMontageTask->ReadyForActivation()`。调用方必须在调用前后管理 `bStartingMontagePlayback` 标志。

### 2. 新增状态

| 字段 | 类型 | 用途 |
|------|------|------|
| `ActiveWaitEventTask` | `TObjectPtr<UAbilityTask_WaitGameplayEvent>` | 服务端创建的事件等待任务引用，用于在启动失败时 `EndTask()` |
| `bStartingMontagePlayback` | `bool` | 是否在 `StartPreparedCastMontageTask` 调用栈内；`OnMontageInterrupted` 用来区分同步/异步中断 |
| `bMontageStartupFailed` | `bool` | 启动阶段是否发生了同步失败；调用方在 `StartPreparedCastMontageTask` 返回后检查 |

移除：`bMontagePlaybackFailedSynchronously`（被 `bStartingMontagePlayback` + `bMontageStartupFailed` 替代）。

### 3. 服务端新顺序

```
OnTargetDataReceived（服务端）：

1. PrepareCastMontageTaskIfConfigured()  -- 准备 MontageTask，不启动

2. 如果 MontageTask 有效 && HasSpawnEventTag：
   a. 创建 ActiveWaitEventTask           -- 先注册监听
   b. WaitEventTask->ReadyForActivation()
   c. bWaitingForMontageSpawnEvent = true
   d. bStartingMontagePlayback = true    -- 设防
   e. StartPreparedCastMontageTask()     -- 再启动 Montage
   f. bStartingMontagePlayback = false   -- 解防
   g. if bMontageStartupFailed:          -- 检查同步失败
        → EndTask(WaitEventTask)
        → ExecuteProjectileSpawn()       -- fallback

3. 如果 MontageTask 有效 && !HasSpawnEventTag：
   → StartPreparedCastMontageTask()     -- 纯表现
   → ExecuteProjectileSpawn()

4. 如果 MontageTask 无效：
   → ExecuteProjectileSpawn()           -- 无条件 fallback
```

### 4. `OnMontageInterrupted()` 双模式

```
OnMontageInterrupted:
  if bStartingMontagePlayback:
    → bMontageStartupFailed = true
    → 清理 WaitEventTask
    → return（不 EndAbility！调用方接管）
  else:
    → 真正中断 → 如果没生成投射物 → EndAbility(cancelled)
```

### 5. `OnMontageSpawnEvent` / `OnMontageCompleted` 清理 `ActiveWaitEventTask`

- `OnMontageSpawnEvent` 成功后：`ActiveWaitEventTask->EndTask()` + 置空
- `OnMontageCompleted` fallback 前：同样清理，避免迟到事件重复触发
- `OnMontageInterrupted` 所有路径：清理
- `EndAbility()`：`ActiveWaitEventTask = nullptr`

## 修改文件

| 文件 | 修改 |
| --- | --- |
| `AuraConfiguredActiveAbility.h` | 移除 `PlayCastMontageIfConfigured()`、`bMontagePlaybackFailedSynchronously`；新增 `PrepareCastMontageTaskIfConfigured()`、`StartPreparedCastMontageTask()`、`ActiveWaitEventTask`、`bStartingMontagePlayback`、`bMontageStartupFailed` |
| `AuraConfiguredActiveAbility.cpp` | 完整重写（~620行）；重构 `OnTargetDataReceived`、`OnMontageInterrupted`、`OnMontageCompleted`、`OnMontageSpawnEvent`、`EndAbility`；新增 Prepare/Start 方法 |

## 编译结果

```
Result: Succeeded
Total execution time: 24.96 seconds
Exit code: 0
```

- `AuraConfiguredActiveAbility.cpp` — **0 错误 0 警告**
- 仅有旧代码 C4996（`DynamicAbilityTags`、`NetUpdateFrequency`），不在本次修改范围

## 剩余风险

1. **`ActiveWaitEventTask->EndTask()` 的线程安全性**：当前所有对 `ActiveWaitEventTask` 的操作都在游戏线程执行（回调、清理），无风险。
2. **专用服务器未测试**：在专用服务器上 `Montage` 可能不会在 `Avatar` 上播放（无渲染组件），但 `ReadyForActivation` 的同步失败行为与 Listen Server 相同，由 `bMontageStartupFailed` 处理。
3. **`PrepareCastMontageTaskIfConfigured` 返回非空但 `StartPreparedCastMontageTask` 时 `ActiveMontageTask` 被意外清空**：`StartPreparedCastMontageTask` 已做非空检查并打 Warning 日志。

## 未修改

- 旧 `GA_FireBolt` / `UAuraProjectileSpell` / `StartupAbilities`
- `AuraSkillDefinition.h`
- 资产配置
- 所有中文注释保留并补充
