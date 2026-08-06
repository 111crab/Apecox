# 火球术 Montage 事件等待容错修复报告

*创建日期：2026-07-07*
*修复者：Claude*
*任务：修复 `UAuraConfiguredActiveAbility` 的 Montage 事件等待容错问题*

## 修复概要

修复了配置化技能在 `CastMontage` + `MontageSpawnEventTag` 路径下的 5 类卡死/崩溃风险：

| 场景 | 修复前 | 修复后 |
|------|--------|--------|
| Montage 加载失败 | 可能永久等待 AnimNotify 事件 | 直接 fallback 生成投射物 |
| AnimNotify 被删除 | Ability 卡死，永不生成投射物 | Montage 完成时 fallback 补救 |
| MontageSpawnEventTag 配错 | 永远等不到事件 | Montage 完成时 Warning + fallback 补救 |
| Montage 被打断（眩晕等） | 继续等待已中断的 Montage | 取消 Ability，正确清理 |
| AnimNotify 重复触发 | 可能生成多个投射物 | `bProjectileSpawned` 防护 |

## 修改文件

| 文件 | 修改性质 |
| --- | --- |
| `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h` | 签名修改 + 新增 2 个状态变量 |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp` | 完整重写 |

## 需求逐项对照

### 1. `PlayCastMontageIfConfigured()` 改为返回 `bool`

- **签名**：`void → bool`
- **`true`**：Montage 资产加载成功 (`LoadSynchronous()`) 且 `PlayMontageAndWait` 任务创建/启动成功
- **`false`**：未配置 (`IsNull()`) / 加载失败 / 任务创建失败
- 调用方基于返回值决定是否创建 `WaitGameplayEvent`

### 2. 新增状态变量

- `bool bProjectileSpawned = false;` — 投射物是否已成功生成，防止重复生成
- `bool bWaitingForMontageSpawnEvent = false;` — 服务端是否正在等待 AnimNotify，供 `OnMontageCompleted` 判断是否需 fallback

### 3. `ExecuteProjectileSpawn()` 防重复

- 函数开头检查 `if (bProjectileSpawned) return;`
- 成功生成投射物后设置 `bProjectileSpawned = true;`
- 所有失败路径（CombatInterface / SourceASC / Projectile / SpecHandle）不设置标记

### 4. 服务端等待事件逻辑条件化

`OnTargetDataReceived` 服务端分支，3 条路径：

```
PlayCastMontageIfConfigured() 返回 bMontagePlayedSuccessfully
                                    │
                    ┌───────────────┼───────────────┐
                    │ true                          │ false
                    ▼                               ▼
          HasSpawnEventTag?                  直接 fallback 生成
          │            │
          │ true       │ false
          ▼            ▼
   创建 WaitGameplayEvent   直接 fallback 生成
   设置 bWaiting = true
```

**关键变化**：旧代码用 `!IsNull() && IsValid()` 判断是否创建 `WaitGameplayEvent`；新代码用 `bMontagePlayedSuccessfully && bHasSpawnEventTag`，确保 Montage 真正播放成功才等待事件。

### 5. `OnMontageSpawnEvent()` 防重复

- 检查 `if (bProjectileSpawned) return;` — 防止 AnimNotify 重复触发导致多生成
- 清除 `bWaitingForMontageSpawnEvent = false;`

### 6. `OnMontageCompleted()` fallback 补救

```cpp
if (bWaitingForMontageSpawnEvent && !bProjectileSpawned)
{
    // 服务端在等事件但 Montage 已结束仍未生成
    // 可能原因：AnimNotify 被删、Tag 配错、Event 发送失败
    // V1-A fallback：在此补救生成，不卡死
    UE_LOG(Warning, ...);
    ExecuteProjectileSpawn();
    return;
}
```

**为什么 completed + 没投射物 → fallback，但 interrupted + 没投射物 → 取消？**
- Completed：Montage 正常播完但 Notify 没触发 → 配置问题 → 补救
- Interrupted：Montage 被打断 → 技能失败 → 取消

### 7. `OnMontageInterrupted()` 正确取消

- 使用 `CachedSkillDef` 前判空
- 如果投射物尚未生成：`EndAbility(..., bWasCancelled=true)`
- 如果投射物已生成：不影响，投射物继续飞行

### 8. `EndAbility()` 清理

新增清理两行：
```cpp
bProjectileSpawned = false;
bWaitingForMontageSpawnEvent = false;
```

## 编译结果

```
Result: Succeeded
Total execution time: 45.71 seconds
Exit code: 0
```

- `AuraConfiguredActiveAbility.cpp` — 编译通过，**0 警告 0 错误**
- `AuraAbilitySystemComponent.cpp` — 有 4 个 C4996 警告（`DynamicAbilityTags` deprecated，是旧代码，不在本次修改范围）
- `AuraPlayerState.cpp` — 1 个 C4996 警告（`NetUpdateFrequency` deprecated，旧代码）
- 链接成功，产物：`UnrealEditor-Aura.dll`

## 未修改内容

- 未新增自定义 AnimNotify
- 未改旧 `GA_FireBolt` / `UAuraProjectileSpell` / `StartupAbilities` 路径
- 未修改 `AuraSkillDefinition.h`
- 保留了所有中文注释，并为所有容错逻辑补充了"为什么这样做"的解释
