# SkillArchitecture Foundation Phase 1 Codex 审查修复报告

*创建日期：2026-07-09*
*修复者：Claude*

## 修复概要

修复了 Codex 对 Phase 1 基础架构的 5 个审查问题，涉及 3 个文件。

| 文件 | 修改范围 |
|------|----------|
| `Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate.h` | 新增 `FAuraSkillActivationContext` 结构体；所有虚函数签名改用 Context 参数；`ActivateTemplate` 改为非纯虚（有默认实现） |
| `Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate.cpp` | 基类 `ActivateTemplate` 默认调用 `Ability->FinishFromTemplate(false)` |
| `Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h` | 新增 `FinishFromTemplate` / `CancelFromTemplate` public 方法；新增 `GetSkillDef`、`bPolicyApplied`、`ApplyActivationPolicy`、`RemoveActivationPolicy` |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp` | 完整重写：Super 调用、Policy 执行、模板 Context 传递、FinishFromTemplate/CancelFromTemplate |

## 逐项修复

### 1. `CanActivateAbility` 先调用 `Super`

```cpp
if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
{
    return false;
}
```

父类 `UAuraGameplayAbility` 的 `CanActivateAbility` 检查 GAS 通用条件（成本/冷却/阻塞 Tag）。父类失败后直接返回，不再继续自定义校验。

### 2. 空壳模板导致 Ability 永久 Active

**根因**：Phase 1 模板 `ActivateTemplate` 是空的虚函数，调用后不结束 Ability → 技能永远处于 Active 状态。

**修复**：
- `UAuraSkillTemplate::ActivateTemplate` 改为非纯虚，基类实现调用 `Ability->FinishFromTemplate(false)` 直接结束 Ability。
- 子类覆盖后应自行管理流程结束时调用 `FinishFromTemplate` 或 `CancelFromTemplate`。
- 新增两个 public 方法：
  - `FinishFromTemplate(bool bWasCancelled)` → 调用 `EndAbility`
  - `CancelFromTemplate()` → 调用 `CancelAbility`
- 模板不再需要直接调用 `protected EndAbility`。

### 3. Policy 字段真正生效

**`CanActivateAbility` 阶段**：
- `RequiredOwnerTags`：ASC 必须拥有所有 Tag，否则拒绝
- `BlockedOwnerTags`：ASC 拥有任何则拒绝

**`ActivateAbility` 阶段**（Commit 后、Template 前）：
- `ActivationOwnedTags` → `ASC->AddLooseGameplayTags()`
- `CancelAbilitiesWithTags` → `ASC->CancelAbilities()`
- `BlockAbilitiesWithTags` → `ASC->BlockAbilitiesWithTags()`
- 设置 `bPolicyApplied = true`

**`EndAbility` 阶段**：
- 仅在 `bPolicyApplied == true` 时清理
- `ActivationOwnedTags` → `ASC->RemoveLooseGameplayTags()`
- `BlockAbilitiesWithTags` → `ASC->UnBlockAbilitiesWithTags()`
- 避免了未成功激活的技能反向清理 Tag

### 4. `CanActivateTemplate` 签名调整

旧签名：
```cpp
virtual bool CanActivateTemplate(const UAuraSkillRuntimeAbility* Ability) const;
virtual void ActivateTemplate(UAuraSkillRuntimeAbility* Ability);
```

新签名：
```cpp
virtual bool CanActivateTemplate(const UAuraSkillRuntimeAbility* Ability, const FAuraSkillActivationContext& Context) const;
virtual void ActivateTemplate(UAuraSkillRuntimeAbility* Ability, const FAuraSkillActivationContext& Context);
```

新增 `FAuraSkillActivationContext` 结构体，包含：
- `Handle`（`FGameplayAbilitySpecHandle`）
- `ActorInfo`（`const FGameplayAbilityActorInfo*`）
- `SkillDef`（`const UAuraSkillDefinition*`）
- `SourceTags` / `TargetTags` / `OptionalRelevantTags`

模板可以通过 Context 访问所有激活参数，不再依赖 GA 内部状态。

### 5. `Config/DefaultEditor.ini`

`Config/DefaultEditor.ini` 有 4 行修改（之前 MCP 操作的 Editor 配置变动），不属于本轮 Phase 1 架构代码提交内容。建议在提交前单独审查或排除。

## 编译结果

```
Result: Succeeded
Total execution time: 69.77 seconds
Exit code: 0
```

- 0 错误
- 仅旧代码 C4996 warnings

## 未修改

- 模板子类（ProjectileCast / ChannelCast）签名未显式改（基类虚函数签名已更新，子类目前使用默认实现，Phase 2/3 覆盖时再适配新签名）
- `CombatEntityDefinition` — 不涉及
- `AuraSkillDefinition.h` — 不涉及
- ASC 授予路径 — 不涉及
