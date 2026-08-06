# ClaudeCode Second Fix Prompt - Apex ProjectileCast 剩余问题

项目：`D:\UnrealProject\Apex`

## 目标

只修复第二轮审查仍未关闭的问题。第一轮已经正确修复的 ActivationGroup、LocationInfo EndPoint 和 Event/Data 双就绪逻辑不得回退。

## 必读

```text
Agent/Reviews/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Review.md
Agent/Reviews/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Second_Review.md
Agent/Prompts/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Fix_ClaudeCode_Prompt.md
Agent/00_Coordination/Current_Code_Design.md
```

先执行 `git status --short`。不得 reset、restore、checkout 或清理现有改动。

## 允许修改

```text
Apex.uproject
Source/Apex/Apex.Build.cs
Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h
Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.h
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
Source/Apex/Public/CombatEntities/Projectile/ApexProjectile.h
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp
Source/Apex/Private/CombatEntities/Projectile/ApexProjectileDefinition.cpp
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

不得修改 UE 资产，不使用 MCP，不执行 Git add/commit/push。

## 1. 移除 UE 5.8 已弃用的 StructUtils 插件依赖

本机 UE 5.8 的 `InstancedStruct.h` 已属于 CoreUObject：

```text
E:\UE_5.8\Engine\Source\Runtime\CoreUObject\Public\StructUtils\InstancedStruct.h
```

执行：

- 从 `Apex.Build.cs` 删除 StructUtils module dependency。
- 从 `Apex.uproject` 删除 StructUtils plugin block。
- 保留 `#include "StructUtils/InstancedStruct.h"`。
- 不删除 `TInstancedStruct` 设计。

最终编译不得再出现 StructUtils deprecated warning。

## 2. 真正接通 Data Validation

`UApexSkillDefinition::IsDataValid()` 必须：

1. 校验 `AbilityTemplateClass`。
2. 校验 `PolicyConfig.ActivationGroup != MAX`。
3. 模板要求 ExecutionConfig 时，校验实际结构相同或派生。
4. 调用模板 CDO：

```cpp
CDO->ValidateSkillDefinition(*this, Context)
```

5. 合并返回结果；模板返回 Invalid 时资产必须 Invalid。

`UApexProjectileCastAbility::ValidateSkillDefinition()` 额外检查：

- `MaxAimDistance` 为有限值且 > 0。
- ProjectileDefinition 存在。
- ProjectileDefinition 的 Class、ImpactEffect、Cue 和数值有效。
- ActivationMontage Soft Pointer 非空。

`UApexProjectileDefinition::IsDataValid()` 对全部 float 同时检查 `FMath::IsFinite()`。

## 3. 修复 CanActivateAbility 缺少 SourceObject

`UApexGameplayAbility::CanActivateAbility()`：

- 找不到 Spec 或 SkillDefinition 时写明确 Warning 并返回 false。
- ActivationGroup 为 MAX 时返回 false。
- 继续使用传入 Handle 查找 Spec。

## 4. 完整实现 TargetData Data/Cancelled 双路径

头文件新增：

```text
AimDataCancelled delegate
OnTargetDataReplicatedCancelled()
TargetDataCancelledDelegateHandle
```

`Activate()`：

- 只有非本地控制端注册 DataSet 和 DataCancelled 两个 delegate。
- 两个 delegate 都必须在 `OnDestroy()` 成对解除。
- 调用 `CallReplicatedTargetDataDelegatesIfSet()` 后仍保留等待状态。

本地确认：

- Controller、Avatar、World、Viewport 或 Deproject 任一失败都调用统一 Cancel 函数。
- Cancel 在预测客户端调用 `ServerSetReplicatedTargetDataCancelled()`，随后本地广播 `AimDataCancelled`。
- Listen Server 本地玩家只本地广播，不向自己调用 Server RPC。

成功数据：

- LocationInfo 同时填写 SourceLocation 和 TargetLocation。
- 只有 `IsLocallyControlled() && !IsNetAuthority()` 时调用 `CallServerSetReplicatedTargetData()`。
- Listen Server 直接本地广播。

服务端接收：

- 先复制/取得安全的 `FGameplayAbilityTargetDataHandle`。
- 然后 Consume ASC 缓存。
- 广播安全副本，不能在 Consume 后继续广播传入引用。
- Cancelled delegate 同样 Consume 并广播 `AimDataCancelled`。

ProjectileCast GA：

- 绑定 `AimDataCancelled`。
- Data Cancelled 时安全取消 Ability。
- `EndAbility()` 清理两个 Task delegate 所属 Task。

