# Aura 战斗基础运行时说明

创建日期：2026-07-13  
状态：Phase B 最小 Tag 与简单伤害表现

## 1. 稳定玩法 Tag 与 GAS 技术键的区别

### 稳定玩法 Tag

这些 Tag 表示长期语义，可被多个系统查询、匹配、阻塞、监听：

| 命名域 | 用途 | 示例 |
|--------|------|------|
| `Ability.*` | 技能身份 | `Ability.Fire.Fireball` |
| `InputTag.*` | 输入绑定 | `InputTag.1` |
| `State.*` | 角色玩法状态 | `State.Skill.Channeling` |
| `Event.Montage.*` | Montage 关键帧事件 | `Event.Montage.FireBolt` |
| `Damage.Channel.*` | 伤害结算通道 | `Damage.Channel.Magical` |
| `Damage.Type.*` | 伤害元素语义 | `Damage.Type.Fire` |
| `Outcome.*` | 技能执行分支结果（模板级） | `Outcome.Channel.Full` |
| `GameplayCue.*` | 表现事件 | `GameplayCue.Skill.Fireball.Impact` |

### GAS 技术数值键（SetByCaller）

`SetByCaller.*` 不是玩法 Tag。它们是 GAS `GameplayEffectSpec` 的运行时数值 key。

用途：创建 GE Spec 时通过 `AssignTagSetByCallerMagnitude` 塞入数值，让 GE 或 ExecCalc 读取。

当前保留的通用键：

| Tag | 用途 |
|-----|------|
| `SetByCaller.Damage` | 未来通用基础伤害 key 候选/预留（当前 C++ 未使用；现有伤害路径使用 Damage.Fire 等作为 SetByCaller key） |
| `SetByCaller.Healing` | 一次瞬间治疗数值 |
| `SetByCaller.HealingPerTick` | 周期治疗每跳数值 |

禁止新增 per-skill 键（如 `SetByCaller.FireballDamage`）。

如果某个数值可以直接放在技能配置、GE 固定 Magnitude 或模板字段里，不必强行走 SetByCaller。

## 2. 当前伤害公式说明

**Phase B 没有新增复杂公式，但当前 ExecCalc_Damage 仍保留教程时代的完整计算：**

- 遍历 `DamageTypesToResistances`，用 `Damage.Fire` / `Damage.Physical` 等作为 SetByCaller key 读取伤害数值。
- 对每种伤害类型查抗性百分比并扣减。
- 计算 BlockChance、Armor/ArmorPenetration（通过曲线）、CriticalHitChance/CriticalHitResistance/CriticalHitDamage。
- 最终写入 `IncomingDamage`。

**"简单伤害表现"是我们的下一阶段目标口径，不是当前代码已完全做到。** 当前公式并未删减为 `FinalDamage = BaseDamage`。

下一阶段技能如果想保持最小验证可选择：继续复用旧 ExecCalc，或另建真正简单的临时 GE 路径（需单独决策）。

后续正式物理/法术通道公式草案：
```
Physical: BaseDamage + PhysicalPower - max(0, Armor - PhysicalPenetration)
Magical:  BaseDamage + MagicalPower - max(0, MagicResistance - MagicPenetration)
True:     BaseDamage
```

## 3. `Damage.Channel.*` 当前阶段的用途

`Damage.Channel.*` 是长期语义，表示伤害走哪条结算通道：

| Tag | 结算含义 |
|-----|----------|
| `Damage.Channel.Physical` | 使用物理攻击力/护甲/物理穿透 |
| `Damage.Channel.Magical` | 使用法术攻击力/法术抗性/法术穿透 |
| `Damage.Channel.True` | 真实伤害，绕过防御 |

当前阶段：
- Tag 已创建，语义已定义。
- 实际结算仍使用旧教程 ExecCalc_Damage（包含抗性、格挡、护甲、暴击等完整计算）。
- 正式物理/法术通道公式待属性系统迁移后实现。

`Damage.Type.*` 与 Channel 的区别：Channel 决定公式，Type 决定语义（Fire 可用于燃烧触发、免疫检查、Cue 选择）。

## 4. 当前属性状态

### 当前已存在（教程属性）

`AuraAttributeSet` 仍保留所有教程属性：Strength, Intelligence, Resilience, Vigor, Armor, ArmorPenetration, BlockChance, CriticalHitChance, CriticalHitDamage, CriticalHitResistance, HealthRegeneration, ManaRegeneration, FireResistance, LightningResistance, ArcaneResistance, PhysicalResistance, Health, MaxHealth, Mana, MaxMana, IncomingDamage。

### V1 目标属性（尚未迁移）

Health, MaxHealth, Mana, MaxMana, PhysicalPower, MagicalPower, Armor, MagicResistance, PhysicalPenetration, MagicPenetration, IncomingDamage, IncomingHealing。

### 本轮不做的迁移

- 不新增 `PhysicalPower` / `MagicalPower` 等新属性。
- 不删除任何旧教程属性。
- `IncomingHealing` 本轮不新增（当前治疗直接通过 Health modifier 跑通，无需中转槽）。
- 后续属性迁移时再统一写迁移方案。

## 5. 当前治疗路径

治疗通过 GE（Instant 或 Periodic）+ `SetByCaller.Healing` / `SetByCaller.HealingPerTick` 直接修改 Health。

`UAuraAttributeSet::PostGameplayEffectExecute` 中 `Health = Clamp(Health + Delta, 0, MaxHealth)`。

没有 `IncomingHealing` Meta Attribute。当前路径足以验证治疗行为。

## 6. 后续才会实现

| 内容 | 阶段 |
|------|------|
| Damage.Channel 正式物理/法术公式 | 属性系统迁移后 |
| 属性迁移（PhysicalPower 等新属性） | 后置阶段 |
| 元素抗性系统 | 后置阶段 |
| 暴击/格挡/闪避 | V1 不做 |
| 完整 BuffDefinition | 后置阶段 |
| IncomingHealing Meta Attribute | 治疗系统整理后 |
