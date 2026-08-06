# ClaudeCode 实施 Prompt：战斗基础 Phase B - 稳定 Tag 与简单伤害表现

创建日期：2026-07-13  
状态：用户已确认 Combat Foundation 阶段 A，可交给 ClaudeCode 执行  
负责人：ClaudeCode 执行，Codex 审查，用户最终验证

## 任务目标

本轮任务不是实现完整火球术，不创建 `.uasset` 技能资产，不使用 MCP 操作 UE 编辑器。

本轮目标是把 Aura 当前技能系统的基础战斗口径整理到可继续开发的状态：

```text
稳定 GameplayTag 语义
  + 简单伤害/治疗表现
  + 不提前实现复杂属性公式
  + 不把临时实现细节扩张成长期 Tag
```

用户已明确要求：

- Tag 应该是稳定、长期的语义。
- 临时实现细节优先使用 enum、结构体字段、配置字段或普通变量。
- `SetByCaller.*` 很容易让人误解，不能当成玩法 Tag 体系扩张。
- 当前项目暂不做复杂属性计算，先继续技能实现和简单伤害表现。

## 必读文件

请先阅读：

- `.agents/ue-project-context.md`
- `Agent/00_Coordination/WorkingAgreement.md`
- `Agent/02_SkillSystem/Combat_Foundation_RFC.md`
- `Agent/02_SkillSystem/2026-07-09_SkillSystem_Architecture_Refactor_Summary.md`
- `Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillArchitecture_Foundation_Phase1_Report.md`
- `Agent/04_AgentHandoff/Claude_Reports/2026-07-09_SkillPolicy_Phase1_1_Fix_Report.md`
- `Config/DefaultGameplayTags.ini`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Source/Aura/Public/AbilitySystem/AuraAttributeSet.h`
- `Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp`
- `Source/Aura/Public/AbilitySystem/ExecCalc/ExecCalc_Damage.h`
- `Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp`
- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraSkillRuntimeAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraSkillRuntimeAbility.cpp`
- `Source/Aura/Public/AbilitySystem/SkillTemplates/`
- `Source/Aura/Private/AbilitySystem/SkillTemplates/`

建议使用的知识/Skill：

- `ue-gameplay-abilities`
- `ue-cpp-foundations`
- `ue-module-build-system`
- 如涉及 AttributeSet 复制，使用 `ue-networking-replication`

## 本轮核心原则

### 1. 稳定玩法 Tag 与 GAS 技术键分开

稳定玩法 Tag 可以整理或新增：

```text
Ability.*
InputTag.*
State.*
Event.Montage.*
Damage.Channel.*
Damage.Type.*
GameplayCue.*
Outcome.*   // 仅模板级少量结果
```

`SetByCaller.*` 不归入稳定玩法 Tag 命名域。它只是 GAS API 要求的 GE Spec 数值 key。

允许保留已有且被代码或 GE 使用的：

```text
SetByCaller.Damage
SetByCaller.Healing
SetByCaller.HealingPerTick
```

禁止新增 per-skill key：

```text
SetByCaller.FireballDamage
SetByCaller.VitalSurgeFullHeal
SetByCaller.BurningFieldTick
```

如果某个数值可以直接放在技能配置、GE Magnitude 或模板字段里，不要为了“数据驱动”强行新增 SetByCaller Tag。

### 2. 不做复杂属性公式

当前阶段的伤害可以先保持：

```text
FinalDamage = BaseDamage
```

不要在本轮实现完整公式：

```text
BaseDamage + SourcePower - TargetDefense
Armor / MagicResistance / Penetration
暴击 / 格挡 / 闪避
元素抗性
```

可以为长期目标补文档或注释，但不要把代码做复杂。

### 3. 不大规模重构 AttributeSet

用户已确认 V1 目标属性清单：

```text
Health
MaxHealth
Mana
MaxMana
PhysicalPower
MagicalPower
Armor
MagicResistance
PhysicalPenetration
MagicPenetration
IncomingDamage
IncomingHealing
```

但本轮不要求立刻迁移旧教程属性，也不要求删除旧属性。

允许做的最小改动：

- 如果当前治疗路径确实需要 `IncomingHealing`，可以补最小字段、复制、访问器和 `PostGameplayEffectExecute` 处理。
- 如果当前治疗已经通过直接 Health modifier 跑通，可以只记录后续迁移建议，不强行改。
- 不要删除 `Strength / Intelligence / Resilience / Vigor / FireResistance` 等旧教程属性。

### 4. `Damage.Channel.*` 是长期语义，不代表本轮公式已完成

建议新增或确认：

```text
Damage.Channel.Physical
Damage.Channel.Magical
Damage.Channel.True
Damage.Type.Fire
```

说明：

- `Damage.Channel.*` 先作为长期通道语义。
- 当前伤害仍可简单扣血。
- 未来再让 ExecCalc 根据 Channel 选择物理/法术公式。

不要急着删除旧：

```text
Damage.Magical
Damage.Physical
Damage.Fire
Damage.Arcane
Damage.Lightning
```

删除旧 Tag 需要先检查资产和代码引用。本轮可以保留旧 Tag，并在报告中说明迁移建议。

## 具体任务

### 任务 1：审计当前 Tag 使用

用 `rg` 检查以下内容：

