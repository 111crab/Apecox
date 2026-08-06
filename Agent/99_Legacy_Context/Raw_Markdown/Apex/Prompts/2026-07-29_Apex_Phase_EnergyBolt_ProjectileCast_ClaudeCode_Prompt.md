# ClaudeCode Prompt - Apex Phase 能量弹与 ProjectileCast 闭环

项目：`D:\UnrealProject\Apex`

## 目标

严格按已批准设计，一次完成首个真实技能所需的 C++ Runtime：

```text
SkillDefinition 多层配置
-> 三态 ActivationGroup
-> ProjectileCast GA 模板
-> Montage GameplayEvent Notify
-> 屏幕中心 Aim TargetData AbilityTask
-> 服务器权威 Projectile
-> Impact GameplayEffect Spec
-> Impact GameplayCue
```

本批只写 C++、编译并输出报告。禁止使用 MCP，禁止创建或修改 UE 资产。

## 开始前必须读取

```text
.agents/ue-project-context.md
Agent/00_Coordination/Working_Agreement.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Agent/00_Coordination/Current_Phase.md
Agent/00_Coordination/Current_Code_Design.md
Agent/02_SkillSystem/Skill_Runbooks/Phase_EnergyBolt_Design.md
Agent/Research_Notes/2026-07-15_Lyra_GAS_Architecture_Study.md

Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h
Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp
Source/Apex/Public/AbilitySystem/Data/ApexSkillDefinition.h
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Public/GameplayTags/ApexGameplayTags.h
Source/Apex/Private/GameplayTags/ApexGameplayTags.cpp
Source/Apex/Apex.Build.cs
```

先运行 `git status --short`。保留全部现有未提交改动，不得 reset、restore、checkout 或清理文件。

实现前定向核对 UE 5.8 与 Lyra 源码中的实际 API，不凭旧版本记忆猜 Include、委托签名或 TargetData 用法：

```text
E:\UE_5.8\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\
D:\UnrealProject\LyraStarterGame\Source\LyraGame\AbilitySystem\
```

重点核对：

- `UGameplayAbility` Cost/Cooldown、`CanActivateAbility()`、`CommitAbility()`。
- `UAbilityTask_WaitGameplayEvent`、`UAbilityTask_PlayMontageAndWait`。
- ASC `AbilityTargetDataSetDelegate`、`ServerSetReplicatedTargetData`、`ConsumeClientReplicatedTargetData`。
- `FGameplayAbilityTargetData_LocationInfo` 的 UE 5.8 构造与网络序列化接口。
- `UAnimNotify::Notify` 在 UE 5.8 的准确签名。
- Lyra ActivationGroup 的激活、结束、取消和计数边界。

## 允许修改

新增：

