# ClaudeCode 实施 Prompt：技能系统基础架构 Phase 1

创建日期：2026-07-09  
状态：用户已确认方向，可交给 ClaudeCode 执行  
负责人：ClaudeCode 执行，Codex 审查，用户最终验证

## 任务目标

本轮任务不是迁移旧火球术，也不是继续给旧 `UAuraConfiguredActiveAbility` 加字段。

本轮任务是在 Aura 项目中搭建新的技能系统基础架构，使后续技能可以按下面的思路重新设计：

```text
InputTag
  -> UAuraSkillRuntimeAbility
  -> UAuraSkillDefinition
  -> Instanced UAuraSkillTemplate
  -> Target / CombatEntity / Effect / State / GameplayCue
```

第一阶段只搭地基，不完整重做火球术和生命涌动，不实现完整技能编辑器，不实现完整 BuffDefinition，不重构最终伤害公式。

## 本轮核心决策

1. 新主动技能运行时 GA 使用 `UAuraSkillRuntimeAbility`。
2. `UAuraConfiguredActiveAbility` 不再作为长期架构目标。若它与新设计冲突，可以删除或替换。
3. `UAuraSkillDefinition` 是技能组装入口，不是万能字段表。
4. `UAuraSkillTemplate` 第一版采用 `Instanced UObject` 内联编辑。
5. 普通技能不手动填写 Steps；V1 使用“选择模板 + 填模板参数”。
6. 第一版模板至少建立：
   - `UAuraSkillTemplate`
   - `UAuraSkillTemplate_ProjectileCast`
   - `UAuraSkillTemplate_ChannelCast`
7. 第一版战斗衍生物配置至少建立 `UAuraCombatEntityDefinition` 的最小壳，只考虑 Projectile / Field，不做 Summon / Trap。
8. Buff 系统第一版只整理到 Effect / State，不引入完整 BuffDefinition 层。
9. SetByCaller 第一版只是快速注入运行时数值；正式伤害公式后续再通过属性捕获和 ExecutionCalculation 重构。
10. 火球术和生命涌动只作为行为参考样本，后续在新架构下重新设计，不要求兼容旧 DA 字段。

## 允许删除和替换的内容

用户已明确允许：不符合新设计的旧实现可以直接删除。

但删除必须有工程依据，不能凭感觉清空项目。执行要求：

1. 删除前用 `rg` 检查引用。
2. 如果删除 C++ 类，必须同步删除或替换 include、Build 依赖、引用代码、授予路径。
3. 如果删除或废弃 UPROPERTY 字段，必须考虑已有 `.uasset` 可能带有旧序列化字段；报告中说明是否需要用户在 UE 编辑器中重新创建相关资产。
4. 可以删除或替换：
   - `UAuraConfiguredActiveAbility` 相关 `.h/.cpp`，前提是新 `UAuraSkillRuntimeAbility` 已接管目标职责。
   - `UAuraSkillDefinition` 中不符合新分层设计的旧字段。
   - 旧配置化火球术/生命涌动原型路径中不再使用的代码。
   - 旧原型专用的临时枚举、结构体、函数。
5. 谨慎处理，不要无关删除：
   - 原始教程资产和美术资源。
   - `GA_FireBolt`、`UAuraProjectileSpell`、`AAuraProjectile` 等教程路径，除非它们直接阻碍新架构编译，并在报告中说明原因。
   - `Content/Assets` 下的教程美术资产。
   - 与本轮无关的 UI、AI、PCG、地图、MCP 插件文件。
6. 不要用脚本硬写或硬删 `.uasset`。如需删除/替换 UE 资产，请使用 UE 编辑器/MCP；如果做不到，就在报告里写手动处理步骤。

## 必读文件

实施前请先阅读：