```text
Damage.Magical
Damage.Physical
Damage.Fire
Damage.Arcane
Damage.Lightning
Damage.Channel
SetByCaller
Outcome.Channel
State.Skill.Channeling
CombatEntity
GameplayCue
```

输出到报告中：

- 哪些 Tag 已存在于 `DefaultGameplayTags.ini`。
- 哪些 Tag 已在 `FAuraGameplayTags` native tag 中注册。
- 哪些 Tag 被 C++ 引用。
- 哪些 Tag 可能被 `.uasset` 引用，无法通过文本确认，需要用户或 UE 编辑器后续验证。

### 任务 2：整理最小稳定 Tag

在不破坏现有资产的前提下，最小化整理：

建议新增到 `Config/DefaultGameplayTags.ini`：

```text
Damage.Channel.Physical
Damage.Channel.Magical
Damage.Channel.True
Damage.Type.Fire
```

如果 `State.Skill.Channeling`、`Outcome.Channel.Full`、`Outcome.Channel.Partial`、`Outcome.Channel.Cancelled` 已经存在，请不要重复添加。

如果 C++ 当前需要 native tag，请在 `FAuraGameplayTags` 中补对应成员和 `AddNativeGameplayTag`。如果暂时只是配置资产用，不必强行 native 化。

注意：

- 不要新增具体 `CombatEntity.Projectile.Fireball`。
- 不要新增 `CombatEntity.*`，除非代码已经明确需要分类判断。
- 不要新增新的 `SetByCaller.*`，除非现有 GE 或代码必须使用。

### 任务 3：明确简单伤害路径

检查当前 `ExecCalc_Damage` 和 `UAuraAttributeSet::PostGameplayEffectExecute`。

要求：

- 本轮不要实现复杂公式。
- 保持或调整为能稳定完成简单扣血。
- 如果 `ExecCalc_Damage` 当前依赖旧 `DamageTypesToResistances`，不要强行重写成新通道公式。
- 可以加中文注释说明：当前是临时简单结算，`Damage.Channel.*` 只是长期语义，正式公式后续实现。

如果发现当前代码已经有复杂教程公式：

- 不要求完全删除。
- 不要扩大它。
- 报告中说明它与新口径的差异，以及后续迁移建议。

### 任务 4：治疗路径只做必要最小处理

检查当前治疗技能或 GE 相关代码是否需要 `IncomingHealing`。

两种可接受结果：

结果 A：不新增 `IncomingHealing`

- 如果当前治疗已经能通过直接修改 `Health` 跑通，则本轮不动 AttributeSet。
- 报告中写明：`IncomingHealing` 是目标口径，但等治疗系统整理时再加。

结果 B：新增最小 `IncomingHealing`

- 只加最小字段、访问器、`PostGameplayEffectExecute` 分支。
- `IncomingHealing` 执行后必须清零。
- 只负责 `Health = Clamp(Health + IncomingHealing, 0, MaxHealth)`。
- 不做治疗加成、暴击治疗、治疗抗性等复杂机制。

请根据当前代码实际情况选择，不要为了“看起来完整”强行改。

### 任务 5：同步中文文档

如有代码改动，请补充或更新一个中文说明文件：

```text
Agent/02_SkillSystem/Combat_Foundation_Runtime_Notes.md
```

内容至少说明：

- 稳定玩法 Tag 与 `SetByCaller.*` 技术键的区别。
- 当前为什么先使用简单伤害表现。
- `Damage.Channel.*` 当前阶段的用途。
- 哪些内容后续才会做：复杂公式、属性迁移、元素抗性、完整 BuffDefinition。

## 不要做

- 不要实现完整火球术。
- 不要创建或修改 `.uasset`。
- 不要使用 MCP 改 UE 编辑器资产。
- 不要做完整技能编辑器。
- 不要重写全部 AttributeSet。
- 不要删除旧教程属性。
- 不要删除旧 Damage Tag，除非你已经证明没有代码和资产引用。
- 不要新增 per-skill `SetByCaller.*`。
- 不要引入复杂伤害公式。
- 不要修改与本轮无关的 UI、AI、地图、PCG、MCP 插件文件。

## 编译要求

请使用 UE 5.8 构建命令：

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
Agent/04_AgentHandoff/Claude_Reports/2026-07-13_CombatFoundation_PhaseB_MinimalTagsAndSimpleDamage_Report.md
```

报告至少包含：

1. 修改了哪些文件。
2. Tag 审计结果。
3. 新增了哪些稳定玩法 Tag。
4. 是否新增或保留 `SetByCaller.*`，理由是什么。
5. 是否修改 AttributeSet，理由是什么。
6. 当前伤害路径是否保持简单。
7. 是否执行编译，结果如何。
8. 未做事项和下一步建议。

## 验收标准

- C++ 可编译，或报告清楚说明未编译原因。
- 不新增临时 per-skill GameplayTag。
- `SetByCaller.*` 不再被描述为玩法 Tag，只作为 GAS 技术数值键。
- `Damage.Channel.Physical / Magical / True` 作为长期通道语义存在或有明确不新增理由。
- 当前伤害实现保持简单，不引入完整公式。
- 不破坏现有火球术/生命涌动原型编译。
- 报告和新增说明文档为中文。