```text
Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastConfig.h
Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.h
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp

Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp

Source/Apex/Public/Animation/Notifies/ApexAnimNotify_SendGameplayEvent.h
Source/Apex/Private/Animation/Notifies/ApexAnimNotify_SendGameplayEvent.cpp

Source/Apex/Public/CombatEntities/Projectile/ApexProjectileDefinition.h
Source/Apex/Private/CombatEntities/Projectile/ApexProjectileDefinition.cpp
Source/Apex/Public/CombatEntities/Projectile/ApexProjectile.h
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp

Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

修改：

```text
Source/Apex/Public/AbilitySystem/Data/ApexSkillDefinition.h
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h
Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/GameplayTags/ApexGameplayTags.h
Source/Apex/Private/GameplayTags/ApexGameplayTags.cpp
Source/Apex/Apex.Build.cs
```

需要范围外修改时停止并报告，不自行扩大范围。

## 一、SkillDefinition 配置分层

实现以下类型和字段，名称以设计文档为准：

```text
EApexAbilityActivationGroup
FApexSkillCommonConfig
FApexSkillPolicyConfig
FApexSkillPresentationConfig
FApexSkillExecutionConfig
TInstancedStruct<FApexSkillExecutionConfig> ExecutionConfig
```

要求：

- `CommonConfig`：`DisplayName`、`CostEffectClass`、`CooldownEffectClass`。
- `PolicyConfig`：`ActivationGroup`，默认 `ExclusiveReplaceable`。
- `PresentationConfig`：`ActivationMontage`。
- `ExecutionConfig` 只允许一个内联结构，不创建 Fragment/Step 数组。
- `Apex.Build.cs` 增加 `StructUtils` 依赖。
- 使用 UE 5.8 正确的 `StructUtils/InstancedStruct.h`。
- `GetExecutionConfig<T>()` 必须能够读取允许的派生结构，不能把合法派生配置误判为不匹配。
- Data Validation 检查 AbilityTemplateClass、ActivationGroup 和模板专属配置。
- 只有 GA 模板返回非空 RequiredExecutionConfigStruct 时，ExecutionConfig 才是必填；根 GA 或 InputProbe 等无专属配置能力允许为空。
- 模板要求类型和实际类型采用“相同或派生”规则，不采用只允许 exact type 的规则。

不要在本批增加：

```text
AbilityRelationshipTags
RequiredOwnerTags
BlockedOwnerTags
CancelAbilityTags
BlockAbilityTags
完整 TagRelationshipMapping
```

## 二、UApexGameplayAbility 根类

增加：

```text
GetRequiredExecutionConfigStruct
ValidateSkillDefinition
GetCostGameplayEffect
GetCooldownGameplayEffect
GetActivationGroup
CanActivateAbility
```

要求：

- Cost/Cooldown 从当前 AbilitySpec 的 `UApexSkillDefinition::CommonConfig` 读取。
- 仍使用 GAS 原生 `CommitAbility()`，不创建第二套 Cost/Cooldown 应用流程。
- 有 Handle 的检查必须通过 `ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle)` 读取 SourceObject。
- `CanActivateAbility()` 中禁止依赖 `GetCurrentAbilitySpec()`；这是 Aura 阶段已经发生过的错误。
- 没有 SkillDefinition 或配置无效时，明确日志并拒绝激活，不允许空指针崩溃。
- `GetActivationGroup()` 读取当前 SkillDefinition.PolicyConfig。
- 保留现有 `InstancedPerActor`、`LocalPredicted` 和 `ReplicateNo`。
- `ExclusiveReplaceable` 不能通过 `SetCanBeCanceled(false)` 变成不可替换；参考 Lyra 做防护。

## 三、UApexAbilitySystemComponent 并发组

轻量实现：

```text
IsActivationGroupBlocked
AddAbilityToActivationGroup
RemoveAbilityFromActivationGroup
CancelActivationGroupAbilities
NotifyAbilityActivated
NotifyAbilityEnded
```

规则：

- `Independent` 不被组阻塞，也不取消其他技能。
- 活跃的 `ExclusiveBlocking` 阻止新的 `ExclusiveReplaceable` 和 `ExclusiveBlocking`。
- 任意新的 Exclusive 技能激活时取消当前 `ExclusiveReplaceable`。
- 计数在 Ability 激活/结束通知中成对维护。
- 使用 ASC 内部计数，不使用 Loose GameplayTag 冒充计数。
- 共享 GA 类时，关系按当前 AbilitySpec 的 SkillDefinition.PolicyConfig 求值。
- 不修改现有 Pressed/Held/Released 输入路由行为。
- 对重复加入、重复移除和非法 `MAX` 做 `ensure`/日志保护，不能数组越界。

本批不增加 ActivationGroup 失败原因 Tag；完整失败原因与关系映射留到第二个关系样本。

## 四、FApexProjectileCastConfig

字段：

```text
ExecutionEventTag
SpawnSocketName
MaxAimDistance
ProjectileDefinition
```

要求：

- 继承 `FApexSkillExecutionConfig`。
- 默认 `MaxAimDistance = 10000.0f`。
- Data Validation 检查 EventTag、Socket、距离和 ProjectileDefinition。
- 不加入穿透、分裂、追踪、黑洞、任意 Steps。

## 五、UApexProjectileCastAbility

它是可以直接授予的完整 GA 模板：

```text
读取并校验配置
-> CommitAbility
-> 注册 Aim TargetData Task
-> WaitGameplayEvent
-> PlayMontageAndWait
-> GameplayEvent 到达
-> 本地确认 Aim TargetData
-> 服务端等待 Event 与 TargetData 都就绪
-> 服务端生成 Projectile
-> 正确结束或取消
```

硬性要求：

- `ActivateAbility(...)` 在该模板声明为 `final`。
- Montage、GameplayEvent Task 和 Aim TargetData Task 都必须保存为 UPROPERTY，防止生命周期不可见。
- 先绑定委托，再 `ReadyForActivation()`。
- 任何失败路径都必须结束 Ability 并清理委托/Task。
- GameplayEvent 前 Montage 被打断：取消 Ability，不生成 Projectile。
- GameplayEvent 后已经生成的 Projectile 不随 GA 被销毁。
- 防止 Client 与 Server 两边各生成一次：Projectile 只在 Authority 生成。
- 防止 Event、TargetData 或 Montage 回调重复导致重复生成/重复结束。

受控扩展接口：

```text
BuildProjectileSpawnTransform
ExecuteProjectileSpawnOnAuthority
SpawnProjectileActorOnAuthority
```

约束：

- `ExecuteProjectileSpawnOnAuthority()` 是可覆写的生成模式钩子，默认生成一枚。
- `SpawnProjectileActorOnAuthority()` 是复用单枚权威生成的非虚原语。
- 普通技能不派生 GA。
- 将来只有“多个技能共享同一种生成流程差异”时，才派生 Volley 等模板。
- Authority 生成和伤害入口不暴露成任意 `BlueprintImplementableEvent`/`BlueprintNativeEvent`。

## 六、UApexAbilityTask_WaitAimTargetData

工厂和入口命名：

```text
WaitAimTargetData
ConfirmAimTarget
```

职责：

- 本地控制端在 `ConfirmAimTarget()` 时从屏幕中心反投影。
- 使用 Visibility Trace；命中使用 ImpactPoint，未命中使用最大距离端点。
- 生成 `FGameplayAbilityTargetData_LocationInfo`。
- 使用当前 AbilitySpecHandle、ActivationPredictionKey 和 `FScopedPredictionWindow`。
- 非权威本地端通过 `ServerSetReplicatedTargetData` 发送。
- 远端服务器注册 TargetData delegate，接收后消费缓存。
- Listen Server 本地玩家不能出现双回调。
- `OnDestroy()` 必须解除 ASC 委托并清理状态。

服务器只信任“玩家希望瞄准的位置”，不信任最终命中者：

- 检查坐标有限。
- 按 `MaxAimDistance` 对 Avatar 到 AimPoint 的距离做拒绝或钳制。
- 真正伤害目标由服务器 Projectile 碰撞决定。

GameplayEvent 本身不是跨网络 RPC。客户端 Notify 负责触发本地确认，TargetData 才通过 GAS 预测键发送；服务器的 GA/Montage 同时运行并等待同一执行时机。

## 七、UApexAnimNotify_SendGameplayEvent

字段：

```text
EventTag
```

要求：

- 使用 UE 5.8 的准确 `Notify` 签名。
- 通过 ASC/GAS 标准入口向 Montage 所属 Actor 发送一次 `FGameplayEventData`。
- Payload 至少填写 EventTag、Instigator 和 Target。
- 不授予持续 Tag，不应用 GE，不生成 Actor。
- 只允许 Authority 或 LocallyControlled Pawn 发送，避免 Simulated Proxy 重放 Montage 时产生本地玩法事件。
- Dedicated Server 必须能触发该 Notify。
- EventTag 无效、Owner/ASC 不存在时安全返回并给出有意义日志或编辑器验证。

## 八、UApexProjectileDefinition

继承 `UDataAsset`，字段：

```text
ProjectileClass
InitialSpeed = 2000.0f
MaxLifetime = 3.0f
CollisionRadius = 12.0f
GravityScale = 0.0f
ImpactEffectClass
ImpactCueTag
```

要求：

- Data Validation 检查 Class、速度、寿命、碰撞半径、ImpactEffect 和 Cue Tag。
- 不创建空的通用 CombatEntityDefinition 基类。
- 不把施法 Montage、输入、瞄准距离放进 ProjectileDefinition。

## 九、AApexProjectile

组件：

```text
USphereComponent
UProjectileMovementComponent
```

要求：

- C++ 创建组件，Sphere 为 Root。
- `bReplicates = true`，开启 Replicate Movement。
- 只由服务器 Spawn。
- 忽略 Owner/Instigator。
- 第一处有效角色或世界命中只处理一次，然后销毁。
- Source ASC 和 ImpactEffectSpec 只保存在服务器运行时，不尝试复制 `FGameplayEffectSpecHandle`。
- Definition 可作为初始复制配置；如使用 RepNotify，初始化必须幂等。
- 命中有 ASC 的 Actor 时，由服务器应用预先构建的 Impact GE Spec。
- 世界命中没有目标 ASC 时仍执行 Impact Cue 并销毁。
- GameplayCue 只做表现；使用正确的 Location、Normal、Instigator/EffectContext 参数。
- ProjectileClass 可以是 Blueprint 子类，以便用户后续挂 Cascade 飞行特效。
- Spawn/Initialize/FinishSpawning 顺序正确；任一关键配置失败时销毁半成品，不留下无效 Actor。

## 十、GameplayTag

只新增 Native Tag：

```text
GameplayEvent.Ability.Execute
```

C++ 变量：

```text
ApexGameplayTags::GameplayEvent_Ability_Execute
```

以下 Tag 由用户后续在项目设置/资产中配置，本批不要强行注册成 Native：

```text
Cooldown.Ability.Phase.EnergyBolt
GameplayCue.Ability.Phase.EnergyBolt.Impact
```

禁止新增：

```text
Ability.Phase.EnergyBolt
CombatEntity.Projectile.EnergyBolt
Damage.Type.*
Damage.Channel.*
SetByCaller.*
临时执行阶段 Tag
Ability.Type.*
Status.*
```

## 网络与清理底线

- LocalPredicted：本地响应，服务端最终确认。
- TargetData：客户端发送意图，服务端校验。
- Projectile：服务端唯一权威实例。
- Damage：服务端唯一应用。
- Cue：由服务端权威结果驱动到相关端。
- Montage：Owning Client 与 Server 能运行，其他客户端通过 GAS Montage 复制看到。
- Ability 结束时清除 Montage/Event/TargetData Task 和委托。
- Projectile 已生成后拥有独立生命周期。
- 不使用 Tick 轮询等待 Event 或 TargetData。

## 注释要求

使用简洁中文注释解释：

- 为什么 SkillDefinition 是 SourceObject。
- 为什么 ActivationGroup 是枚举而不是 Tag。
- 为什么 `CanActivateAbility()` 必须使用传入 Handle。
- 为什么 GameplayEvent 与 TargetData 是两个网络概念。
- 为什么客户端瞄准不直接决定命中者。
- 为什么 Projectile 的 GE Spec 只保存在服务器。
- 为什么 GA 只开放少量受控 C++ 钩子。

不要给每一行代码写翻译式注释。

## 禁止事项

- 不创建或修改 `.uasset`、蓝图、Montage、GE、Cue、地图和项目设置。
- 不使用 MCP。
- 不修改 Paragon、Phase 或其他第三方资产。
- 不生成 Rider/Visual Studio 项目文件。
- 不执行 Git add、commit、push。
- 不实现伤害公式、ExecCalc、IncomingDamage、队伍或友军规则。
- 不实现客户端预测 Projectile。
- 不实现完整 TagRelationshipMapping、Skill Timeline、任意 Step 解释器或技能编辑器。
- 不创建 Volley、Homing、BlackHole 等尚未进入本批验证的模板/实体。

## 编译

关闭 UE Editor/Live Coding 占用后执行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如果编译失败：

1. 只修复本批引入的问题。
2. 不通过删除网络校验、Data Validation 或生命周期清理来绕过。
3. 记录每次失败的根因和最终修复。
4. 遇到 UE 5.8 API 差异时，以本机引擎源码为准。

## 报告

写入：

```text
Agent/Reports/2026-07-29_Apex_Phase_EnergyBolt_ProjectileCast_Report.md
```

中文报告必须包含：

- 开始与结束时 `git status --short`。
- 实际修改文件，是否超出范围。
- 所有新增/修改类、结构体、枚举、父类和职责。
- 所有新增成员变量：名称、类型、UPROPERTY 暴露和作用。
- 所有新增函数：名称、参数、返回值、UFUNCTION/virtual/final 和作用。
- SkillDefinition 配置分层与 Data Validation。
- ActivationGroup 的激活、阻塞、取消、计数和清理链路。
- ProjectileCast GA 的完整运行链路。
- TargetData 在 Owning Client、Listen Server、Remote Server 的路径。
- Projectile 的复制、命中、GE 和 Cue 路径。
- 新增 GameplayTag。
- Build.cs 依赖变化。
- 编译命令、结果和修复过的编译问题。
- 明确说明未创建 UE 资产、未进行 PIE 验证。
- 用户后续必须人工创建哪些资产，但不要替 Codex 编写最终操作清单。

完成后停止，等待 Codex 审查。
