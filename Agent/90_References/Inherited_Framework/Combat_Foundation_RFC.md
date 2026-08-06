# Aura 战斗基础 RFC：Tag、属性、伤害与技能命中链路

创建日期：2026-07-13  
状态：阶段 A 已确认；已按用户反馈修订  
范围：GameplayTag 命名、基础属性系统、伤害通道、GAS 技术数值键、战斗衍生物、技能从释放到命中的公共链路  
当前结论：先不继续写具体技能代码，先把战斗系统的基础语言定清楚。

## 1. 本文目的

我们前面已经确认了技能系统的长期方向：

- `UAuraSkillDefinition` 是技能组装入口。
- `UAuraSkillRuntimeAbility` 是 GAS 运行时入口。
- `UAuraSkillTemplate` 描述同类技能流程。
- `UAuraCombatEntityDefinition` 描述投射物、法术场、召唤物、陷阱等战斗衍生物。
- V1 先做 Effect / State，不急着做完整 BuffDefinition。

但这些结构要能长期工作，必须先统一几件更底层的事情：

- 哪些东西应该是 GameplayTag。
- 哪些东西应该是 Attribute。
- 哪些东西只是配置字段。
- 一次技能从输入、生成、命中到结算，需要带着哪些信息流动。
- 火球术这种技能到底需要哪些 Tag 和属性参与。

本文就是这个基础口径。

## 2. 先回答当前疑惑

### 2.1 `CombatEntity.*` 是否必须有

结论：不是每个战斗衍生物都必须单独创建一个身份 Tag。

`CombatEntity.*` 的用途不是伤害计算，也不是生成 Actor 的必要条件。生成投射物、法术场时，真正必需的是 `UAuraCombatEntityDefinition` 资产引用和对应 Actor 类。

`CombatEntity.*` 只在这些场景才有价值：

- 编辑器里按类型检索和过滤战斗衍生物。
- 运行时规则需要判断“这个衍生物属于哪一类”。
- 日志、调试、GameplayCue 或 Effect 需要读取衍生物语义。
- 某些技能规则要针对一类衍生物生效，例如“强化下一个 Projectile”。

所以 V1 建议：

- 先保留根域 `CombatEntity.*` 的设计位置。
- 不强制给每个具体投射物写 `CombatEntity.Projectile.Fireball`。
- 如需分类，优先使用粗粒度 Tag，例如 `CombatEntity.Projectile`、`CombatEntity.Field`。
- 具体火球投射物的身份优先由资产 `DA_Entity_FireballProjectile` 表达。

这能避免“每做一个衍生物就多一个 Tag”的膨胀。

### 2.2 `SetByCaller.*` 是服务于什么

结论：`SetByCaller.*` 不是我们设计的玩法语义 Tag，而是 GAS `GameplayEffectSpec` 里的技术数值键。

它的用途是：创建 GE Spec 时临时塞入一个数值，让 GE 或 ExecutionCalculation 能读取。

例子：

```text
SetByCaller.Damage = 50
SetByCaller.Healing = 40
SetByCaller.HealingPerTick = 10
```

这里的 GameplayTag 只是“键名”。它不是角色拥有的状态，也不应该挂在 ASC 上，更不应该参与技能分类、状态判断或编辑器语义搜索。

这个点需要单独强调：GAS 的 SetByCaller API 要求用 GameplayTag 做 key，这不是因为它适合表达玩法语义，而是 UE 提供的机制就是这样。因此我们不把 `SetByCaller.*` 归入“稳定玩法 Tag 体系”，只把它当作“GAS 技术数值键”。

如果某个阶段确定不会用 SetByCaller，就不应该为了预留而新增一堆 `SetByCaller.*` Tag。V1 只在 GE 资产确实需要动态数值键时，保留极少数通用键。

V1 如果继续使用 SetByCaller，建议只保留少量通用键：

| Tag | 用途 |
| --- | --- |
| `SetByCaller.Damage` | 一次伤害的基础数值或技能给出的伤害量 |
| `SetByCaller.Healing` | 一次瞬间治疗数值 |
| `SetByCaller.HealingPerTick` | 周期治疗每跳数值 |