- `.agents/ue-project-context.md`
- `Agent/00_Coordination/WorkingAgreement.md`
- `Agent/00_Coordination/Decision_Log.md`
- `Agent/02_SkillSystem/2026-07-09_SkillSystem_Architecture_Refactor_Summary.md`
- `Agent/02_SkillSystem/Montage_GameplayEvent_Timing_Decision.md`
- `Agent/02_SkillSystem/Skill_Runtime_Risk_Checklist.md`
- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`

建议使用的 UE 知识/Skill：

- `ue-gameplay-abilities`
- `ue-data-assets-tables`
- `ue-cpp-foundations`
- `ue-module-build-system`
- 如涉及输入释放或移动取消，再使用 `ue-input-system`
- 如涉及蒙太奇事件字段，再使用 `ue-animation-system`
- 如涉及网络边界说明，再使用 `ue-networking-replication`

## 允许修改范围

允许新增、修改、删除：

- `Source/Aura/Public/AbilitySystem/Data/`
- `Source/Aura/Private/AbilitySystem/Data/`
- `Source/Aura/Public/AbilitySystem/Abilities/`
- `Source/Aura/Private/AbilitySystem/Abilities/`
- `Source/Aura/Public/AbilitySystem/SkillTemplates/`
- `Source/Aura/Private/AbilitySystem/SkillTemplates/`
- `Source/Aura/Public/AbilitySystem/CombatEntities/`
- `Source/Aura/Private/AbilitySystem/CombatEntities/`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`
- 必要时 `Source/Aura/Aura.Build.cs`
- `Agent/02_SkillSystem/Skill_Runbooks/` 下的中文说明文档
- `Agent/04_AgentHandoff/Claude_Reports/` 下的中文实施报告

不建议本轮修改：

- UI Widget 系统。
- AttributeSet 正式属性重构。
- Damage ExecutionCalculation 正式公式。
- 完整 Skill Timeline。
- 完整技能编辑器。
- 完整 BuffDefinition 层。

## C++ 架构要求

### 1. `UAuraSkillRuntimeAbility`

建议路径：

```text
Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h
Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp
```

职责：

- 作为新的主动技能通用运行时 GA。
- 从当前 `FGameplayAbilitySpec.SourceObject` 读取 `UAuraSkillDefinition`。
- 执行 GAS 标准生命周期：
  - `CanActivateAbility`
  - `ActivateAbility`
  - `CommitAbility`
  - `CancelAbility`
  - `EndAbility`
- 创建并维护一次激活期间的轻量运行时上下文。
- 将具体流程委托给 `UAuraSkillTemplate`。
- 负责失败时安全 `EndAbility`。

建议：

- 继承当前项目合适的 Ability 基类，优先考虑 `UAuraGameplayAbility`，除非现有继承链要求不同。
- `InstancingPolicy` 采用适合异步 AbilityTask 的策略，通常为 `InstancedPerActor` 或 `InstancedPerExecution`。请在注释/报告中说明选择原因。
- `NetExecutionPolicy` 第一版可以沿用当前配置化主动技能已有策略；如果调整，必须说明多人影响。

禁止：

- 不要把 Projectile / Channel / Field 的全部逻辑写死在这个 GA 里。
- 不要在这个 GA 里继续堆巨大 `switch`。
- 不要在客户端权威修改 Health/Mana/State。

### 2. `UAuraSkillDefinition`

建议继续作为 `UPrimaryDataAsset`。

第一版建议字段方向：

```cpp
// Identity
FGameplayTag SkillTag;
FText DisplayName;
FText Description;
TSoftObjectPtr<UTexture2D> Icon;

// Grant / Input
TSubclassOf<UGameplayAbility> RuntimeAbilityClass;
FGameplayTag InputTag;
int32 DefaultLevel = 1;

// Policy
FGameplayTagContainer RequiredOwnerTags;
FGameplayTagContainer BlockedOwnerTags;
FGameplayTagContainer ActivationOwnedTags;
FGameplayTagContainer CancelAbilitiesWithTags;
FGameplayTagContainer BlockAbilitiesWithTags;

// Template
UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Skill")
TObjectPtr<UAuraSkillTemplate> SkillTemplate;
```

说明：

- `RuntimeAbilityClass` 默认为 `UAuraSkillRuntimeAbility`。
- `SkillTemplate` 是内联实例，用户创建 DA 后在细节面板选择具体模板类型。
- 不要保留一堆旧顶层字段来兼容老火球术，例如顶层 `ProjectileClass`、顶层 `DeliveryMode`、顶层 `FullChargeDuration` 等。如果它们属于模板，就放进模板。
- 如果旧字段需要删除，请同步调整引用代码并报告。

