# Apex 继承自 Aura 的技能系统上下文

生成日期：2026-07-14
来源：Aura `Agent/02_SkillSystem/` 多轮讨论与实现复盘。
状态：用于 Apex 后续 RFC，不代表当前已经实施。第 2-3 节是 Aura 阶段的历史提案，已由 `Agent/Research_Notes/2026-07-15_Skill_Framework_Extensibility_Synthesis.md` 校准；最新口径不追求一个万能配置或一个万能 RuntimeAbility。

## 1. 顶层方向

Apex 继续继承 Aura 阶段确认过的长期方向：

```text
少量 C++ 运行时能力 + 数据配置资产 + 原生机制模板
```

但 Apex 不直接迁移 Aura 代码。Aura 只作为经验库，Apex 会重新命名、重新设计模块、重新写 C++ 基础。

## 2. 技能配置不是万能字段表

技能根配置资产的定位是“技能组装入口”，不是承载所有特殊逻辑的巨大 DA。

根技能资产应该回答：

- 技能是谁。
- 用哪个输入触发。
- 用哪个 RuntimeAbility 承接 GAS 生命周期。
- 使用哪个 SkillTemplate。
- 引用哪些 CombatEntity、Effect/State、Montage、GameplayCue、UI 资源。
- 被哪些状态、资源、冷却、Tag 关系限制。

它不应该因为每出现一个特殊技能，就在顶层继续加临时字段。

## 3. RuntimeAbility 与 SkillTemplate 的边界（历史提案，待 RFC 重写）

Aura 阶段的建议曾是：

```text
RuntimeAbility = GAS 生命周期入口
SkillTemplate = 同类技能流程定义
```

RuntimeAbility 负责：

- `CanActivateAbility` / `CommitAbility` / `EndAbility`。
- 从 AbilitySpec / SourceObject 或未来 AbilitySet 中拿到技能配置。
- 创建一次技能执行上下文。
- 把具体流程委托给 SkillTemplate。

SkillTemplate 负责：

- 投射物施法。
- 引导施法。
- 区域场。
- 召唤 / 陷阱。
- 连招 / 近战窗口。
- 后续更特殊的技能流程。

普通技能优先“选择模板 + 填模板参数”，不要求编辑者手写一串 Steps。

Apex 当前修订方向：

- `UApexGameplayAbility` 作为公共 GAS 基类。
- Projectile / Channel / Area 等流程模板优先考虑 GA 子类。
- AbilityTask 负责可复用异步操作。
- 特殊技能允许专用 GA、Task 或 CombatEntity 行为。
- 若后续确需普通流程协调器，可以抽出，但不自造第二套 Ability 生命周期。

## 4. 战斗衍生物 CombatEntity

战斗衍生物指非角色本体的战斗载体：

- 投射物。
- 法术场。
- 召唤物。
- 陷阱。
- 光环区域。

它应该独立配置，因为它有自己的生成、移动、检测、命中、周期、结束和清理逻辑。

重要边界：

```text
技能释放目标 != 衍生物检测目标
```

例如伤害光环：技能释放目标是 Self；光环生成后检测目标是周围敌人。

## 5. Effect / State / Buff 分层

Aura 阶段确认的长期语义：

| 层 | 含义 |
| --- | --- |
| State | 原子玩法状态，通常体现为 GameplayTag，例如不能移动、沉默、眩晕。 |
| Effect | 可复用效果模板，例如瞬间伤害、周期治疗、减速、眩晕。 |
| Buff | 某个技能使用的具体效果实例，后置实现。 |

Apex V1 可以先做到 Effect + State，不急着做完整 BuffDefinition 层。

## 6. GameplayTag 粒度原则

Tag 应该是稳定、长期、跨系统可查询的语义。不要为了每个特殊技能临时造一堆 Tag。

适合做 Tag：

- 技能身份：`Ability.*`
- 输入：`InputTag.*`
- 角色状态：`State.*`
- Montage/技能关键事件：`Event.*`
- 伤害通道：`Damage.Channel.*`
- 表现：`GameplayCue.*`

不适合做 Tag：

- 只在一个函数内部用一次的临时分支。
- 可以用 enum / bool / 结构体字段表达的局部结果。
- 每个技能专属的 SetByCaller key。

`SetByCaller.*` 是 GAS 技术数值键，不是玩法语义 Tag。只有 GE 确实需要动态数值时才少量使用。

## 7. 属性和伤害第一版口径

Apex 后续可采用更简洁的属性集：

- `Health` / `MaxHealth`
- `Mana` / `MaxMana`
- `PhysicalPower`
- `MagicalPower`
- `Armor`
- `MagicResistance`
- `PhysicalPenetration`
- `MagicPenetration`
- `IncomingDamage`
- `IncomingHealing`

当前阶段不急着做复杂公式。技能实现早期可以先保证简单伤害表现和多人正确性，后续再引入正式物理/法术通道公式。

## 8. Montage / GameplayEvent / Timeline

V1 仍然可以使用：

```text
Montage Notify -> Send GameplayEvent -> AbilityTask_WaitGameplayEvent
```

职责边界：

- Montage Notify 负责“动画哪一帧发事件”。
- SkillDefinition / SkillTemplate 负责“收到事件后做什么”。
- 普通音效、脚步、纯 VFX 用普通 Notify。
- 驱动 GA 逻辑的关键帧用 GameplayEvent Notify。

长期 Skill Timeline 负责玩法时序；Montage 负责动画表现。等 2-3 个技能原型稳定后，再考虑最小 Timeline。

## 9. GameplayCue 边界

GameplayCue 只做表现：Niagara、音效、材质、镜头、UI、飘字。

GameplayCue 不决定：

- 是否命中。
- 伤害多少。
- 是否添加 Buff。
- 是否改变属性。

玩法结果必须由 GAS / 服务器权威路径产生。

## 10. 代表性技能池

Aura 阶段确认过的代表性技能池仍适合作为 Apex 后续技能设计参考，但不再硬绑定旧 Aura 资产：

1. 火球术 / 能量弹：投射物 + 命中伤害。
2. 引导治疗：按住、松开、取消、不同结果。
3. 范围持续场：地面区域 + 周期效果。
4. 蓄力射击：蓄力时间影响输出。
5. 近战连招：Montage Section + 命中窗口。
6. 被动反击 / 护盾：监听受击事件、防递归。

Apex 当前会结合 Paragon 资产和 Montage 重新设计具体技能，不照搬 Aura 旧技能名。

## 11. 后续 RFC 要求

正式进入 GAS / 技能系统实现前，需要重新写 Apex 专用 RFC，至少确认：

- 模块与目录边界。
- 核心类命名。
- GameplayTag 命名表。
- Attribute 最小集合。
- ASC 初始化路径。
- Ability 授予方式。
- SkillDefinition / SkillTemplate / CombatEntity / Effect / State 的资产与 C++ 形态。
- 第一批技能原型与 Paragon 资产关系。

这些命名需要用户审阅后再实施。
