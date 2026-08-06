# 火球术 Montage 同步播放失败 + 事件漏检修复报告

*创建日期：2026-07-07*
*修复者：Claude*
*审查来源：Codex 复审*

## 问题描述

**问题 1**：`PlayCastMontageIfConfigured()` 返回 `true` 只代表 `PlayMontageAndWait` 任务已创建并 `ReadyForActivation`，不代表 Montage 真的播放成功。UE 内部的实际播放失败通过 `OnCancelled` 同步回调暴露。如果 `OnCancelled` → `OnMontageInterrupted()` → `EndAbility()` 清空了 `CachedSkillDef`，调用方后续访问 `CachedSkillDef->MontageSpawnEventTag` → 崩溃。

**问题 2**：服务端先播放 Montage 再创建 `WaitGameplayEvent`。如果 AnimNotify 非常靠前（Montage 前几帧触发），可能漏掉事件 → 火球延迟到 Montage 完成 fallback 才生成。

## 修复概要

3 处修改，集中在 1 个文件 + 1 个头文件：

| 文件 | 修改 |
| --- | --- |
| `AuraConfiguredActiveAbility.h` | +1 状态变量 `bMontagePlaybackFailedSynchronously` |
| `AuraConfiguredActiveAbility.cpp` | 重写 `OnTargetDataReceived` 服务端路径、重写 `OnMontageInterrupted`、更新 `EndAbility` 和 `PlayCastMontageIfConfigured` |

## 修复逐项对照

### 1. 调用 `PlayCastMontageIfConfigured()` 后检查 `CachedSkillDef`

所有调用位置（服务端 2 条路径）都在调用后立即检查：

```cpp
if (!CachedSkillDef)
{
    UE_LOG(LogTemp, Warning, TEXT("...Montage 播放启动时发生同步失败，Ability 已结束。"));
    return;
}
```

如果 `PlayCastMontageIfConfigured()` 内部 `ReadyForActivation()` 同步失败 → `OnCancelled` → `OnMontageInterrupted()` → `EndAbility()`，则 `CachedSkillDef` 已被清空。此时直接 return，不再访问。

### 2. 重构服务端顺序：先创建 WaitGameplayEvent，再播放 Montage

`OnTargetDataReceived` 服务端路径 A（配置了 `SpawnEventTag`）：

```
旧顺序：PlayCastMontageIfConfigured() → Create WaitGameplayEvent
新顺序：Create WaitGameplayEvent → PlayCastMontageIfConfigured()
```

确保 AnimNotify 无论多靠前都不会被漏掉。

### 3. Montage 播放失败 → 取消等待 + fallback，而不是 EndAbility

新增 `bMontagePlaybackFailedSynchronously` 标志（见下），配合 `OnTargetDataReceived` 中的 fallback 逻辑：

- Montage 同步失败 → `OnCancelled` → `OnMontageInterrupted()` → 检查标志 → **不** `EndAbility` → 仅 return
- 调用方检测到 `!bMontagePlayed` → 取消 `bWaitingForMontageSpawnEvent` → 调用 `ExecuteProjectileSpawn()` fallback

### 4. 新增 `bMontagePlaybackFailedSynchronously` 状态

在 `AuraConfiguredActiveAbility.h`：

```cpp
bool bMontagePlaybackFailedSynchronously = false;
```

**机制**：

```
OnTargetDataReceived 调用方:
  bMontagePlaybackFailedSynchronously = true;   // 设防
  PlayCastMontageIfConfigured();                // 可能内部触发 OnCancelled
  bMontagePlaybackFailedSynchronously = false;  // 解防

OnMontageInterrupted:
  if (bMontagePlaybackFailedSynchronously)
      return;  // 同步失败，不结束 Ability，让调用方接管
  // 否则是真正的异步中断（眩晕等）→ EndAbility
```

**三种场景的完整行为**：

| 场景 | 触发路径 | `bMontagePlaybackFailedSynchronously` | 行为 |
|------|----------|--------------------------------------|------|
| Montage 同步启动失败 | `ReadyForActivation()` → `OnCancelled` 同步 | `true` | `OnMontageInterrupted` 直接 return；调用方 fallback 生成 |
| Montage 异步被打断（眩晕） | 眩晕 → `OnInterrupted` 异步 | `false` | `OnMontageInterrupted` → `EndAbility(bWasCancelled)` |
| Montage 正常播放 | 正常执行 | `false`（调用后已恢复） | `OnMontageSpawnEvent` 或 `OnMontageCompleted` fallback |

## 修改后的 OnTargetDataReceived 服务端流程

```
HasSpawnEventTag?
├── Yes:
│   1. bWaitingForMontageSpawnEvent = true
│   2. Create WaitGameplayEvent   ← 先创建，不错过靠前的 Notify
│   3. bMontagePlaybackFailedSynchronously = true
│      PlayCastMontageIfConfigured()
│      bMontagePlaybackFailedSynchronously = false
│   4. Check CachedSkillDef (null → return)
│   5. if (!bMontagePlayed) → 取消等待 + ExecuteProjectileSpawn()
│      else → 等待事件或完成/中断
│
└── No:
    1. bMontagePlaybackFailedSynchronously = true
       PlayCastMontageIfConfigured()  ← 纯表现
       bMontagePlaybackFailedSynchronously = false
    2. Check CachedSkillDef (null → return)
    3. ExecuteProjectileSpawn()
```

客户端路径简化为：`PlayCastMontageIfConfigured()` + return（表现层失败不影响玩法）。

## 编译结果

```
Result: Succeeded
Total execution time: 26.57 seconds
Exit code: 0
```

- `AuraConfiguredActiveAbility.cpp` — **0 错误 0 警告**
- 仅有旧代码的 C4996 警告（`DynamicAbilityTags`、`NetUpdateFrequency`），不在本次修改范围

## 未修改

- 未新增自定义 AnimNotify
- 未改旧 `GA_FireBolt` / `UAuraProjectileSpell` / `StartupAbilities`
- 未改 `AuraSkillDefinition.h`
- 所有中文注释已更新，解释同步失败机制和顺序设计原因
