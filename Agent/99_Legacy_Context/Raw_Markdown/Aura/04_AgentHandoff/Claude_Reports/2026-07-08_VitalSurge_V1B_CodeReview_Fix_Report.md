# 生命涌动 V1-B 代码审查修复报告

*创建日期：2026-07-08*
*修复者：Claude*
*审查来源：Codex 对 `UAuraConfiguredActiveAbility` HoldRelease 路径的审查*

## 修复概要

修复了 6 个问题，涉及 3 个文件。

| 文件 | 修改范围 |
|------|----------|
| `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h` | 签名修改 + 新增 1 个状态变量 |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp` | `CanActivateAbility`、`OnHoldInputReleased`、`ApplyEffectEntriesToSelf`、`AddChannelingState`/`RemoveChannelingState`、`EndAbility` |
| `Source/Aura/Private/AuraGameplayTags.cpp` | 缩进修正 |

## 逐项修复

### 1. `bAddedChannelingState` 防护

**问题**：`RemoveChannelingState()` 在 `EndAbility()` 中被无条件调用。Press 模式的技能（火球术）结束时也会执行 `RemoveChannelingState()` → 误扣 `State.Skill.Channeling`（该 Tag 从未被添加）。

**修复**：
- 新增 `bool bAddedChannelingState = false;`
- `AddChannelingState()`：ASC 有效且 `AddLooseGameplayTag` 成功后 → `bAddedChannelingState = true`
- `RemoveChannelingState()`：仅在 `bAddedChannelingState` 为 true 时才调用 `RemoveLooseGameplayTag`，并重置为 false
- `EndAbility()`：同样重置 `bAddedChannelingState = false`

### 2. `ApplyEffectEntriesToSelf` 返回 `bool`，移除内部 `EndAbility`

**问题**：`ApplyEffectEntriesToSelf` 内部有 3 处 `EndAbility` 调用（ASC 缺失时、Spec 创建失败时）。而 `OnHoldInputReleased` 在调用后又额外调了一次 `EndAbility`。双重 `EndAbility` 可能导致状态清理异常。

**修复**：
- 签名：`void` → `bool`
- `true`：成功应用 EffectEntries（或防重复已跳过）
- `false`：不可恢复错误（ASC 缺失、非 Self Target、Spec 创建失败）
- 移除内部所有 `EndAbility` 调用
- `OnHoldInputReleased`：检查返回值，`false` 时 `EndAbility(bWasCancelled=true)`，`true` 时 `EndAbility(bWasCancelled=false)`
- 确保每条代码路径只调用一次 `EndAbility`

### 3. `PartialChargeScale` 真正生效

**问题**：`UAuraSkillDefinition::PartialChargeScale` 字段存在但在运行时从未使用。PartialCharge Entry 的 SetByCaller 数值直接应用，未缩放。

**修复**：
- `ApplyEffectEntriesToSelf` 中注入 SetByCaller 时：
  ```cpp
  float ScaledValue = Pair.Value.GetValueAtLevel(AbilityLevel);
  if (ChargeResult == PartialCharge && Entry.Condition == PartialCharge)
      ScaledValue *= CachedSkillDef->PartialChargeScale;
  ```
- 注释说明：PartialCharge Entry 中配置的是未缩放基准值，运行时乘系数。这样 DA 中只需配一份基准值，不必手动重复计算半量。

### 4. 拒绝非 Self Target

**问题**：V1-B 只支持 `EffectEntry.Target == Self`。如果配置了 `Target == HitActor` 或 `Target == Source`，会静默调用 `ApplyGameplayEffectSpecToSelf`，行为错误。

**修复**：
- `ApplyEffectEntriesToSelf`：遇到 `Target != Self` → Error 日志 + 返回 `false`
- `CanActivateAbility`：Direct 分支增加 `bHasNonSelfEntry` 检测 → Warning 日志 + 拒绝激活

### 5. HoldRelease 配置校验

**问题**：配置错误（`FullChargeDuration <= 0`、缺少 FullCharge/PartialCharge EffectEntry）在 `CanActivateAbility` 阶段无校验，运行时可能静默失败。

**修复**（`CanActivateAbility` 末尾新增 HoldRelease 校验块）：
- `FullChargeDuration <= 0` → 拒绝激活
- Direct 分支校验：
  - `bHasFullChargeEntry`：至少一个 `Condition == FullCharge || Always` 且 `EffectClass` 有效
  - `bHasPartialChargeEntry`：至少一个 `Condition == PartialCharge || Always` 且 `EffectClass` 有效
  - 两者缺一不可

### 6. `AuraGameplayTags.cpp` 缩进整理

**问题**：新增 tag 区域（`SetByCaller_Healing`、`SetByCaller_HealingPerTick`、`State_Skill_Channeling`）使用 3-tab 缩进，与文件其余部分的 2-tab 缩进不一致。

**修复**：`sed` 替换，将相关区域（~15 行）的缩进从 3-tab 减为 2-tab。

## 编译结果

```
Result: Succeeded
Total execution time: 12.14 seconds
Exit code: 0
```

- `AuraConfiguredActiveAbility.cpp` — **0 错误 0 警告**
- 仅有旧代码 C4996（`DynamicAbilityTags`、`NetUpdateFrequency`），不在本次修改范围

## 未修改

- 未使用 MCP
- 未改 `.uasset`
- 未改 `Config/DefaultEditor.ini`
- 未改 Tag 名称和枚举整数值
- 未改旧 `GA_FireBolt` / `UAuraProjectileSpell` / `StartupAbilities`
- `DA_VitalSurge` 资产配置仍需用户手动完成
