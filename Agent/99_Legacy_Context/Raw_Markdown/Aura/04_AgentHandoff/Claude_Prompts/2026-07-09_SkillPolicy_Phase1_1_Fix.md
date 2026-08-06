# ClaudeCode 实施 Prompt：技能系统 Phase 1.1 Policy 收尾修复

创建日期：2026-07-09  
状态：交给 ClaudeCode 执行  
负责人：ClaudeCode 执行，Codex 审查，用户最终验证

## 任务目标

本轮不是实现火球术，不创建新技能资产，不使用 MCP 改 UE 编辑器资产。

本轮只修复 `UAuraSkillRuntimeAbility` 中配置化技能 Policy 的一个架构问题：  
`CancelAbilitiesWithTags` / `BlockAbilitiesWithTags` 不能依赖 GAS 原生 `Ability AssetTags`，因为我们的新架构是：

```text
一个通用 UAuraSkillRuntimeAbility
  + 每个 AbilitySpec.SourceObject 指向不同 UAuraSkillDefinition
  + 每个 UAuraSkillDefinition.SkillTag 表示具体技能身份
```

GAS 原生 `ASC->CancelAbilities()` 和 `ASC->BlockAbilitiesWithTags()` 匹配的是 `UGameplayAbility` 类自身的 `AssetTags`。如果继续使用它们，多个配置化技能共享同一个 GA 类时，无法按火球术、生命涌动等具体技能区分取消/阻塞关系。

## 必读文件

请先阅读：

- `.agents/ue-project-context.md`
- `Agent/02_SkillSystem/2026-07-09_SkillSystem_Architecture_Refactor_Summary.md`
- `Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillArchitecture_Foundation_Phase1_Report.md`
- `Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillArchitecture_Foundation_Phase1_Fix_Report.md`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp`
- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`

建议使用的知识：

- GAS `UGameplayAbility::CanActivateAbility`
- `FGameplayAbilitySpec`
- `AbilitySpec.SourceObject`
- `UAbilitySystemComponent::GetActivatableAbilities`
- `UAbilitySystemComponent::CancelAbilitySpec` 或等价安全 API

## 必须修复的问题

### 1. 取消技能不要使用 GAS 原生 Ability AssetTags

当前问题代码大致类似：

```cpp
ASC->CancelAbilities(&CachedSkillDef->CancelAbilitiesWithTags);
```

请替换为 Aura 自己的配置化技能取消逻辑。

要求：

1. 遍历 `ASC->GetActivatableAbilities()`。
2. 只处理 `AbilitySpec.IsActive()` 的技能。
3. 读取 `AbilitySpec.SourceObject`，只处理能 cast 成 `UAuraSkillDefinition` 的配置化技能。
4. 跳过当前技能自己的 `SpecHandle`，避免自取消。
5. 使用 `OtherSkillDef->SkillTag` 与 `CachedSkillDef->CancelAbilitiesWithTags` 匹配。
6. 匹配时取消对应 `AbilitySpec`。

匹配口径：

```text
当前技能的 CancelAbilitiesWithTags
  命中
其他活跃技能的 OtherSkillDef->SkillTag
```

建议使用父子 Tag 匹配，而不是只做 exact 匹配：

```cpp
CancelAbilitiesWithTags.HasTag(OtherSkillDef->SkillTag)
```

如果你认为 UE API 中此方向不对，请在报告中明确说明并给出实际使用的匹配函数。

### 2. 阻塞技能不要使用 GAS 原生 BlockAbilitiesWithTags

当前问题代码大致类似：

```cpp
ASC->BlockAbilitiesWithTags(CachedSkillDef->BlockAbilitiesWithTags);
ASC->UnBlockAbilitiesWithTags(CachedSkillDef->BlockAbilitiesWithTags);
```

请移除这条路径。不要把配置化技能关系写进 GAS ASC 的 `BlockedAbilityTags`，因为那仍然是围绕 GA `AssetTags` 工作的。

改为在 `UAuraSkillRuntimeAbility::CanActivateAbility` 的自定义检查阶段执行：

1. 当前技能有 `SkillDef->SkillTag`。
2. 遍历当前 ASC 所有活跃配置化技能。
3. 跳过当前 `SpecHandle`。
4. 读取其他活跃技能 `OtherSkillDef->BlockAbilitiesWithTags`。
5. 如果其他活跃技能的 `BlockAbilitiesWithTags` 命中当前 `SkillDef->SkillTag`，则当前技能不能激活。

