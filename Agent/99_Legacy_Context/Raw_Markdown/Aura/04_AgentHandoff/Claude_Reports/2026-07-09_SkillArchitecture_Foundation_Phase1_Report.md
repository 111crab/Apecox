# 技能系统基础架构 Phase 1 实施报告

*创建日期：2026-07-09*
*实施者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-09_SkillArchitecture_Foundation_Phase1.md*
*参考架构：Agent/02_SkillSystem/2026-07-09_SkillSystem_Architecture_Refactor_Summary.md*

## 1. 简要总结

在 Aura 项目中搭建了新的技能系统基础架构 Phase 1（可编译最小壳）。新架构将"万能配置表 + 巨型 GA"升级为分层设计：

```
UAuraSkillDefinition（技能组装入口）
  → UAuraSkillRuntimeAbility（GAS 生命周期入口）
  → UAuraSkillTemplate（内联模板，子类描述流程）
  → UAuraCombatEntityDefinition（战斗衍生物配置）
```

旧原型代码（`UAuraConfiguredActiveAbility` 及配套字段/枚举）已删除。

## 2. 文件变更清单

### 新增文件（10 个）

| 文件 | 职责 |
|------|------|
| `Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h` | 新通用主动技能 GA（替换旧 ConfiguredActiveAbility） |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp` | GA 实现：CanActivate/Activate/Cancel/End，委托给模板 |
| `Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate.h` | 技能模板抽象基类（UObject, EditInlineNew, DefaultToInstanced） |
| `Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate.cpp` | 基类实现 |
| `Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate_ProjectileCast.h` | 投射物施法模板（最小壳） |
| `Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate_ProjectileCast.cpp` | 壳实现 |
| `Source/Aura/Public/AbilitySystem/SkillTemplates/AuraSkillTemplate_ChannelCast.h` | 引导施法模板（最小壳） |
| `Source/Aura/Private/AbilitySystem/SkillTemplates/AuraSkillTemplate_ChannelCast.cpp` | 壳实现 |
| `Source/Aura/Public/AbilitySystem/CombatEntities/AuraCombatEntityDefinition.h` | 战斗衍生物配置（UDataAsset, Projectile/Field） |
| `Source/Aura/Private/AbilitySystem/CombatEntities/AuraCombatEntityDefinition.cpp` | 壳实现 |

### 修改文件（6 个）

| 文件 | 变更 |
|------|------|
| `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h` | 完整重写：移除旧字段/枚举/结构体，新增 Identity/Policy/SkillTemplate |
| `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp` | 简化实现 |
| `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h` | 更新注释，`CancelActiveChannelingAbilities` 改为模板检测 |
| `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp` | 完整重写：移除 `UAuraConfiguredActiveAbility` include，改用 `UAuraSkillRuntimeAbility`；`CancelActiveChannelingAbilities` 用 `UAuraSkillTemplate_ChannelCast` 检测 |
| `Source/Aura/Private/Character/AuraCharacterBase.cpp` | 注释更新 |
| `Config/DefaultGameplayTags.ini` | 新增 5 个 Tag |

### 删除文件（2 个）

| 文件 | 删除理由 |
|------|----------|
| `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h` | 被 `UAuraSkillRuntimeAbility` 替代。旧类的职责（GAS 入口 + 巨量分支逻辑）不符合分层设计，且与新模板调度模型冲突 |
| `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp` | 同上 |

### 未修改

- `Aura.Build.cs` — 新目录自动在模块 Public/Private 路径下，无需额外依赖
- `AAuraCharacterBase.h` — `StartupSkillDefinitions` 字段保持 `TArray<TObjectPtr<UAuraSkillDefinition>>`，兼容
- `AAuraPlayerController::Move` — 移动取消逻辑仍调用 `CancelActiveChannelingAbilities`，函数签名未变
- 旧教程路径（`GA_FireBolt`、`UAuraProjectileSpell`、`AAuraProjectile`）— 完全保留

## 3. 删除内容的理由和引用检查

### 删除的类

**`UAuraConfiguredActiveAbility`**：职责过于集中（GAS 入口 + Projectile 生成 + Montage 管理 + ChannelCast 流程 + EffectEntry 应用），违反单一职责原则。新设计中 GA 只做生命周期管理，流程委托给 `UAuraSkillTemplate`。

引用检查（删除前）：
- `AuraAbilitySystemComponent.cpp` include → 已替换为 `AuraSkillRuntimeAbility.h`
- `AuraCharacterBase.cpp` 注释 → 已更新
- `AuraSkillRuntimeAbility.h` 文档注释（仅提及作为对比）→ 保留
- 无其他引用

### 删除的枚举和结构体

| 删除内容 | 所属文件 | 删除理由 | 引用检查 |
|----------|----------|----------|----------|
| `EAuraSkillTargetingMode` | AuraSkillDefinition.h | 旧原型目标模式。新模板 `EAuraSkillTargetRule` 取代 | 仅被 `UAuraConfiguredActiveAbility` 使用 |
| `EAuraSkillDeliveryMode` | AuraSkillDefinition.h | 旧原型送达模式。模板类型本身决定了送达方式 | 仅被 `UAuraConfiguredActiveAbility` 使用 |
| `EAuraSkillInputMode` | AuraSkillDefinition.h | 旧原型输入模式。模板类型本身决定了输入模式 | 被 `UAuraConfiguredActiveAbility` 和 `CancelActiveChannelingAbilities` 使用。后者已改为模板检测 |
| `EAuraSkillEffectTarget` | AuraSkillDefinition.h | V1-B 原型效果目标。后续 Effect 系统重构时重新设计 | 仅被 `UAuraConfiguredActiveAbility` 使用 |
| `EAuraSkillEffectCondition` | AuraSkillDefinition.h | V1-B 原型效果条件。ChannelCast 模板用 Outcome Tag 替代 | 仅被 `UAuraConfiguredActiveAbility` 使用 |
| `FAuraSkillEffectEntry` | AuraSkillDefinition.h | V1-B 原型效果条目。后续 Effect 系统重构 | 仅被 `UAuraSkillDefinition::EffectEntries` 和 `UAuraConfiguredActiveAbility` 使用 |

### 删除的 UAuraSkillDefinition 字段

所有旧 V1-A/B 原型字段已删除：`TargetingMode`, `DeliveryMode`, `ProjectileClass`, `DamageEffectClass`, `DamageTypes`, `InputMode`, `FullChargeDuration`, `PartialChargeScale`, `bCancelOnMoveInputDuringChannel`, `EffectEntries`, `CastMontage`, `MontageSpawnEventTag`。

这些字段的职责迁移到：
- 投射物相关 → `UAuraSkillTemplate_ProjectileCast`
- 引导相关 → `UAuraSkillTemplate_ChannelCast`
- 效果相关 → 后续 Effect 系统（Phase 5）
- 策略相关 → `UAuraSkillDefinition` 的 Policy 字段（`RequiredOwnerTags`, `BlockedOwnerTags` 等）

**影响**：旧 `DA_Fireball` 和 `DA_VitalSurge` 资产中保存了已删除枚举的整数值和已删除字段的序列化数据。UE 在打开这些资产时可能报字段缺失警告。建议在 UE 编辑器中删除并重新创建这些资产（Phase 2/3 将用新架构重新设计）。

## 4. 新增类职责说明

### `UAuraSkillRuntimeAbility`

新的通用主动技能运行时 GA。

- **父类**：`UAuraGameplayAbility` → `UGameplayAbility`
- **InstancingPolicy**：`InstancedPerExecution`（每次激活新实例，不同技能用同一 GA 类时互不干扰）
- **职责**：GAS 生命周期入口，不包含任何具体技能逻辑。将 `ActivateAbility` 委托给 `CachedSkillDef->SkillTemplate->ActivateTemplate(this)`。
- **与旧类的区别**：旧 `UAuraConfiguredActiveAbility` 包含 ~600 行混合逻辑（Montage、Projectile、ChannelCast、EffectEntry 全部在一起）。新 GA 仅 ~80 行，做生命周期 + 模板调度。

### `UAuraSkillTemplate`

抽象模板基类。

- **父类**：`UObject`（非 `USTRUCT`，因为需要虚函数、多态细节面板、`EditInlineNew`）
- **`EditInlineNew`**：允许在父资产中直接创建内联子对象
- **`DefaultToInstanced`**：创建父资产时自动实例化默认模板
- **虚函数**：`CanActivateTemplate`, `ActivateTemplate`, `CancelTemplate`, `EndTemplate`

### `UAuraSkillTemplate_ProjectileCast`

投射物施法模板。字段：`CastMontage`（软引用）、`FireEventTag`、`ProjectileEntity`、`TargetRule`（新 `EAuraSkillTargetRule` 枚举）。Phase 1 仅壳，Phase 2 实现火球术重做。

### `UAuraSkillTemplate_ChannelCast`

引导/蓄力模板。字段：`FullDuration`、`bCancelOnMoveInput`、`FullOutcomeTag`、`PartialOutcomeTag`、`CancelledOutcomeTag`。通用模板（非治疗专用），Phase 3 实现生命涌动重做。

### `UAuraCombatEntityDefinition`

战斗衍生物配置。`UDataAsset`（非 `UPrimaryDataAsset`，因为常用 TOjectPtr 内联引用，暂不需要 AssetManager 扫描）。第一版仅 `Projectile` 和 `Field` 类型。预留 Detection / OnHit / OnTick / OnEnd 字段。

## 5. `UAuraSkillRuntimeAbility` 与 `UAuraSkillTemplate` 的关系

```text
InputTag → ASC 激活 AbilitySpec
  → UAuraSkillRuntimeAbility::ActivateAbility
    → CommitAbility（GAS 成本/冷却）
    → CachedSkillTemplate->ActivateTemplate(this)
      → 子类覆盖（ProjectileCast / ChannelCast / ...）
        → 创建 AbilityTask
        → 服务端生成衍生物/应用 GE
        → EndAbility