后续如果确实有需要，再补：

| Tag | 用途 |
| --- | --- |
| `SetByCaller.Duration` | 运行时决定持续时间 |
| `SetByCaller.Radius` | 运行时决定范围半径 |
| `SetByCaller.Stacks` | 运行时决定层数 |

命名原则：

- `SetByCaller.*` 只能表示“通用数值槽”。
- 不写 `SetByCaller.FireballDamage` 这种 per-skill 键。
- 不把它作为设计层 Tag 展示给普通技能编辑流程。
- 如果某个数值可以直接存在技能配置或 GE 固定 Magnitude 里，就不必强行 SetByCaller。

### 2.3 `Damage.Channel.*` 是服务于什么

结论：`Damage.Channel.*` 用来选择伤害公式和防御属性。

它回答的是：“这次伤害走哪条结算通道？”

V1 建议三类就够：

| Tag | 结算含义 |
| --- | --- |
| `Damage.Channel.Physical` | 使用物理攻击、物理防御、物理穿透 |
| `Damage.Channel.Magical` | 使用法术攻击、法术抗性、法术穿透 |
| `Damage.Channel.True` | 真实伤害，绕过普通防御，后置实现 |

例如火球术：

```text
Damage.Channel.Magical
SetByCaller.Damage = 50
```

这表示火球术的基础伤害值是 50，但最终伤害要进入法术通道，未来可以叠加释放者法术伤害、目标法术抗性、法术穿透等属性。

### 2.4 `Damage.Type.*` 与 `Damage.Channel.*` 的区别

结论：`Channel` 决定公式，`Type` 决定语义。

`Damage.Channel.Magical` 表示用法术公式结算。  
`Damage.Type.Fire` 表示这次伤害是火焰语义。

火焰语义未来可用于：

- 触发燃烧。
- 被火焰免疫阻挡。
- 选择火焰命中特效。
- 与“受到火焰伤害时触发”的被动交互。

V1 如果暂时只关心物理和法术，可以先实现 `Damage.Channel.*`，把 `Damage.Type.*` 作为预留。

建议不要继续使用含义模糊的 `Damage.Magical` 作为长期主命名。它既像通道，又像类型。长期应迁移为：

```text
Damage.Channel.Magical
Damage.Channel.Physical
Damage.Type.Fire
Damage.Type.Arcane
```

### 2.5 `Outcome.*` 是服务于什么

结论：`Outcome.*` 表示一次技能执行内部的分支结果，不是角色状态。

它不应该长期挂在 ASC 上，也不表示角色进入某个状态。

例子：生命涌动引导技能。

```text
Outcome.Channel.Full
Outcome.Channel.Partial
Outcome.Channel.Cancelled
```

这些结果只服务于这一次技能结算：

- Full：应用完整瞬间治疗。
- Partial：应用半量持续治疗。
- Cancelled：不应用治疗。

是否必须用 GameplayTag 表示 Outcome？

V1 可以接受两种实现：

| 方式 | 优点 | 缺点 |
| --- | --- | --- |
| C++ 枚举，例如 `EAuraSkillOutcome` | 类型安全，不污染 GameplayTag 表 | 编辑器配置分支时不如 Tag 灵活 |
| GameplayTag，例如 `Outcome.Channel.Full` | 易做配置映射，适合技能编辑器和数据驱动 | 需要严格限制粒度，避免膨胀 |

当前建议：保留 `Outcome.Channel.Full / Partial / Cancelled` 这一组，因为它们是引导模板的通用结果，不是某个技能的临时 Tag。

禁止写法：

```text
Outcome.Fireball.BigHit
Outcome.VitalSurge.SpecialCase
```

如果未来某个模板需要新结果，应按模板粒度扩展，例如：

```text
Outcome.Combo.First
Outcome.Combo.Second
Outcome.Combo.Finisher
```

## 3. GameplayTag 总原则

GameplayTag 只应该承担这些职责：