## 5. Montage 必须在 Commit 前成功加载

在 `ActivateAbility()`：

1. 读取并检查 Soft Pointer。
2. `LoadSynchronous()`。
3. 返回空则结束 Ability。
4. 只有成功取得 Montage 后才 `CommitAbility()`。
5. 创建 Montage Task 失败也必须结束，不允许永久 Active。

`MaxAimDistance` 的运行时检查必须包含 `FMath::IsFinite()`。

## 6. 完整修复 Projectile 初始化与权威碰撞

将：

```cpp
void InitializeProjectile(...)
```

改为：

```cpp
bool InitializeProjectile(...)
```

只有满足以下条件才返回 true：

- Authority。
- Definition 有效。
- Impact Spec 有效。
- InitialSpeed、MaxLifetime、CollisionRadius、GravityScale 都是有限值。
- Speed/Lifetime/Radius > 0。
- CollisionSphere 和 ProjectileMovement 有效。

初始化时：

- 设置半径、速度、重力和寿命。
- 显式设置 ProjectileMovement Velocity 为 Actor Forward * InitialSpeed。
- 对 Owner 和 Instigator 调用 `IgnoreActorWhenMoving()`。
- 确保 ProjectileMovement 正确 Activate。

生成函数：

- 检查 `InitializeProjectile()` 返回值。
- 失败时 Destroy 半成品并返回 nullptr。
- 只有成功才 FinishSpawning/返回。

碰撞：

- 服务端使用 QueryOnly + Blocking WorldStatic/WorldDynamic/Pawn。
- 以 ProjectileMovement Sweep / `OnProjectileStop` 作为第一命中主路径。
- 删除不再需要的 Overlap 回调，或保证它只在 Authority 且不会重复。
- 客户端 BeginPlay 时关闭 CollisionSphere Collision。
- `HandleImpact()` 开头必须：
  - `if (!HasAuthority() || bHasImpacted) return;`
  - 拒绝 Owner/Instigator。
- 有目标 ASC 时应用 GE。
- 世界命中仍执行 Cue。
- 服务端只处理一次并 Destroy。

不能再保留“客户端先把 bHasImpacted 设为 true”的行为。

## 7. 不得回退的修复

必须继续保留：

- `NotifyAbilityActivated/Ended` 真正覆写。
- IgnoreHandle 取消旧 Replaceable。
- `EApexAbilityActivationGroup::MAX`。
- ExecutionConfig 纯数据及 Data 目录。
- ProjectileCast `NotBlueprintable` + `ActivateAbility final`。
- 每次激活状态重置。
- `HasEndPoint()` / `GetEndPoint()`。
- Event 和 Data 都调用 `TryExecuteProjectileSpawn()`。
- 服务端创建 Impact GE Spec。
- Socket 存在性检查。

## 8. 强制自检

修复后运行并检查：

```text
rg -n "StructUtils" Apex.uproject Source/Apex/Apex.Build.cs
rg -n "AimDataCancelled|AbilityTargetDataCancelledDelegate|CallServerSetReplicatedTargetData" Source/Apex
rg -n "bool InitializeProjectile|IgnoreActorWhenMoving|!HasAuthority" Source/Apex/Private/CombatEntities Source/Apex/Public/CombatEntities
rg -n "ValidateSkillDefinition" Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
rg -n "HasHitResult|GetHitResult|NotifyAbilityActivated_Group|NotifyAbilityEnded_Group" Source/Apex
```

验收：

- 第一条应无结果。
- 第二、三、四条必须出现新实现。
- 第五条应无旧错误实现。

## 9. 重新编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

要求：

- `Result: Succeeded`
- 0 error
- 不出现 StructUtils deprecated warning

## 10. 必须覆盖报告

覆盖：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

报告标题标明“第二轮修复后最终报告”，并包含：

- 最新文件路径，不能再写 `AbilitySystem/Shared`。
- 所有真实范围变化。
- 类、枚举、结构体。
- 完整成员变量和函数签名。
- Data Validation 调用链。
- ActivationGroup 调用链。
- Data/Cancelled 在预测客户端、Listen Server、远端服务器的路径。
- Projectile Authority、初始化、移动、碰撞、GE、Cue、销毁路径。
- 删除 StructUtils 依赖的 UE 5.8 原因。
- 最新编译完整结果和耗时。
- 自检命令结果。
- 未创建 UE 资产、未 PIE。

如果没有覆盖报告，不得宣布完成。

完成后停止，等待 Codex 第三轮审查。
