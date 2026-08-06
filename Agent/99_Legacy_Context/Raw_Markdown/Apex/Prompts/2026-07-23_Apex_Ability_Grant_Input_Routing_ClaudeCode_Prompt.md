# ClaudeCode Prompt - Apex Ability 授予与输入路由

项目：`D:\UnrealProject\Apex`

## 目标

严格按 `Agent/00_Coordination/Current_Code_Design.md`，一次实现：

```text
UApexGameplayAbility
UApexSkillDefinition
UApexAbilitySet
UApexAbilityInputConfig
Native InputTag
ASC Pressed/Held/Released 输入路由
Character 授予与输入桥接
PlayerController 每帧输入处理
```

本批只建立基础闭环，不实现具体技能模板。

## 开始前读取

```text
.agents/ue-project-context.md
Agent/00_Coordination/Working_Agreement.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Agent/00_Coordination/Current_Code_Design.md
Agent/Research_Notes/2026-07-15_Skill_Framework_Extensibility_Synthesis.md
Agent/90_References/AttributeSet_Best_Practices/Lyra_AttributeSet_Best_Practices_Summary.md

Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
Source/Apex/Public/Player/ApexPlayerState.h
Source/Apex/Private/Player/ApexPlayerState.cpp
Source/Apex/Apex.Build.cs
```

先运行 `git status --short`。保留全部现有未提交改动，不得 reset、restore、清理或覆盖范围外文件。

## 允许修改

新增：

```text
Source/Apex/Public/GameplayTags/ApexGameplayTags.h
Source/Apex/Private/GameplayTags/ApexGameplayTags.cpp
Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h
Source/Apex/Private/AbilitySystem/Abilities/ApexGameplayAbility.cpp
Source/Apex/Public/AbilitySystem/Data/ApexSkillDefinition.h
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Public/Input/ApexAbilityInputConfig.h
Source/Apex/Private/Input/ApexAbilityInputConfig.cpp
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

修改：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
Source/Apex/Apex.Build.cs
```

需要范围外修改时停止并报告。

## 硬性要求

- 类名、结构体名、函数名、属性名、Tag 名和生命周期严格以 `Current_Code_Design.md` 为准。
- `UApexGameplayAbility::GetSkillDefinition()` 必须声明为 `UFUNCTION(BlueprintPure, Category = "Apex|Ability")`，确保后续输入探针技能能在蓝图中读取 AbilitySpec 的 SkillDefinition SourceObject。
- AbilitySet 条目必须引用 SkillDefinition，不得直接配置 GA Class。
- AbilitySpec SourceObject 必须是对应 SkillDefinition。
- InputTag 必须写入 Spec Dynamic Source Tags，不使用旧式 InputID。
- `GiveAbility/ClearAbility` 只在权威端执行。
- `AbilitySpecInputPressed/Released` 必须使用 GAS Generic Replicated Event，支持 WaitInputPress/Release。
- Character 现有 Jump、Move、Look、MouseLook 成员与绑定不得删除、重命名或重构。
- CoreAbilitySet 的授予和撤销必须幂等，不能因重复 Possess/EndPlay 留下重复 Spec。
- DataAsset 不保存运行时 Handle。
- 为 SkillDefinition、AbilitySet 和 AbilityInputConfig 实现编辑器 Data Validation。
- `GameplayTags` 调整为公共模块依赖；不要加入 `StructUtils`，本批还没有 `FInstancedStruct`。
- 使用有价值的中文注释解释分层、权威输入边界、SourceObject 和 Handle 回收原因。

## 禁止事项

- 不创建 Projectile、Channel、Area 等模板类。
- 不创建 GA/GE/Cue/CombatEntity/AbilityTask 的具体业务实现。
- 不加入 OnSpawn、ActivationGroup、InputBlocked 或 TagRelationshipMapping。
- 不修改 UE 资产、蓝图、地图、配置文件或 `.uproject`。
- 不使用 MCP。
- 不生成 Rider/Visual Studio 项目文件。
- 不执行 Git add、commit、push。

## 参考原则

可以定向参考本地 Lyra 的：

```text
LyraGameplayAbility
LyraAbilitySet
LyraInputConfig
LyraAbilitySystemComponent 的输入队列
LyraPlayerController::PostProcessInput
```

只移植本设计明确要求的轻量部分，不复制 Experience、PawnData、GameFeature、AbilityGroup、额外 Cost 或失败消息系统。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如遇 Live Coding、编辑器或其他构建进程占用，保留原始错误并停止，不通过扩大代码范围绕过。

## 报告

写入：

```text
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

中文报告必须列出：

- 实际修改文件和是否超出范围。
- 所有新增类、结构体、枚举、父类和职责。
- 所有新增成员、函数和 UPROPERTY/UFUNCTION 暴露方式。
- 五个 Native GameplayTag。
- AbilitySet 授予、SourceObject、Handle 回收和输入队列链路。
- Character 授予/撤销时序与 PlayerController 输入处理时序。
- Data Validation 规则。
- 是否新增 GA、GE、Cue、CombatEntity、AbilityTask 或 UE 资产。
- 编译命令和结果。
- 尚未执行的 UE 单人/多人验证。

完成后停止，等待 Codex 审查。