- 身份：这个技能是谁，这个输入是谁。
- 分类：这类技能、这类效果、这类伤害属于什么。
- 状态：角色当前是否处于某个可被规则查询的状态。
- 事件：某个关键帧或技能阶段发生了。
- GAS 技术键：SetByCaller 这类 UE API 需要的 key，但它不归入玩法语义 Tag。
- 表现入口：GameplayCue 事件。

GameplayTag 不应该承担这些职责：

- 不存数值。
- 不替代配置资产。
- 不表达每个临时特殊字段。
- 不当作所有技能逻辑的万能开关。

一句话规则：

```text
需要被多个系统查询、匹配、阻塞、监听、配置分支的语义，才考虑做 Tag。
只在一个类内部使用的临时变量，优先用 enum、bool、结构体字段或资产引用。
```

## 4. V1 建议 Tag 命名域

### 4.1 必须稳定的玩法命名域

| 命名域 | 用途 | 示例 |
| --- | --- | --- |
| `Ability.*` | 技能身份和技能大类 | `Ability.Fire.Fireball` |
| `InputTag.*` | 输入绑定 | `InputTag.LMB`、`InputTag.1` |
| `State.*` | 角色或 ASC 当前玩法状态 | `State.Skill.Channeling`、`State.Control.Stunned` |
| `Event.Montage.*` | Montage Notify 发给 GA 的关键帧事件 | `Event.Montage.Fire` |
| `Damage.Channel.*` | 伤害结算通道 | `Damage.Channel.Magical` |
| `GameplayCue.*` | 表现事件 | `GameplayCue.Skill.Fireball.Impact` |

### 4.1.1 GAS 技术键，不归入玩法命名域

| 命名域 | 用途 | V1 态度 |
| --- | --- | --- |
| `SetByCaller.*` | GE Spec 运行时数值 key | 只在 GE 确实需要动态数值时使用；不扩张，不做 per-skill key |

### 4.2 可以保留但不急着大规模使用的命名域

| 命名域 | 用途 | V1 态度 |
| --- | --- | --- |
| `Damage.Type.*` | 元素、来源语义、免疫和后续状态 | 先预留，火球可先记录为 Fire，但公式优先看 Channel |
| `CombatEntity.*` | 战斗衍生物分类或身份 | 不强制每个实体都有具体 Tag |
| `Outcome.*` | 技能执行分支结果 | 只保留模板级少量 Tag |
| `Effect.*` | 可复用效果语义 | 等 Effect/State 整理时再稳定 |

### 4.3 暂不建议新增的命名域

| 命名域 | 暂缓原因 |
| --- | --- |
| `Buff.*` | 完整 BuffDefinition 后置，V1 先做 Effect / State |
| `Damage.Element.*` | 和 `Damage.Type.*` 容易重复，先不用 |
| `SkillPhase.*` | 长期 Skill Timeline 再考虑 |

## 5. 基础属性系统 V1

用户已确认概率属性目前先不考虑。因此 V1 目标属性建议收敛为下列清单。

注意：这是目标口径，不等于下一步马上做完整伤害公式。当前阶段优先继续技能实现和简单伤害表现，属性公式、复杂抗性、成长曲线可以后置。

### 5.1 生命资源

| Attribute | 作用 |
| --- | --- |
| `Health` | 当前生命 |
| `MaxHealth` | 最大生命 |
| `Mana` | 当前蓝量 |
| `MaxMana` | 最大蓝量 |

### 5.2 攻击属性

| Attribute | 作用 |
| --- | --- |
| `PhysicalPower` | 物理伤害来源属性 |
| `MagicalPower` | 法术伤害来源属性 |

### 5.3 防御属性

| Attribute | 作用 |
| --- | --- |
| `Armor` | 物理防御 |
| `MagicResistance` | 法术抗性 |

### 5.4 穿透属性

| Attribute | 作用 |
| --- | --- |
| `PhysicalPenetration` | 物理穿透 |
| `MagicPenetration` | 法术穿透 |

### 5.5 Meta Attribute

| Attribute | 作用 |
| --- | --- |
| `IncomingDamage` | 伤害结算临时入口，执行后清零 |
| `IncomingHealing` | 治疗结算临时入口，执行后清零 |

