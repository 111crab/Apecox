# Apex Ability 授予与输入路由审查

审查日期：2026-07-23  
状态：最终通过；代码、编译、单人和多人 PIE 运行验证均已完成。

## 范围与编译结论

- 实际新增 10 个源码文件、修改 7 个源码文件，符合本批 Prompt 范围。
- 没有提前创建具体技能、Projectile/Channel/Area 模板、GE、Cue、CombatEntity、AbilityTask 或 UE 资产。
- 子代理报告记录 `ApexEditor Win64 Development` 编译成功，但仍有 2 个 `FGameplayAbilitySpec::ActivationInfo` 弃用警告。
- 类名、结构体名、主要成员名和五个 Native GameplayTag 均符合已确认设计。

编译成功不代表输入链路已经成立。下面的问题会影响长按、松键、预测、重生以及编辑器配置，必须先修复再进入 UE 验证。

## P1 - Pressed/Released 队列没有被真正处理

位置：

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:10
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:59
```

当前实现存在三个相互关联的问题：

1. `AbilityInputTagPressed()` 在遍历 ActivatableAbilities 时直接调用 `TryActivateAbility()`。
2. `ProcessAbilityInput()` 只处理 Held，没有处理 Pressed 和 Released。
3. `Spec.InputPressed` 从未更新，`AbilitySpecInputPressed/Released()` 也从未被调用。

结果：

- `WaitInputPress` / `WaitInputRelease` 收不到正确事件。
- 蓄力、引导和“按下开始、松开结算”能力可能立即结束或无法结算。
- 已激活能力无法收到再次按下或松开。
- 激活发生在 ActivatableAbilities 遍历内部；如果能力激活过程授予或移除 Spec，存在容器失效风险。
- 文件中的 `AbilitySpecHandlesToProcess` 只 Reset、从未使用，说明原本预期的集中激活流程没有完成。

修复必须采用 Lyra 已验证的集中处理顺序：

```text
InputTag 回调只入队
-> Held 收集 WhileInputActive
-> Pressed 设置 InputPressed、向已激活能力发事件、收集 OnInputTriggered
-> 统一 TryActivateAbility
-> Released 清除 InputPressed、向已激活能力发事件
-> 清空本帧 Pressed/Released
```

## P1 - SkillDefinition 资产选择器使用了错误的 RowStructure 过滤

位置：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h:24
```

`SkillDefinition` 已经是强类型的 `TObjectPtr<UApexSkillDefinition>`，不需要额外类过滤。当前：

```cpp
meta = (RequiredAssetDataTags = "RowStructure=UApexSkillDefinition")
```

`RowStructure` 是 DataTable 行结构资产标签；`UApexSkillDefinition` 不是 DataTable Row。该条件可能把正常 SkillDefinition 资产从选择器中全部过滤掉，直接阻断人工填写 AbilitySet。

修复：删除 `RequiredAssetDataTags`。类型本身已经提供正确过滤。

## P2 - 输入事件使用了已弃用且不适用于实例化 GA 的 ActivationInfo

位置：

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:105
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:112
```

`Spec.ActivationInfo` 从 UE 5.5 起只适用于已弃用的 NonInstanced Ability。Apex 根 GA 是 `InstancedPerActor`，预测键应优先来自：

```cpp
Spec.GetPrimaryInstance()->GetCurrentActivationInfo().GetActivationPredictionKey()
```

可按 UE 5.8 Lyra 的兼容写法保留受 pragma 包围的 legacy fallback，但修复后不应再产生 C4996 警告。

## P2 - InputTag 根域约束只写在设计中，没有落实

位置：

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp:94
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp:128
Source/Apex/Private/Input/ApexAbilityInputConfig.cpp:23
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:12
```

已批准规则是：有效的技能输入 Tag 必须是 `InputTag.Ability` 的具体子 Tag，根 Tag 本身不能充当输入槽。

当前实现只检查 `IsValid()` 和重复项，因此任意 GameplayTag 都可能进入 AbilitySet、InputConfig 和 ASC 路由。

修复要求：

- AbilitySet 运行时授予与 Data Validation 都检查根域。
- AbilityInputConfig Data Validation 检查根域。
- ASC 的公开 Pressed/Released 入口拒绝根域外 Tag。
- `FApexAbilitySetAbilityGrant::InputTag` 增加 `meta = (Categories = "InputTag.Ability")`，改善编辑器填写体验。

## P2 - 输入队列可能跨 Avatar 残留

位置：

```text
Source/Apex/Private/Character/ApexPlayerCharacter.cpp:96
```

ASC 挂在 PlayerState 上，会跨 Character/Avatar 存活；输入队列也因此不会随旧 Character 自动销毁。当前撤销能力只 ClearAbility Handle，没有调用 `ClearAbilityInput()`。

如果 UnPossessed 时按键仍处于 Held，旧 Handle 会长期残留在 PlayerState ASC，污染重生或换英雄后的输入状态。

修复：撤销 CoreAbilitySet 时，在客户端和服务端都清空 ASC 输入队列，再由权威端清除 Ability Handle。

## P2 - 权威检查存在不必要的空指针风险

位置：

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp:25
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp:59
```

当前直接调用：

```cpp
ASC->GetOwnerActor()->HasAuthority()
```

基础 API 不应假设 ActorInfo 一定有效。UE 5.8 已提供：

```cpp
ASC->IsOwnerActorAuthoritative()
```

应使用该接口完成空安全、语义明确的权威检查。

## P3 - 幂等注释描述不准确

位置：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h:66
```

