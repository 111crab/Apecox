# 战斗基础 Phase B — 稳定 Tag 与简单伤害表现 实施报告

*创建日期：2026-07-13*
*实施者：Claude*
*任务来源：Agent/04_AgentHandoff/Claude_Prompts/2026-07-13_CombatFoundation_PhaseB_MinimalTagsAndSimpleDamage.md*

## 1. 修改文件

| 文件 | 变更 |
|------|------|
| `Config/DefaultGameplayTags.ini` | 新增 4 个稳定玩法 Tag；更新 `SetByCaller.Damage` 和 `Damage.Magical` DevComment |
| `Source/Aura/Public/AuraGameplayTags.h` | 更新 SetByCaller 注释，标明为 GAS 技术数值键非玩法 Tag |
| `Agent/02_SkillSystem/Combat_Foundation_Runtime_Notes.md` | 新建中文说明文档 |

## 2. Tag 审计结果

### 已存在于 `DefaultGameplayTags.ini` 的 Tag

| Tag | 状态 |
|------|------|
| `Ability.Fire.Fireball` | ✅ 存在 |
| `Ability.Vital.VitalSurge` | ✅ 存在 |
| `Event.Montage.FireBolt` | ✅ 存在 |
| `Outcome.Channel.Full` | ✅ 存在 |
| `Outcome.Channel.Partial` | ✅ 存在 |
| `Outcome.Channel.Cancelled` | ✅ 存在 |
| `SetByCaller.Damage` | ✅ 存在（已标注为 GAS 技术键） |
| `Damage.Magical` | ✅ 存在（已标注为 Legacy，迁移方向 Damage.Channel.Magical） |

### 已在 `FAuraGameplayTags` native tag 注册的 Tag

| Native Tag | 对应 Tag Name |
|------------|--------------|
| `Damage_Fire` | `Damage.Fire` |
| `Damage_Lightning` | `Damage.Lightning` |
| `Damage_Arcane` | `Damage.Arcane` |
| `Damage_Physical` | `Damage.Physical` |
| `SetByCaller_Healing` | `SetByCaller.Healing` |
| `SetByCaller_HealingPerTick` | `SetByCaller.HealingPerTick` |
| `State_Skill_Channeling` | `State.Skill.Channeling` |
| `Attributes_Resistance_Fire` | `Attributes.Resistance.Fire` |
| `Attributes_Resistance_Lightning` | `Attributes.Resistance.lightning`（旧教程遗留大小写问题，本轮未修复，后续 Tag 规范化阶段处理） |
| `Attributes_Resistance_Arcane` | `Attributes.Resistance.Arcane` |
| `Attributes_Resistance_Physical` | `Attributes.Resistance.Physical` |

### C++ 引用的 Tag

- `Damage.Fire` / `Damage.Lightning` / `Damage.Arcane` / `Damage.Physical` — `AuraGameplayTags.cpp` native 注册 + `AuraProjectileSpell.cpp` SetByCaller 写入 + `ExecCalc_Damage.cpp` 读取
- `SetByCaller.Healing` / `SetByCaller.HealingPerTick` — `AuraGameplayTags.cpp` native 注册
- `State.Skill.Channeling` — `AuraGameplayTags.cpp` native 注册
- `Damage.Magical` — 仅 `DefaultGameplayTags.ini` 配置，未被 C++ 引用

### 无法确认的 .uasset 引用

以下 Tag 可能被已有 `.uasset`（GE、GA 蓝图、技能资产）引用，需用户在 UE 编辑器中确认：
- `Damage.Fire` — 可能被 `GE_Damage` 或火球术资产引用
- `SetByCaller.Healing` / `SetByCaller.HealingPerTick` — 可能被治疗 GE 资产引用
- `Event.Montage.FireBolt` — 被 `AM_Cast_FireBolt` Montage 引用

## 3. 新增的稳定玩法 Tag

| Tag | DevComment | 用途 |
|-----|-----------|------|
| `Damage.Channel.Physical` | Physical damage calculation channel | 物理伤害结算通道（长期） |
| `Damage.Channel.Magical` | Magical damage calculation channel | 法术伤害结算通道（长期） |
| `Damage.Channel.True` | True damage — bypasses defense | 真实伤害通道（预留） |
| `Damage.Type.Fire` | Fire semantic type | 火焰元素语义（预留） |

