# ClaudeCode Third Fix Prompt - Apex ProjectileCast 定点收口

项目：`D:\UnrealProject\Apex`

## 目标

只修复第三轮审查列出的遗漏，不重构、不重写已经正确的链路。

## 必读

```text
Agent/Reviews/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Third_Review.md
Agent/Prompts/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Second_Fix_ClaudeCode_Prompt.md
Agent/00_Coordination/Current_Code_Design.md
```

先执行 `git status --short`。不得 reset、restore、checkout 或清理现有改动。

## 允许修改

```text
Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp
Source/Apex/Private/CombatEntities/Projectile/ApexProjectileDefinition.cpp
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

不得修改 UE 资产，不使用 MCP，不执行 Git add/commit/push。

## 1. 接通 SkillDefinition 到模板 CDO 的 Data Validation

在 `UApexSkillDefinition::IsDataValid()` 中：

1. 使用 `CombineDataValidationResults()` 合并 `Super::IsDataValid()`、本类校验和模板校验。
2. `AbilityTemplateClass` 为空时添加 Error 并返回 Invalid 结果。
3. 直接拒绝 `PolicyConfig.ActivationGroup == EApexAbilityActivationGroup::MAX`。
4. 保留 ExecutionConfig 类型检查。
5. 取得模板 CDO 后必须调用并合并：

```cpp
CDO->ValidateSkillDefinition(*this, Context)
```

模板返回 Invalid 时，SkillDefinition 必须返回 Invalid。

## 2. CanActivateAbility 必须拒绝坏 Spec

在 `UApexGameplayAbility::CanActivateAbility()` 中：

- 继续使用传入的 Handle 和 ASC `FindAbilitySpecFromHandle()`。
- 找不到 Spec 时记录明确 Warning 并返回 `false`。
- `SourceObject` 不是 `UApexSkillDefinition` 时记录明确 Warning 并返回 `false`。
- `ActivationGroup == MAX` 时记录 Warning 并返回 `false`。
- 删除重复的 `#include "Misc/DataValidation.h"`。

不要改动已经正确的 ActivationGroup 阻塞语义。

## 3. 补齐 Aim Task 本地失败收口

在 `SendAimTargetData()` 中，以下任一条件失败都调用现有统一 `CancelAimTarget()` 并立即返回：

- Ability、ActorInfo 或 ASC 无效；
- PlayerController 无效；
- Avatar 无效；
- World 无效；
- Viewport 宽或高不大于 0；
- Deproject 失败。

成功路径继续：

- 填写 SourceLocation 和 TargetLocation；
- 只有本地控制的非 Authority 客户端调用 `CallServerSetReplicatedTargetData()`；
- Listen Server 只本地广播；
- 不回退已经完成的 Data/Cancelled delegate 注册、解绑和安全副本逻辑。

## 4. 补齐 Projectile 自身忽略

在 `AApexProjectile::InitializeProjectile()` 成功初始化路径中：

```cpp
CollisionSphere->IgnoreActorWhenMoving(GetOwner(), true);
CollisionSphere->IgnoreActorWhenMoving(GetInstigator(), true);
```

指针为空时应安全跳过。保留服务器 Blocking + Sweep、客户端 NoCollision、Authority-first Impact 和 `bool InitializeProjectile()`。

## 5. 补齐有限值和引用资产校验

- `UApexProjectileCastAbility::ActivateAbility()`：运行时配置检查加入 `FMath::IsFinite(Config->MaxAimDistance)`。
- `UApexProjectileDefinition::IsDataValid()`：InitialSpeed、MaxLifetime、CollisionRadius、GravityScale 全部先检查 `FMath::IsFinite()`；前三项还必须 > 0。
- `UApexProjectileCastAbility::ValidateSkillDefinition()`：当 ProjectileDefinition 非空时，调用其 `IsDataValid(Context)` 并使用 `CombineDataValidationResults()` 合并结果，保证引用资产的 Class、ImpactEffect、Cue 和数值会影响 SkillDefinition 的最终校验结果。

## 6. 强制自检

```text
rg -n "if \\(!SkillDef\\) return true|StructUtils" Apex.uproject Source/Apex/Apex.Build.cs Source/Apex/Private/AbilitySystem
rg -n "ValidateSkillDefinition\\(\\*this, Context\\)|CombineDataValidationResults" Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
rg -n "IgnoreActorWhenMoving" Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp
rg -n "FMath::IsFinite" Source/Apex/Private/CombatEntities/Projectile/ApexProjectileDefinition.cpp Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
```

第一条不得命中坏 `return true` 或项目依赖；头文件中的 `StructUtils/InstancedStruct.h` include 不在该命令范围内。

## 7. 重新编译

确认没有其他 Build 正在运行后执行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

要求：

- `Result: Succeeded`
- 0 error
- 不出现 StructUtils deprecated warning

若 Mutex 持续被占用，不得虚构成功；报告真实阻塞状态。

## 8. 必须覆盖实施报告

覆盖：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

报告必须反映当前最终代码，至少包含：

- 最新新增/修改文件路径；
- 类、枚举、结构体；
- 全部成员变量和函数签名及职责；
- SkillDefinition -> 模板 CDO Data Validation 链；
- ActivationGroup 链；
- TargetData 的预测客户端、Listen Server、远端服务器 Data/Cancelled 链；
- Projectile 的初始化、Owner 忽略、服务器移动/碰撞、GE、Cue、销毁链；
- 删除 StructUtils 依赖的 UE 5.8 原因；
- 本轮真实编译命令、结果、耗时和警告；
- 未创建 UE 资产、未执行 PIE。

不得保留旧 `AbilitySystem/Shared` 路径、旧 BlueprintNativeEvent 或已删除的 StructUtils 依赖描述。

完成后停止，等待 Codex 最终代码审查。