### 3. `UAuraSkillTemplate`

建议路径：

```text
Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate.h
Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate.cpp
```

建议声明：

```cpp
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class AURA_API UAuraSkillTemplate : public UObject
{
    GENERATED_BODY()
public:
    virtual bool CanActivateTemplate(...) const;
    virtual void ActivateTemplate(...);
    virtual void CancelTemplate(...);
    virtual void EndTemplate(...);
};
```

具体参数请结合项目风格设计，可以使用 `UAuraSkillRuntimeAbility*`、`const FGameplayAbilityActorInfo*`、`FGameplayAbilityActivationInfo`、`FGameplayAbilitySpecHandle` 或自定义 `FAuraSkillRuntimeContext`。

要求：

- 解释为什么模板是 `UObject` 而不是 `USTRUCT`。
- 解释 `EditInlineNew` / `DefaultToInstanced` 的意义。
- 避免模板对象持有运行时强状态；运行时状态放在 Ability 或 RuntimeContext 中。
- 模板可以创建 AbilityTask，但 AbilityTask 是实现工具，不是暴露给普通配置者手填的 Step。

### 4. `UAuraSkillTemplate_ProjectileCast`

建议路径：

```text
Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate_ProjectileCast.h
Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate_ProjectileCast.cpp
```

本轮只做最小壳和字段，不要求完整重做火球术。

建议字段：

```cpp
TSoftObjectPtr<UAnimMontage> CastMontage;
FGameplayTag FireEventTag;
TObjectPtr<UAuraCombatEntityDefinition> ProjectileEntity;
```

如果需要目标规则，可以先做轻量枚举或结构：

```cpp
EAuraSkillTargetRule TargetRule = CursorHitPoint;
```

要求：

- 说明 ProjectileCast 的内部默认流程，但不要要求用户手填 Steps。
- 可以暂时不完整实现发射逻辑，但接口要能支撑下一轮火球术重做。

### 5. `UAuraSkillTemplate_ChannelCast`

建议路径：

```text
Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate_ChannelCast.h
Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate_ChannelCast.cpp
```

本轮只做最小壳和字段，不要求完整重做生命涌动。

建议字段：

```cpp
float FullDuration = 2.f;
bool bCancelOnMoveInput = false;
FGameplayTag FullOutcomeTag;
FGameplayTag PartialOutcomeTag;
FGameplayTag CancelledOutcomeTag;
```

Effect/GE 字段可以先预留最小结构，不要做完整 BuffDefinition。

要求：

- 说明 ChannelCast 是通用引导/蓄力模板，不是治疗专用模板。
- 说明 Full / Partial / Cancelled 是单次执行结果，不是长期角色状态。

### 6. `UAuraCombatEntityDefinition`

建议路径：

```text
Source/Aura/Public/AbilitySystem/CombatEntities/AuraCombatEntityDefinition.h
Source/Aura/Private/AbilitySystem/CombatEntities/AuraCombatEntityDefinition.cpp
```

第一版可以继承 `UPrimaryDataAsset` 或 `UDataAsset`。如果未来希望 AssetManager 扫描，优先 `UPrimaryDataAsset`；请说明选择原因。

建议最小字段：

```cpp
EAuraCombatEntityType EntityType; // Projectile / Field
TSoftClassPtr<AActor> ActorClass;
float Lifetime = 0.f;
```

可以预留但不必完整实现：

- Detection shape。
- Team filter。
- OnHit effect。
- OnTick effect。
- OnEnd cue。

要求：

- 说明 CombatEntity 与 SkillDefinition 的目标不同：SkillDefinition 关心释放目标，CombatEntity 关心生成后的检测目标。
- 不要本轮实现召唤物、陷阱、AI。

### 7. Effect / State 第一版

本轮只做最小结构或命名准备。

建议：