`GiveToAbilitySystem()` 本身多次调用会重复授予；真正的幂等性来自调用方先使用旧 Handles 撤销，再重新授予。应修改注释，避免学习文档误导。

## 已确认正确

- `AbilitySet -> SkillDefinition -> AbilityTemplateClass` 分层正确。
- SkillDefinition 作为 AbilitySpec SourceObject 的方向正确。
- InputTag 写入 Dynamic Spec Source Tags 正确。
- AbilitySet 只保存配置，运行时 Handle 独立保存，边界正确。
- GA 根类的 InstancedPerActor、LocalPredicted、ReplicateNo 默认值符合当前玩家技能基线。
- Character 现有 Jump、Move、Look、MouseLook 绑定均保留。
- PlayerController 从 PlayerState 获取 ASC 并在 PostProcessInput 推进队列，职责归属正确。
- Ability InputAction 使用 Started / Completed / Canceled 的绑定方式正确。

## 修复后验收

1. 仅修改修复 Prompt 允许的源码和原实施报告。
2. 重新编译 `ApexEditor Win64 Development`，要求 0 error，且本批不再产生 ActivationInfo C4996。
3. Codex 复审输入处理顺序、Tag 约束、资产选择器和 Avatar 清理。
4. 复审通过后，用户再刷新 Rider 项目文件并执行 UE 人工配置。

## 第一次修复复审

复审日期：2026-07-23  
状态：仍未通过，只剩一项运行时问题和两项文档清理。

### 已通过

- InputTag 回调已改为只维护 Pressed/Held/Released 队列，不再在 ActivatableAbilities 遍历中激活能力。
- OnInputTriggered 已移动到 `ProcessAbilityInput()` 的集中激活阶段。
- Released 已正确设置 `Spec.InputPressed = false` 并调用 `AbilitySpecInputReleased()`。
- Generic Replicated Event 的 PredictionKey 已优先取自 PrimaryInstance CurrentActivationInfo。
- 错误的 `RequiredAssetDataTags/RowStructure` 已删除。
- AbilitySet、InputConfig 和 ASC 均已落实 `InputTag.Ability` 具体子 Tag 校验。
- Avatar 撤销能力前已调用 `ClearAbilityInput()`。
- 权威检查已改为 `IsOwnerActorAuthoritative()`。
- 子代理报告记录修复版本编译为 0 error、0 warning。

### P1 - WhileInputActive 的 Pressed 状态仍被跳过

位置：

```text
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp:92
```

Held 阶段会先把 `WhileInputActive` 的 Handle 加入 `AbilitiesToActivate`。Pressed 阶段开头当前存在：

```cpp
if (AbilitiesToActivate.Contains(Handle))
{
	continue;
}
```

因此同一 Handle 不会执行后面的：

```cpp
Spec->InputPressed = true;
```

这会导致 WhileInputActive 能力被激活时仍认为输入未按下，`WaitInputRelease` 可能把它判断为已经释放。Lyra 的正确实现不会跳过 Pressed 状态处理；只在加入待激活数组时使用 `AddUnique()` 去重。

修复：删除 Pressed 循环开头的 Contains/continue。保留后面 OnInputTriggered 分支中的 `AddUnique()`。

### P3 - 注释拼写

位置：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySet.h:23
```

`TOjectPtr` 应改为 `TObjectPtr`。

### P3 - 实施报告前半部分仍描述旧逻辑

报告追加的修复章节是正确的，但原有成员表、运行链路、Data Validation 和首次编译结果仍写着“InputTagPressed 立即激活”“Process 只处理 WhileInputActive”“2 个 C4996”等旧状态。

为避免上下文恢复和学习时误读，应直接把原有章节同步为最终实现，保留修复历史但不保留相互矛盾的现状描述。

## 最终复审结果

复审日期：2026-07-23  
状态：代码审查通过，等待 UE 单人/多人运行验证。

- Pressed 循环已删除 `AbilitiesToActivate.Contains()` 提前跳过逻辑。
- `WhileInputActive` 与 `OnInputTriggered` 都会正确设置 `Spec.InputPressed = true`。
- Held、Pressed、统一激活、Released 的处理顺序正确。
- Generic Replicated Event 使用当前 GA PrimaryInstance 的 Activation PredictionKey。
- SkillDefinition 资产选择器、InputTag 根域、Avatar 输入清理和权威检查均正确。
- `TObjectPtr` 注释拼写已修正。
- 实施报告现状章节已由 Codex 统一为最终逻辑；修复章节保留历史记录。
- `ApexAbilitySystemComponent.cpp` 修改时间为 2026-07-23 14:14:31。
- `UnrealEditor-Apex.dll` 链接时间为 2026-07-23 14:16:23，晚于源码修改。
- 子代理最终编译结果：0 error、0 warning。

当前没有遗留代码审查问题，可以刷新 Rider 项目文件并进入 UE 人工配置和 PIE 验证。

## UE 运行验证结果

验证日期：2026-07-23  
状态：通过。

- 项目自有 InputAction、InputConfig、SkillDefinition、AbilitySet 与测试 Ability 均可正常配置和加载。
- AbilitySet 能在权威端正确授予 AbilitySpec。
- `InputAction -> InputTag -> AbilitySpec` 路由正确。
- Pressed、Held、Released 输入语义均符合预期。
- 单人 PIE 验证通过。
- 多人 PIE 验证通过。

本批没有遗留运行问题，可以进入第一个真实 GA 模板的需求设计。