Meta Attribute 不代表角色长期属性。它们是 GE/ExecCalc 把结果送进 AttributeSet 的中转槽。

## 6. 与当前 Aura 教程属性的关系

当前项目仍保留教程属性：

- `Strength`
- `Intelligence`
- `Resilience`
- `Vigor`
- `Armor`
- `ArmorPenetration`
- `BlockChance`
- `CriticalHitChance`
- `CriticalHitDamage`
- `CriticalHitResistance`
- `HealthRegeneration`
- `ManaRegeneration`
- `FireResistance`
- `LightningResistance`
- `ArcaneResistance`
- `PhysicalResistance`
- `Health`
- `Mana`
- `IncomingDamage`

长期重构方向不是立刻全部删除，而是先建立新口径：

| 旧属性 | V1 方向 |
| --- | --- |
| `Strength` | 可迁移为或派生出 `PhysicalPower` |
| `Intelligence` | 可迁移为或派生出 `MagicalPower` |
| `Resilience` | 暂不作为核心属性 |
| `Vigor` | 暂不作为核心属性，可由装备/等级直接给 `MaxHealth` |
| `ArmorPenetration` | 命名迁移为 `PhysicalPenetration` |
| `FireResistance / LightningResistance / ArcaneResistance` | 元素抗性后置 |
| `PhysicalResistance` | 可并入或映射到物理防御模型 |
| 暴击、格挡、回血回蓝 | V1 暂缓 |

后续真正迁移属性系统时，需要先写迁移方案，再改 `UAuraAttributeSet` 和对应 GE/ExecCalc。当前阶段不急着做完整属性迁移。

## 7. 伤害结算 V1 口径

一次伤害至少由三部分组成：

```text
基础数值：SetByCaller.Damage 或技能配置/等级曲线
结算通道：Damage.Channel.Physical / Magical / True
可选语义：Damage.Type.Fire / Arcane / Physical 等
```

### 7.1 当前实现策略：先简单表现

当前项目先不做复杂计算。V1 技能验证阶段可以采用最简单的结算：

```text
FinalDamage = BaseDamage
```

这里的 `BaseDamage` 可以来自技能配置、GE 固定 Magnitude，或者在确实需要动态注入时来自 `SetByCaller.Damage`。

这样做的目的不是放弃伤害系统，而是先保证：

- 技能能释放。
- 命中能被服务器判定。
- 伤害能正确扣血和同步。
- GameplayCue / 飘字 / UI 能看到结果。
- 多人下其他端能看到正确表现。

### 7.2 后续物理通道公式草案

物理通道未来可读取：

- Source: `PhysicalPower`
- Target: `Armor`
- Source: `PhysicalPenetration`

后续公式草案：

```text
EffectiveArmor = max(0, Target.Armor - Source.PhysicalPenetration)
FinalDamage = max(0, BaseDamage + Source.PhysicalPower - EffectiveArmor)
```

### 7.3 后续法术通道公式草案

法术通道未来可读取：

- Source: `MagicalPower`
- Target: `MagicResistance`
- Source: `MagicPenetration`

后续公式草案：

```text
EffectiveResistance = max(0, Target.MagicResistance - Source.MagicPenetration)
FinalDamage = max(0, BaseDamage + Source.MagicalPower - EffectiveResistance)
```

### 7.4 真实通道

真实通道后置。第一版可以只定义 Tag，不实现技能。

```text
FinalDamage = BaseDamage
```

### 7.5 火球术示例

火球术在 V1 可以表达为：

```text
SetByCaller.Damage = 50
Damage.Channel.Magical
Damage.Type.Fire
```

如果使用临时简单结算，执行含义是：

- `SetByCaller.Damage` 或技能配置提供基础伤害。
- `Damage.Channel.Magical` 先作为长期语义保留，不马上启用复杂法术公式。
- `Damage.Type.Fire` 给后续燃烧、免疫、Cue 或元素扩展保留语义。

如果 V1 暂时不做元素系统，也可以先只落：

```text
SetByCaller.Damage = 50
Damage.Channel.Magical
```

## 8. 技能从释放到命中的信息链路

以火球术为例，一次完整链路应该是：