旧 Tag 保留，未删除。`Damage.Magical` 的 DevComment 已标注为 Legacy。

## 4. SetByCaller 处理

### 保留的通用键

| Tag | 理由 |
|-----|------|
| `SetByCaller.Damage` | 未来通用基础伤害 key 候选/预留。**当前 C++ 未读取它**；已有教程伤害路径使用 `Damage.Fire` / `Damage.Lightning` / `Damage.Arcane` / `Damage.Physical` 作为 SetByCaller key。下一阶段如果要改用此 key，需同步更新 GE / ExecCalc 使其能读取 |
| `SetByCaller.Healing` | 现有治疗 GE 需要动态治疗值时使用 |
| `SetByCaller.HealingPerTick` | 现有持续治疗 GE 需要动态每跳值时使用 |

### 禁止新增的 per-skill 键

未新增任何 `SetByCaller.FireballDamage`、`SetByCaller.VitalSurgeFullHeal` 等 per-skill 键。

### 注释更新

`AuraGameplayTags.h` 中 SetByCaller 区域已添加注释说明这些是 GAS 技术数值键，不归入玩法 Tag 体系。

## 5. AttributeSet 处理

**结果 A：不新增 `IncomingHealing`**

理由：
- 当前治疗路径通过 GE 直接修改 Health 已经跑通。
- `UAuraAttributeSet::PostGameplayEffectExecute` 已处理 `Health = Clamp(Health + Delta, 0, MaxHealth)`。
- 没有需要中转入 `IncomingHealing` 的复杂治疗公式需求。
- 等治疗系统整体整理时再引入 `IncomingHealing` Meta Attribute。

未修改 `AuraAttributeSet.h/.cpp`。

## 6. 当前伤害路径

Phase B 没有新增复杂公式，但当前项目 `ExecCalc_Damage` 仍保留教程时代的复杂计算：

- 遍历 `FAuraGameplayTags::DamageTypesToResistances`，用 `Damage.Fire` / `Damage.Lightning` / `Damage.Arcane` / `Damage.Physical` 作为 SetByCaller key，读取伤害数值。
- 对每种伤害类型查抗性 (`Attributes.Resistance.*`），扣百分比抗性。
- 计算 `BlockChance`，格挡后伤害减半。
- 读取 `Armor` 和 `ArmorPenetration`，通过曲线计算有效护甲并扣减。
- 读取 `CriticalHitChance`、`CriticalHitResistance`、`CriticalHitDamage`，计算暴击。
- 最终写入 `IncomingDamage` Meta Attribute。
- `PostGameplayEffectExecute` 处理 `IncomingDamage` → 扣 Health。

**"简单伤害表现"是下一阶段的目标口径，不是当前代码已经做到的。** 下一阶段技能如果想保持最小验证，可以选择：
- 继续复用旧 `ExecCalc_Damage`（包含上述复杂计算）。
- 或另建/配置一个真正简单的临时 GE 路径（需单独决策）。

`Damage.Channel.*` 作为长期语义已创建，但正式物理/法术公式待属性迁移后实现。

## 7. 编译结果

本轮为文档准确性修复，未改 C++，未重新编译。上一轮构建结果仍为 `Result: Succeeded`（Exit code 0）。

## 8. 未做事项和下一步建议

| 未做事项 | 原因 | 建议阶段 |
|----------|------|----------|
| 新增 `PhysicalPower` / `MagicalPower` 等新属性 | 本轮不迁移属性系统 | 属性迁移阶段 |
| 实现 `Damage.Channel.*` 正式公式 | 先稳定 Tag 语义 | 属性迁移后 |
| 删除旧 `Damage.Fire` / `Damage.Arcane` 等 Tag | 可能被 .uasset 引用 | 资产审计后逐步替换 |
| 新增 `IncomingHealing` | 当前治疗路径无需中转槽 | 治疗系统整理后 |
| 在 ExecCalc 中引入 `Damage.Channel.*` 分支 | 当前 ExecCalc 使用旧 `DamageTypesToResistances` | 属性迁移阶段 |
| 重写火球术为新架构 | 先稳定基础语言 | Phase C |
