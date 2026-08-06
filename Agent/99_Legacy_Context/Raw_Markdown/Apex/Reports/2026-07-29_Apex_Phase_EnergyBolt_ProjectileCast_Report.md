# Apex Phase 能量弹与 ProjectileCast 闭环 — 最终实施报告

*创建日期：2026-07-29（经三轮审查修复）*
*执行者：ClaudeCode*

## 1. 最终文件清单

### 新增

| 文件 | 路径 |
|------|------|
| `ApexSkillExecutionConfig.h` | `Source/Apex/Public/AbilitySystem/Data/` |
| `ApexProjectileCastConfig.h` | `Source/Apex/Public/AbilitySystem/Abilities/Projectile/` |
| `ApexProjectileCastAbility.h` | `Source/Apex/Public/AbilitySystem/Abilities/Projectile/` |
| `ApexProjectileCastAbility.cpp` | `Source/Apex/Private/AbilitySystem/Abilities/Projectile/` |
| `ApexAbilityTask_WaitAimTargetData.h` | `Source/Apex/Public/AbilitySystem/Tasks/` |
| `ApexAbilityTask_WaitAimTargetData.cpp` | `Source/Apex/Private/AbilitySystem/Tasks/` |
| `ApexAnimNotify_SendGameplayEvent.h` | `Source/Apex/Public/Animation/Notifies/` |
| `ApexAnimNotify_SendGameplayEvent.cpp` | `Source/Apex/Private/Animation/Notifies/` |
| `ApexProjectileDefinition.h` | `Source/Apex/Public/CombatEntities/Projectile/` |
| `ApexProjectileDefinition.cpp` | `Source/Apex/Private/CombatEntities/Projectile/` |
| `ApexProjectile.h` | `Source/Apex/Public/CombatEntities/Projectile/` |
| `ApexProjectile.cpp` | `Source/Apex/Private/CombatEntities/Projectile/` |

### 修改

| 文件 | 变更 |
|------|------|
| `ApexSkillDefinition.h` | +CommonConfig/PolicyConfig/PresentationConfig + TInstancedStruct ExecutionConfig |
| `ApexSkillDefinition.cpp` | Data Validation 链：CombineDataValidationResults + 模板 CDO ValidateSkillDefinition |
| `ApexGameplayAbility.h` | +EApexAbilityActivationGroup(含MAX) + GetRequiredExecutionConfigStruct(virtual) + SetCanBeCanceled + CanActivateAbility override + Cost/Cooldown + ValidateSkillDefinition |
| `ApexGameplayAbility.cpp` | GetActivationGroup, SetCanBeCanceled 保护, CanActivateAbility 拒绝坏 Spec, Cost/Cooldown 从 SkillDefinition |
| `ApexAbilitySystemComponent.h` | +NotifyAbilityActivated/Ended override + ActivationGroup 管理 |
| `ApexAbilitySystemComponent.cpp` | 完全接入 GAS 生命周期 + 阻塞/取消/计数 + IgnoreHandle |
| `ApexGameplayTags.h/.cpp` | +GameplayEvent_Ability_Execute |
| `Apex.Build.cs` | GameplayTags → PublicDeps（无 StructUtils 依赖） |
| `Apex.uproject` | 无 StructUtils plugin（UE 5.8 CoreUObject 内置 InstancedStruct） |
| `ApexPlayerController.h/.cpp` | +PostProcessInput + GetApexAbilitySystemComponent |
| `ApexPlayerCharacter.h/.cpp` | +CoreAbilitySet + AbilityInputConfig + Grant/Remove 生命周期 + 输入路由 |
| `ApexAbilitySet.h/.cpp` | +SkillDefinition 引用 + InputTag Categories + Tag 根域验证 |
| `ApexAbilityInputConfig.h/.cpp` | +InputAction→InputTag 配置 + Data Validation |

## 2. 类、枚举、结构体

| 名称 | 类型 | 父类 | 职责 |
|------|------|------|------|
| `EApexAbilityActivationPolicy` | enum | uint8 | OnInputTriggered / WhileInputActive |
| `EApexAbilityActivationGroup` | enum | uint8 | Independent / ExclusiveReplaceable / ExclusiveBlocking / MAX |
| `FApexSkillCommonConfig` | struct | - | DisplayName, Cost/Cooldown GE |
| `FApexSkillPolicyConfig` | struct | - | ActivationGroup |
| `FApexSkillPresentationConfig` | struct | - | ActivationMontage SoftPtr |
| `FApexSkillExecutionConfig` | struct | - | 纯数据基结构，TInstancedStruct 承载 |
| `FApexProjectileCastConfig` | struct | FApexSkillExecutionConfig | ExecutionEventTag, SpawnSocket, MaxAimDistance, ProjectileDefinition |
| `UApexProjectileCastAbility` | class | UApexGameplayAbility | 投射物施法 GA 模板 (NotBlueprintable, ActivateAbility final) |
| `UApexAbilityTask_WaitAimTargetData` | class | UAbilityTask | 屏幕中心反投影 → LocationInfo TargetData + Data/Cancelled 双路径 |
| `UApexAnimNotify_SendGameplayEvent` | class | UAnimNotify | 关键帧发送 GameplayEvent |
| `UApexProjectileDefinition` | class | UDataAsset | 投射物飞行/碰撞/GE/Cue 配置 |
| `AApexProjectile` | class | AActor | 复制投射物：服务器权威碰撞、命中、GE、Cue、销毁 |