```

- **GA** 拥有 GAS 生命周期（CanActivate / Commit / Cancel / End），负责失败兜底
- **Template** 拥有流程逻辑（何时播放 Montage、何时等待事件、何时生成投射物），但不直接调用 GAS API——通过 GA 的 public 方法间接控制
- **GA** 的 `EndAbility` 调用 `Template->EndTemplate` 做模板专属清理

## 6. `UAuraSkillDefinition` 新结构说明

| 分组 | 字段 | 用途 |
|------|------|------|
| Identity | `SkillTag`, `DisplayName`, `Description`, `Icon` | 技能身份和 UI |
| Grant/Input | `RuntimeAbilityClass`, `InputTag`, `DefaultLevel` | 授予和输入绑定 |
| Policy | `RequiredOwnerTags`, `BlockedOwnerTags`, `ActivationOwnedTags`, `CancelAbilitiesWithTags`, `BlockAbilitiesWithTags` | 技能策略限制 |
| Skill | `SkillTemplate`（Instanced） | 内联模板实例 |

## 7. CombatEntityDefinition 第一版边界

- ✅ `EAuraCombatEntityType`：Projectile / Field
- ✅ `ActorClass`（软引用）：衍生物 Actor 类
- ✅ `Lifetime`：生命周期
- ❌ 未实现：Detection shape、Team filter、OnHit/OnTick/OnEnd effect

## 8. Effect / State 第一版边界

- State：`State.Skill.Channeling`（已在 native tag）
- Outcome：`Outcome.Channel.Full` / `Partial` / `Cancelled`（ini tag，供 ChannelCast 模板配置用）
- Effect：第一版未引入结构化 Effect 层，待 Phase 5
- GE：继续直接引用（过渡方案）

## 9. 编译结果

```
Result: Succeeded
Total execution time: 54.76 seconds
Exit code: 0
```

- 0 错误
- 仅旧代码 C4996 warnings（`DynamicAbilityTags`、`ActivationInfo`、`NetUpdateFrequency`），不在本次修改范围

## 10. 尚未实现的内容

| 内容 | 计划阶段 |
|------|----------|
| ProjectileCast 完整激活逻辑（Montage 播放、WaitGameplayEvent、生成投射物） | Phase 2 |
| ChannelCast 完整激活逻辑（WaitInputRelease、Outcome 分支、Effect 应用） | Phase 3 |
| CombatEntity 生成/检测/命中/周期/结束逻辑 | Phase 4 |
| Effect 结构化层 | Phase 5 |
| BuffDefinition | 后置 |
| Skill Timeline | 后置 |
| 技能编辑器 | 后置 |
| 新火球术 DA（DA_Skill_Fireball） | Phase 2 |
| 新生命涌动 DA（DA_Skill_VitalSurge） | Phase 3 |

## 11. 下一步建议

**Phase 2：用新架构重新设计火球术**

1. 实现 `UAuraSkillTemplate_ProjectileCast::ActivateTemplate` 完整逻辑。
2. 创建新的 `DA_Skill_Fireball` 资产，使用 ProjectileCast 模板。
3. 创建 `DA_Entity_FireballProjectile` 战斗衍生物资产。
4. 删除或标记旧 `DA_Fireball` 为 deprecated。
5. 验证：单人 PIE 火球术在新架构下表现与 V1-A 一致。