```text
输入
  -> ASC 根据 InputTag 找到 AbilitySpec
  -> UAuraSkillRuntimeAbility 激活
  -> 从 AbilitySpec.SourceObject 读取 UAuraSkillDefinition
  -> Policy 检查释放者状态、资源、冷却、阻塞关系
  -> Commit 成本和冷却
  -> SkillTemplate 执行 ProjectileCast 流程
  -> 播放 Montage
  -> WaitGameplayEvent 等待 Event.Montage.Fire
  -> 解析释放目标，例如鼠标点或方向
  -> 根据 CombatEntityDefinition 生成投射物
  -> 投射物移动并检测命中
  -> 命中时服务器创建 GE Spec
  -> 写入基础伤害数值，必要时使用 SetByCaller.Damage
  -> 写入或携带 Damage.Channel / Damage.Type
  -> V1 先使用临时简单结算；后续 ExecCalc 再读取 Source/Target 属性
  -> 写入 Target.IncomingDamage
  -> AttributeSet 在 PostGameplayEffectExecute 中扣 Health
  -> 触发 GameplayCue.Skill.Fireball.Impact
  -> 投射物销毁，Ability 结束或清理
```

这条链路里，不同信息的归属如下：

| 信息 | 应该放在哪里 |
| --- | --- |
| 技能是谁 | `UAuraSkillDefinition.SkillTag` |
| 玩家按哪个键 | `InputTag.*` |
| 能不能释放 | PolicyConfig + GAS Cost/Cooldown + Owner/Target Tags |
| 释放过程怎么走 | `UAuraSkillTemplate` |
| 生成什么投射物 | `UAuraCombatEntityDefinition` 资产引用 |
| 投射物命中谁 | CombatEntity 的检测规则 |
| 基础伤害多少 | 技能配置、GE Magnitude、技能等级曲线；必要时使用 SetByCaller |
| 走物理还是法术公式 | `Damage.Channel.*` |
| 是火焰还是冰霜 | `Damage.Type.*`，可选 |
| 最终扣血 | AttributeSet + ExecCalc |
| 播什么特效音效 | `GameplayCue.*` |

## 9. State / Effect / Outcome 的边界

### 9.1 State

`State.*` 表示角色或 ASC 当前拥有的玩法状态，能被其他技能查询。

示例：

```text
State.Skill.Channeling
State.Control.Stunned
State.Control.Silenced
State.Dead
```

这些 Tag 可以由 GE 授予，也可以由运行时代码短暂授予。但如果需要多人同步，优先通过 GE 或可复制方式处理。

### 9.2 Effect

`Effect.*` 表示可复用效果语义，第一版先做概念整理，不急着强行资产化。

示例：

```text
Effect.Damage.Instant
Effect.Heal.Instant
Effect.Heal.Periodic
Effect.State.Stun
```

实际修改属性仍由 GE 完成。

### 9.3 Outcome

`Outcome.*` 表示一次技能执行的局部结果。

它不是 State，不应该成为角色身上的长期 Tag。

生命涌动示例：

```text
按满 2 秒 -> Outcome.Channel.Full -> Apply GE_Heal_Instant
提前松开 -> Outcome.Channel.Partial -> Apply GE_Heal_Periodic
移动取消 -> Outcome.Channel.Cancelled -> 不治疗
```

## 10. GameplayCue 边界

`GameplayCue.*` 只做表现，不改变玩法结果。

可以做：

- Niagara。
- 音效。
- 材质闪烁。
- 镜头震动。
- 飘字。

不应该做：

- 扣血。
- 回血。
- 添加 Buff。
- 决定命中。
- 决定最终伤害。

命名建议：

| 用途 | 示例 |
| --- | --- |
| 技能施法表现 | `GameplayCue.Skill.Fireball.Cast` |
| 技能命中表现 | `GameplayCue.Skill.Fireball.Impact` |
| 持续区域表现 | `GameplayCue.Skill.FireField.Loop` |
| 通用受击表现 | `GameplayCue.Hit.Default` |

V1 可以先用技能级 Cue，后续当表现复用变多，再抽成效果级 Cue。

## 11. 对当前 GameplayTags 的迁移建议