- 不新增完整 `UAuraBuffDefinition`。
- 可新增轻量 `FAuraSkillEffectSpec` 或类似结构，如果模板需要引用 GE。
- State 用 GameplayTag 表达，例如：
  - `State.Skill.Channeling`
  - `Outcome.Channel.Full`
  - `Outcome.Channel.Partial`
  - `Outcome.Channel.Cancelled`
- 如果 C++ 直接引用这些 Tag，加入 `FAuraGameplayTags` native tags；否则可先在 `DefaultGameplayTags.ini` 配置。

### 8. 授予路径

更新或新增 ASC 授予函数，使新 `UAuraSkillDefinition` 能授予 `UAuraSkillRuntimeAbility`。

核心规则：

```text
SkillDefinition
  -> RuntimeAbilityClass or UAuraSkillRuntimeAbility
  -> FGameplayAbilitySpec
  -> Spec.SourceObject = SkillDefinition
  -> Spec.DynamicAbilityTags.AddTag(InputTag)
  -> GiveAbility
```

如果旧授予路径和新设计冲突，可以删除或替换旧配置化授予函数。

请保留教程 `StartupAbilities` 路径，除非它直接造成编译冲突。它可以作为原始教程对照，不属于本轮必须删除对象。

## GameplayTag 建议

新增或确认：

```text
State.Skill.Channeling
Outcome.Channel.Full
Outcome.Channel.Partial
Outcome.Channel.Cancelled
SetByCaller.Damage
SetByCaller.Healing
Damage.Physical
Damage.Magical
```

注意：

- 本轮不要大规模规范化所有旧 Attribute Tag。
- 本轮不要求删除 `Damage.Fire` 等旧教程 Tag，但新设计文档应说明第一版正式伤害类型先面向 Physical / Magical。

## 编译要求

请使用 UE 5.8 构建命令：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat AuraEditor Win64 Development -Project="D:\UnrealProject\Aura\Aura.uproject" -WaitMutex -MaxParallelActions=4
```

如果环境无法运行编译，请在报告中明确原因，不要假装已通过。

## 注释与文档要求

新增 C++ 注释必须帮助用户学习，而不是解释显而易见的语法。

重点解释：

1. GA 生命周期和 SkillTemplate 流程的区别。
2. 为什么第一版主动技能可以只有一个 `UAuraSkillRuntimeAbility`。
3. 为什么模板使用内联 UObject。
4. 为什么不手填 Steps。
5. 为什么 CombatEntity 要从 SkillDefinition 拆出来。
6. 为什么 GameplayCue 只做表现。
7. 为什么 SetByCaller 只是运行时数值入口，不是正式伤害公式。

如果解释太长，请写入中文 Markdown，而不是把代码注释写得很臃肿。

## 必须输出的报告

请写入：

```text
Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillArchitecture_Foundation_Phase1_Report.md
```

报告必须包含：

1. 简要总结。
2. 新增/修改/删除文件列表。
3. 删除内容的理由和引用检查结果。
4. 新增类职责说明。
5. `UAuraSkillRuntimeAbility` 与 `UAuraSkillTemplate` 的关系说明。
6. `UAuraSkillDefinition` 新结构说明。
7. CombatEntityDefinition 第一版边界。
8. Effect / State 第一版边界。
9. 是否执行编译，编译结果如何。
10. 如果未编译，原因是什么。
11. 尚未实现的内容。
12. 下一步建议：用新架构重新设计火球术。

## 验收标准

本轮完成时至少应满足：

1. C++ 可编译，或报告清楚说明未编译原因。
2. `UAuraSkillRuntimeAbility` 存在，并能作为 `UAuraSkillDefinition.RuntimeAbilityClass` 使用。
3. `UAuraSkillDefinition` 可以内联编辑 `UAuraSkillTemplate`。
4. `UAuraSkillTemplate_ProjectileCast` 和 `UAuraSkillTemplate_ChannelCast` 至少有可编译的最小壳。
5. `UAuraCombatEntityDefinition` 至少有 Projectile / Field 的类型边界。
6. 不再为了兼容旧火球术/生命涌动而保留明显违背新设计的字段。
7. 删除过的内容都有报告说明。
8. 没有引入 Editor-only 依赖到 Runtime 模块。
9. 报告和关键说明为中文。
