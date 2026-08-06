# 技能系统 Phase 1.1 Policy 收尾修复报告

*创建日期：2026-07-09*
*修复者：Claude*

## 修复概要

将 `UAuraSkillRuntimeAbility` 中 `CancelAbilitiesWithTags` 和 `BlockAbilitiesWithTags` 的实现从 GAS 原生 API（基于 `UGameplayAbility` 类 `AssetTags`）改为自定义实现（基于 `UAuraSkillDefinition.SkillTag`）。

## 修改文件

| 文件 | 变更 |
|------|------|
| `Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h` | +2 个 private helper 声明：`IsBlockedByActiveConfiguredSkill`、`CancelActiveConfiguredSkillsBySkillTags` |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp` | 重写 `ApplyActivationPolicy` / `RemoveActivationPolicy`；`CanActivateAbility` 新增阻塞检查；+2 个 helper 实现 |

## 删除/替换的 GAS 原生 Policy 调用

| 旧调用 | 新调用 | 原因 |
|--------|--------|------|
| `ASC->CancelAbilities(&Tags)` | `CancelActiveConfiguredSkillsBySkillTags(ASC, Tags, Handle)` | GAS 原生取消匹配 GA 类 AssetTags；配置化技能共享同一 GA 类，无法按火球术/生命涌动区分 |
| `ASC->BlockAbilitiesWithTags(Tags)` | `IsBlockedByActiveConfiguredSkill(ASC, SkillDef, Handle)`（在 `CanActivateAbility` 中调用） | 同上 |
| `ASC->UnBlockAbilitiesWithTags(Tags)` | 已移除 | 不在 ASC 层注册阻塞，因此也无需解除 |

## 取消技能的匹配口径

```
当前技能激活时
  → CancelActiveConfiguredSkillsBySkillTags(ASC, CancelAbilitiesWithTags, ThisHandle)
  → 遍历 ASC->GetActivatableAbilities()
  → 跳过 ThisHandle（不自取消）
  → 只处理 IsActive() 的技能
  → Cast<UAuraSkillDefinition>(SourceObject)
  → 使用 HasTag（父子匹配）检查：
      CancelAbilitiesWithTags.HasTag(OtherSkillDef->SkillTag)
  → 命中则 ASC->CancelAbilityHandle(OtherSpec.Handle)
```

示例：`Ability.Fire` 作为 `CancelAbilitiesWithTags` 可以取消 `Ability.Fire.Fireball`（父子匹配）。

## 阻塞技能的匹配口径

```
当前技能尝试激活时
  → IsBlockedByActiveConfiguredSkill(ASC, ThisSkillDef, ThisHandle)
  → 遍历 ASC->GetActivatableAbilities()
  → 跳过 ThisHandle
  → 只处理 IsActive() 的技能
  → Cast<UAuraSkillDefinition>(SourceObject)
  → 使用 HasTag 检查：
      OtherSkillDef->BlockAbilitiesWithTags.HasTag(ThisSkillDef->SkillTag)
  → 命中则返回 true → CanActivateAbility 返回 false
```

语义：其他活跃技能声明"我阻止 Ability.Fire.Fireball" → 当前准备激活的火球术被拒绝。

## 跳过当前 SpecHandle

两个 helper 都通过 `Handle == ThisHandle` 跳过当前技能自身：
- 取消时不自取消。
- 阻塞检查时不自阻塞。

## 编译结果

```
Result: Succeeded
Total execution time: 35.93 seconds
Exit code: 0
```

- 0 错误
- 仅旧代码 C4996 warnings

## 保留的正确逻辑

以下全部保留且未修改：
- `Super::CanActivateAbility()`
- `RequiredOwnerTags` / `BlockedOwnerTags`
- `ActivationOwnedTags`（`AddLooseGameplayTags` / `RemoveLooseGameplayTags`）
- 空壳模板默认 `FinishFromTemplate(false)`
- `FinishFromTemplate` / `CancelFromTemplate`
- `FAuraSkillActivationContext`

## 剩余风险

1. `CancelAbilityHandle` 直接取消而不检查是否可安全取消（V1 可接受，后续可增加 `CanBeCanceled` 检查）。
2. 父子 Tag 匹配 (`HasTag`) 语义可能过于宽松：`Ability.Fire` 作为 `CancelAbilitiesWithTags` 会取消所有子 Tag 技能。这是设计意图，但需要在文档中说明。
3. 阻塞检查发生在 `CanActivateAbility` 中，阻塞方结束 (`EndAbility`) 后阻塞自动解除——这是正确的设计意图。