当前 `Config/DefaultGameplayTags.ini` 已经有：

```text
Ability.Fire.Fireball
Ability.Vital.VitalSurge
Event.Montage.FireBolt
Outcome.Channel.Full
Outcome.Channel.Partial
Outcome.Channel.Cancelled
SetByCaller.Damage
Damage.Magical
```

建议后续稳定玩法 Tag 迁移为：

```text
Ability.Fire.Fireball
Ability.Vital.VitalSurge
Event.Montage.Fire
Outcome.Channel.Full
Outcome.Channel.Partial
Outcome.Channel.Cancelled
Damage.Channel.Magical
Damage.Channel.Physical
Damage.Channel.True
Damage.Type.Fire
State.Skill.Channeling
```

如现有 GE 需要动态数值，可保留少量 GAS 技术键：

```text
SetByCaller.Damage
SetByCaller.Healing
SetByCaller.HealingPerTick
```

注意：迁移时不要一口气删除旧 Tag。先确认资产引用是否仍然存在，再逐步替换。

## 12. 阶段 A 确认结果

用户已确认：

1. V1 采用 `Damage.Channel.Physical / Magical / True` 作为长期公式通道。
2. 火球术允许保留 `Damage.Type.Fire` 作为语义预留，但当前公式不强依赖它。
3. `CombatEntity.*` 作为可选分类 Tag，不强制每个衍生物都有具体身份 Tag。
4. `Outcome.Channel.*` 保留为模板级结果，不做 per-skill Outcome Tag。
5. 属性 V1 不做概率属性，目标清单为 Health、Mana、PhysicalPower、MagicalPower、Armor、MagicResistance、PhysicalPenetration、MagicPenetration、IncomingDamage、IncomingHealing。

额外修订：

- Tag 应当是稳定和长远的设计语义。
- 临时实现细节优先用 enum、结构体字段、配置字段或普通变量。
- `SetByCaller.*` 只作为 GAS 技术数值键，不归入稳定玩法 Tag 命名域。
- 当前阶段不做复杂属性计算，先继续技能实现和简单伤害表现。

## 13. 下一阶段建议

### 阶段 B：给 ClaudeCode 写实施 Prompt

确认后，再让 ClaudeCode 做基础代码重构 Prompt，而不是继续写具体技能。

建议实施顺序：

1. 整理稳定 GameplayTag 命名，只新增确定长期需要的玩法 Tag。
2. 不把 `SetByCaller.*` 当成玩法 Tag 扩张；如果现有 GE 需要动态数值，只保留少量通用技术键。
3. 暂不做完整属性公式和复杂 ExecCalc 重构。
4. 保留当前能跑的简单伤害路径，确保技能命中、扣血、同步、表现可验证。
5. 如需补 `IncomingHealing`，只做治疗闭环所需的最小实现。
6. 更新中文说明文档和迁移报告。

### 阶段 C：再回到火球术

基础战斗语言稳定后，再用新结构重做火球术：

- `UAuraSkillDefinition`
- `UAuraSkillTemplate_ProjectileCast`
- `UAuraCombatEntityDefinition`
- `Damage.Channel.Magical`
- 基础伤害配置，必要时使用 `SetByCaller.Damage`
- 可选 `Damage.Type.Fire`

这样火球术不再只是一个临时原型，而是验证新基础框架的第一个样本。

## 14. 本文当前结论

Tag 不是越多越好。我们的原则是：

- 技能身份、输入、状态、事件、伤害通道、Cue 需要稳定玩法 Tag。
- `SetByCaller.*` 是 GAS 技术数值键，只在确实需要动态 GE 数值时少量使用。
- 战斗衍生物具体身份不一定需要 Tag，优先由资产表达。
- Outcome 只做模板级少量分支结果，不做每个技能的临时结果 Tag。
- Damage Channel 和 Damage Type 要分开：前者服务公式，后者服务语义。
- 属性系统 V1 先简化，不做暴击、格挡、闪避等概率属性；当前实现先保持简单伤害表现。

这一步做完，后续技能模板、战斗衍生物、GE、Cue 才有共同语言。
