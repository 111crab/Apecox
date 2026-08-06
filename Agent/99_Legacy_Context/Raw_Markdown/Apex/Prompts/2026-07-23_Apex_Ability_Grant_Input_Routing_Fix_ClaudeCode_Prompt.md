# ClaudeCode Prompt - Apex Ability 授予与输入路由审查修复

项目：`D:\UnrealProject\Apex`

## 目标

修复首次代码审查发现的输入队列、预测键、资产过滤、Tag 根域和 Avatar 清理问题。不得扩展功能范围。

## 开始前读取

```text
Agent/00_Coordination/Current_Code_Design.md
Agent/Reviews/2026-07-23_Apex_Ability_Grant_Input_Routing_Review.md
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md

Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Public/Input/ApexAbilityInputConfig.h
Source/Apex/Private/Input/ApexAbilityInputConfig.cpp
```

定向参考本地 UE 5.8 Lyra：

```text
D:\UnrealProject\LyraStarterGame\Source\LyraGame\AbilitySystem\LyraAbilitySystemComponent.cpp
```

重点参考 `AbilityInputTagPressed/Released()`、`ProcessAbilityInput()`、`AbilitySpecInputPressed/Released()`，不要复制 Lyra 的 InputBlocked、ActivationGroup 或其他超出本批范围的系统。

先运行 `git status --short`。保留所有现有改动，不使用 reset、restore，不清理范围外文件。

## 允许修改

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Private/Input/ApexAbilityInputConfig.cpp
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

需要范围外修改时停止并报告。

## 修复 1：完整实现输入队列

`AbilityInputTagPressed()`：

- 只匹配 Dynamic Spec Source Tags。
- 只向 Pressed/Held 添加 Handle。
- 不得在 ActivatableAbilities 遍历中调用 `TryActivateAbility()`。

`AbilityInputTagReleased()`：

- 向 Released 添加 Handle。
- 从 Held 移除 Handle。

`ProcessAbilityInput()` 必须按以下顺序实现：

1. 创建或 Reset `AbilitiesToActivate`。
2. 遍历 Held：未激活且策略为 `WhileInputActive` 的 Handle 加入待激活数组。
3. 遍历 Pressed：
   - 找到有效 Spec 和 Ability。
   - 设置 `Spec.InputPressed = true`。
   - 已激活则调用 `AbilitySpecInputPressed()`。
   - 未激活且策略为 `OnInputTriggered` 则加入待激活数组。
4. 统一遍历待激活数组调用 `TryActivateAbility()`。
5. 遍历 Released：
   - 设置 `Spec.InputPressed = false`。
   - 已激活则调用 `AbilitySpecInputReleased()`。
6. Reset Pressed/Released；Held 保留到释放。

使用 `AddUnique()`，避免同一能力在一帧中被重复加入激活列表。删除当前未实际使用的局部数组。

## 修复 2：正确取得输入事件 PredictionKey

`AbilitySpecInputPressed/Released()`：

- 先调用 `Super`。
- 只对 Active Spec 发送事件。
- 优先从 `Spec.GetPrimaryInstance()->GetCurrentActivationInfo()` 取得 Activation PredictionKey。
- 如保留对 legacy NonInstanced 的 `Spec.ActivationInfo` fallback，只能按 UE 5.8 Lyra 的方式用局部 `PRAGMA_DISABLE_DEPRECATION_WARNINGS` / `PRAGMA_ENABLE_DEPRECATION_WARNINGS` 包围。
- 修复后本批不应再出现 `FGameplayAbilitySpec::ActivationInfo` C4996。

不要启用 `bReplicateInputDirectly`，继续使用 Generic Replicated Event。

## 修复 3：修正 AbilitySet 编辑器元数据

在 `FApexAbilitySetAbilityGrant`：

- 删除 `SkillDefinition` 上错误的 `RequiredAssetDataTags = "RowStructure=UApexSkillDefinition"`。
- `TObjectPtr<UApexSkillDefinition>` 的强类型本身就是正确的资产类型限制。
- 给 `InputTag` 增加 `meta = (Categories = "InputTag.Ability")`。

同步修正 `UApexAbilitySet` 注释：幂等性由调用方通过 Handles 先撤销再授予实现，不得声称 `GiveToAbilitySystem()` 本身幂等。

## 修复 4：落实 InputTag 根域

具体输入槽必须满足：

```text
Tag.IsValid()
Tag.MatchesTag(ApexGameplayTags::InputTag_Ability)
!Tag.MatchesTagExact(ApexGameplayTags::InputTag_Ability)
```

落实位置：

- AbilitySet 运行时授予：InputTag 为空仍允许；有效但不属于具体子 Tag 时记录 Warning 并跳过该错误条目。
- AbilitySet Data Validation。
- AbilityInputConfig Data Validation。
- ASC 的 `AbilityInputTagPressed/Released()` 公共入口；非法 Tag 直接返回。

不得新增 GameplayTag、公共 Helper、枚举或类。

## 修复 5：清理 Avatar 输入状态

`AApexPlayerCharacter::RemoveCoreAbilitySet()` 取得 ASC 后：

1. 无论当前是否为 Authority，都调用 `ASC->ClearAbilityInput()`，因为输入队列是本地运行时状态。
2. 再调用 `CoreAbilitySetHandles.TakeFromAbilitySystem(ASC)`；该函数内部只在 Authority 清除 Spec。

保持 UnPossessed 和 EndPlay 的幂等调用，不新增生命周期函数。

## 修复 6：使用 ASC 权威接口

AbilitySet 的授予与回收使用：

```cpp
ASC->IsOwnerActorAuthoritative()
```

替代 `ASC->GetOwnerActor()->HasAuthority()`，避免 ActorInfo 未初始化时解引用空 OwnerActor。

## 禁止事项

- 不新增具体 GA、GE、Cue、CombatEntity、AbilityTask 或 UE 资产。
- 不新增 InputBlocked、OnSpawn、ActivationGroup、TagRelationshipMapping。
- 不改动 Move、Look、Jump、MouseLook。
- 不修改蓝图、地图、`.uproject` 或配置文件。
- 不使用 MCP。
- 不生成 Rider/Visual Studio 项目文件。
- 不执行 Git add、commit、push。
- 不进行顺手重构。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

要求：

- 0 error。
- 报告完整记录 warning；本批不得再产生 `FGameplayAbilitySpec::ActivationInfo` C4996。

遇到 Live Coding 或编辑器占用时保留原始错误并停止，不扩大代码范围绕过。

## 报告

更新：

```text
Agent/Reports/2026-07-23_Apex_Ability_Grant_Input_Routing_Report.md
```

追加“首次审查修复”章节，逐项说明：

- 输入队列新顺序。
- `Spec.InputPressed` 和 Generic Replicated Event 链路。
- PredictionKey 来源。
- 资产过滤元数据修正。
- Tag 根域验证位置。
- Avatar 输入清理。
- 权威检查方式。
- 实际修改文件。
- 完整编译结果和 warning。

完成后停止，等待 Codex 复审。