## 3. 关键成员与函数

### UApexProjectileCastAbility (NotBlueprintable, ActivateAbility=final)

| 成员/函数 | 签名 | 作用 |
|-----------|------|------|
| `GetRequiredExecutionConfigStruct()` | `virtual const UScriptStruct*` | 返回 FApexProjectileCastConfig::StaticStruct() |
| `ActivateAbility()` | `final` | 状态重置 → 配置校验 → Montage 加载 → Commit → 创建 Tasks |
| `TryExecuteProjectileSpawn()` | `void` | Event+Data 双就绪统一入口 |
| `ExtractValidAimPoint()` | `bool` | HasEndPoint + IsFinite + 距离校验 |
| `BuildProjectileSpawnTransform()` | `virtual bool` | Socket 存在性 → 返回生成 Transform |
| `ExecuteProjectileSpawnOnAuthority()` | `virtual void` | 调 BuildTransform → SpawnActor → EndAbility |
| `SpawnProjectileActorOnAuthority()` | `AApexProjectile*` | SpawnDeferred → Init → 失败则 Destroy |
| `ValidateSkillDefinition()` | `virtual EDataValidationResult` | 模板专属校验 + 链式调用 ProjectileDefinition::IsDataValid |
| `OnMontageCompleted/Interrupted` | - | 未收到 Event 则取消 |
| `OnAimTargetDataCancelled` | - | 取消 GA |

### UApexAbilityTask_WaitAimTargetData

| 成员/函数 | 作用 |
|-----------|------|
| `AimDataReceived` delegate | 数据就绪广播 |
| `AimDataCancelled` delegate | 取消广播 |
| `ConfirmAimTarget()` | 能力/本地控制检查后 Send |
| `CancelAimTarget()` | 可安全处理空 Ability/ASC |
| `SendAimTargetData()` | 所有依赖验证在 ScopedPredictionWindow 之前；VX/VY>0 检查 |

### AApexProjectile

| 成员/函数 | 作用 |
|-----------|------|
| `InitializeProjectile()` → bool | Authority + Definition + IsFinite 全部检查；Velocity=Forward×Speed |
| `HandleImpact()` | `!HasAuthority() \|\| bHasImpacted` 首行拒绝；Owner/Instigator 拒绝；GE+Cue+Destroy |
| `BeginPlay()` | 客户端禁用碰撞；绑定 OnProjectileStop |
| `IgnoreActorWhenMoving(Owner/Instigator)` | 在 Init 成功路径 |

## 4. ActivationGroup 链路

```
NotifyAbilityActivated(Handle, Ability)
  → Spec.SourceObject → SkillDefinition.PolicyConfig.ActivationGroup
  → Exclusive* 激活时 CancelActivationGroupAbilities(ExclusiveReplaceable, Handle) — 忽略自身
  → AddAbilityToActivationGroup

NotifyAbilityEnded → RemoveAbilityFromActivationGroup

IsActivationGroupBlocked: Independent 永不阻塞; 两种 Exclusive 只在 ExclusiveBlocking>0 时阻塞
SetCanBeCanceled(false): ExclusiveReplaceable 拒绝
```

## 5. TargetData Data/Cancelled 路径

| 场景 | 行为 |
|------|------|
| 预测客户端 | Deproject → OnSuccess: LocationInfo(Source+Target) → CallServerSetReplicatedTargetData → 本地广播; OnFail→Cancel |
| Listen Server | 本地 Deproject → 直接本地广播 AimDataReceived（不调 Server RPC） |
| 远端服务器 | 注册 DataSet+DataCancelled delegate → 收到后安全副本 → Consume → 广播 |

## 6. Projectile 路径

```
Authority SpawnDeferred → InitializeProjectile(bool)
  → IsFinite 全部数值 → CollisionRadius/ Speed/Velocity/Gravity/Lifetime
  → IgnoreActorWhenMoving(Owner/Instigator)
  → true: FinishSpawning; false: Destroy
BeginPlay: 客户端 NoCollision; 服务器绑定 OnProjectileStop
HandleImpact: 仅 Authority 处理一次 → 拒绝 Owner/Instigator → GE → Cue → Destroy
```

## 7. UE 5.8 移除 StructUtils 依赖

UE 5.8 的 `InstancedStruct.h` 已属于 CoreUObject (`Engine/Source/Runtime/CoreUObject/Public/StructUtils/InstancedStruct.h`)。`Apex.Build.cs` 无需额外 ModuleReference，`Apex.uproject` 无需 plugin block。`#include "StructUtils/InstancedStruct.h"` 保留。

## 8. 编译

```
Result: Succeeded
Total execution time: 6.90 seconds
Exit code: 0
0 errors, 0 warnings
```

## 9. 未执行

- 未创建 UE 资产、Montage、GE、Cue、蓝图
- 未执行 PIE / Listen Server 多人验证