匹配口径：

```text
其他活跃技能的 BlockAbilitiesWithTags
  命中
当前准备激活技能的 SkillDef->SkillTag
```

这样语义就是：

```text
技能 A 激活期间声明：我阻止 Ability.Fire.Fireball
技能 B 尝试激活，B.SkillTag = Ability.Fire.Fireball
=> B 被拒绝激活
```

### 3. 保留当前已经正确的 Policy

以下逻辑不要移除：

- `Super::CanActivateAbility(...)`
- `RequiredOwnerTags`
- `BlockedOwnerTags`
- `ActivationOwnedTags`
- 空壳模板默认 `FinishFromTemplate(false)`
- `FinishFromTemplate` / `CancelFromTemplate`
- `FAuraSkillActivationContext`

可以调整函数拆分和注释，但不要退回到旧 `UAuraConfiguredActiveAbility` 路径。

## 推荐代码组织

可以选择以下任意一种方式，优先保持清晰：

### 方案 A：放在 `UAuraSkillRuntimeAbility`

新增 private helper：

```cpp
bool IsBlockedByActiveConfiguredSkill(
    UAbilitySystemComponent* ASC,
    const UAuraSkillDefinition* ThisSkillDef,
    FGameplayAbilitySpecHandle ThisHandle) const;

void CancelActiveConfiguredSkillsBySkillTags(
    UAbilitySystemComponent* ASC,
    const FGameplayTagContainer& TagsToCancel,
    FGameplayAbilitySpecHandle ThisHandle);
```

### 方案 B：放在 `UAuraAbilitySystemComponent`

如果你认为 ASC 更适合拥有遍历 `FGameplayAbilitySpec` 的逻辑，也可以在 `UAuraAbilitySystemComponent` 中新增 helper。

要求：

- `UAuraSkillRuntimeAbility` 调用 helper。
- helper 名称必须明确表达“配置化技能 / SkillDefinition / SkillTag”。
- 不要命名成泛泛的 `CancelAbilitiesByTags`，避免和 GAS 原生 API 混淆。

## 注释要求

请写中文注释解释这个设计原因：

1. 为什么不能直接用 GAS `CancelAbilitiesWithTag` / `BlockAbilitiesWithTag`。
2. 为什么配置化技能关系以 `UAuraSkillDefinition.SkillTag` 为匹配对象。
3. 为什么取消/阻塞时要跳过当前 `SpecHandle`。

注释不要太长；复杂解释写在报告中。

## 不要做

- 不要实现火球术完整发射流程。
- 不要创建或修改 `.uasset` 技能资产。
- 不要使用 MCP 改编辑器资产。
- 不要重构属性、伤害公式、GE、GameplayCue。
- 不要提交或处理 `Config/DefaultEditor.ini` 的预览场景配置变动。
- 不要删除旧教程 `GA_FireBolt` / `UAuraProjectileSpell` / `AAuraProjectile`。

## 验证要求

必须执行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat AuraEditor Win64 Development -Project="D:\UnrealProject\Aura\Aura.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

如果命令行只输出 UBT 启动行，请读取：

```text
C:\Users\Lenovo\AppData\Local\UnrealBuildTool\Log.txt
```

并在报告里写明真实结果。

## 报告要求

完成后写中文报告：

```text
Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillPolicy_Phase1_1_Fix_Report.md
```

报告至少包含：

1. 修改了哪些文件。
2. 删除/替换了哪些 GAS 原生 Policy 调用。
3. 现在取消技能的匹配口径。
4. 现在阻塞技能的匹配口径。
5. 是否跳过当前 SpecHandle。
6. 编译结果。
7. 剩余风险或下一步建议。

## 完成标准

- 编译通过。
- `CancelAbilitiesWithTags` 不再调用 `ASC->CancelAbilities(&Tags)` 这种基于 Ability AssetTags 的路径。
- `BlockAbilitiesWithTags` 不再调用 `ASC->BlockAbilitiesWithTags / UnBlockAbilitiesWithTags`。
- 配置化技能取消/阻塞以 `UAuraSkillDefinition.SkillTag` 为核心。
- 空壳模板仍能安全结束 Ability。

