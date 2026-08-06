# Aura 技能系统架构复盘与重构总结

创建日期：2026-07-09  
状态：RFC 草案，用于继续讨论和指导后续 ClaudeCode 实施  
范围：技能运行时、技能配置资产、Buff/Effect/Status、战斗衍生物、技能编辑器长期方向

## 1. 为什么现在需要复盘

火球术和生命涌动两个原型已经证明了当前路线可以跑通：

- 技能可以通过 `UAuraSkillDefinition` 配置。
- 通用 GA 可以从 `AbilitySpec.SourceObject` 取回具体技能配置。
- 火球术可以完成输入、蒙太奇事件、投射物、伤害。
- 生命涌动可以完成按住、松开、移动取消、完整引导与非完整引导的不同治疗。

但是这两个原型也暴露出一个更重要的问题：如果继续把所有差异都塞进同一个 `UAuraSkillDefinition` 和同一个 `UAuraConfiguredActiveAbility`，后面很快会变成“万能配置表 + 巨型 GA”。这种结构短期能加字段，长期会很难解释、很难编辑、很难扩展，也不利于未来技能编辑器。

因此，当前结论不是推翻原型，而是把原型验证出的东西升级为更清晰的分层架构。

## 2. 当前核心结论

### 2.1 技能配置不是“所有逻辑字段化”

`UAuraSkillDefinition` 的定位应当是“技能组装入口”，不是一个字段包打天下。

它应该回答：

- 这个技能是谁。
- 用哪个输入触发。
- 用哪个运行时 GA 执行。
- 这个技能属于哪类技能模板。
- 需要哪些战斗衍生物、Buff、Effect、表现资源。
- 受到哪些状态、标签、冷却、资源消耗限制。

它不应该把每一种技能的所有特殊逻辑都变成顶层字段。否则后续只要出现一个新技能，就会继续添加临时字段，最终难以维护。

### 2.2 优先采用“模板优先”，不是“裸 Steps 优先”

我们可以在讨论时把技能拆成步骤，例如：

```text
PlayMontage -> WaitGameplayEvent -> ResolveTarget -> SpawnProjectile -> ApplyEffect -> End
```

但正式设计上，普通技能不应该要求每次手填一串 Steps。

更合理的是：

- 常见技能使用明确的技能模板，例如投射物施法、引导施法、范围场、召唤、近战连招。
- 模板内部可以隐藏或生成一组默认步骤。
- 特殊技能如果超出模板，再通过高级 Step、AbilityTask 或 C++ 子类扩展。

也就是说，Steps 是底层执行语言或高级扩展点，不是 V1 的主要编辑入口。

补充决定：项目冷启动初期不要求手动填写 Steps。V1 的普通使用方式是“选择模板 + 填模板参数”。例如 ProjectileCast 模板内部固定包含播放蒙太奇、等待发射事件、解析目标、生成投射物、结束技能等内部步骤；用户只需要填写蒙太奇、事件 Tag、目标规则、投射物/战斗衍生物配置。Step 可以在代码里表现为函数、AbilityTask、UObject 策略对象或未来编辑器节点，但 V1 不把它暴露成必须手工编排的流程表。

### 2.3 少量通用 GA 仍然成立，但命名和职责要调整

`UAuraConfiguredActiveAbility` 这个名字已经不够清晰，因为未来几乎所有技能都会“可配置”。建议长期改名为：

```cpp
UAuraSkillRuntimeAbility
```

它的职责不是代表某一类玩法，而是作为 GAS 生命周期入口：

- 接收输入激活。
- 执行 GAS 的 `CanActivateAbility` / `CommitAbility` / `EndAbility`。
- 从 `AbilitySpec.SourceObject` 读取 `UAuraSkillDefinition`。
- 创建技能运行时上下文。
- 把具体流程委托给 `UAuraSkillTemplate`。

换句话说，GA 是 GAS 执行器；技能模板才描述“这类技能怎么执行”。

补充决定：V1 的主动技能可以先只有一个通用运行时 GA，即 `UAuraSkillRuntimeAbility`。不同技能的主要差异由 `UAuraSkillTemplate`、战斗衍生物、Effect/State、目标规则和表现配置承担。只有当某类技能需要完全不同的 GAS 激活策略、预测策略、被动监听生命周期或移动网络预测时，才新增其他 Runtime GA。

### 2.4 多配置层，而不是一个 DA 万能

长期结构应当接近下面这种分层：

```text
UAuraSkillDefinition
  - Identity / UI / Input
  - PresentationConfig
  - PolicyConfig
  - Instanced SkillTemplate
  - Buff / Entity / Cue / TargetRule references

UAuraSkillTemplate_ProjectileCast
UAuraSkillTemplate_ChannelCast
UAuraSkillTemplate_AreaField
UAuraSkillTemplate_Summon
UAuraSkillTemplate_Combo

UAuraCombatEntityDefinition
  - Projectile / Field / Summon / Trap lifecycle
  - Detection rules
  - OnSpawn / OnHit / OnTick / OnEnd effects

Later UAuraBuffDefinition
  - Optional future layer for concrete skill-specific buff/effect instance

UAuraEffectDefinition
  - Reusable effect template

Status tags / Status definition
  - Atomic state, e.g. State.CannotMove, State.Silenced
```

注意：多配置层不等于每个技能都必须新建一堆资产。

原则是：

- 每个技能通常有一个根 `UAuraSkillDefinition`。
- 技能模板参数优先内联在根资产里，例如 `Instanced UObject`。
- 只有可复用、独立生命周期、或内容复杂的东西，才拆成单独资产。
- 战斗衍生物适合单独资产，因为它有自己的生命周期、检测、命中、结束逻辑。
- Buff/Effect 可以先轻量实现，等复用需求变强再进一步资产化。

## 3. 建议的配置层职责

### 3.1 `UAuraSkillDefinition`

根技能资产，未来技能编辑器最主要编辑对象。

建议包含：

| 分组 | 作用 | 示例 |
| --- | --- | --- |
| Identity | 技能身份和编辑器显示 | SkillId、DisplayName、Description、Icon、SkillTags |
| Input | 输入绑定 | InputTag、ActivationInputMode |
| Runtime | GAS 执行入口 | RuntimeAbilityClass，默认可为 `UAuraSkillRuntimeAbility` |
| PresentationConfig | 通用表现、镜头、预览 | 准星、镜头震动、公共 Cue、预览资源 |
| PolicyConfig | 技能关系、限制、取消、阻塞 | RequiredTags、BlockedTags、CancelTags、CooldownGroup |
| SkillTemplate | 技能类型模板 | ProjectileCast、ChannelCast、AreaField |
| References | 引用其他配置 | CombatEntityDefinition、Effect/GE、Cue、Montage |
| Preview | 编辑器预览所需信息 | PreviewPawn、PreviewTarget、PreviewMap |

它负责“组装技能”，不负责承载所有特殊逻辑。

补充决定：`BaseConfig` 这个词容易和 `Identity` 重合，后续不作为独立核心概念推进。技能根资产中保留 Identity / Input / Runtime 这些基础字段；镜头、准星、公共 Cue、预览等内容收拢到更明确的 `PresentationConfig` 或直接作为表现分组字段。

### 3.2 `UAuraSkillTemplate`

技能模板是“同类技能的流程定义”。

建议使用 C++ `UObject`，并以内联实例的方式放在 `UAuraSkillDefinition` 里：

```cpp
UPROPERTY(EditDefaultsOnly, Instanced)
TObjectPtr<UAuraSkillTemplate> SkillTemplate;
```

模板类示例：

| 模板 | 适合的技能 | 典型流程 |
| --- | --- | --- |
| `UAuraSkillTemplate_ProjectileCast` | 火球、箭、飞弹、锁定弹 | 播放施法表现，等待发射事件，解析目标，生成投射物 |
| `UAuraSkillTemplate_ChannelCast` | 治疗引导、蓄力、持续施法 | 等待输入释放/时间完成/取消，按结果分支应用效果 |
| `UAuraSkillTemplate_AreaField` | 火焰地面、毒圈、伤害光环 | 解析释放位置，生成范围场衍生物 |
| `UAuraSkillTemplate_Summon` | 召唤物、陷阱、图腾 | 解析位置，生成有生命周期的衍生物 |
| `UAuraSkillTemplate_Combo` | 三段连招、近战窗口 | 管理连段、输入窗口、命中检测窗口 |

模板内部可以使用 AbilityTask，例如 `PlayMontageAndWait`、`WaitGameplayEvent`、`WaitDelay`、自定义 `WaitInputRelease`。但 AbilityTask 是实现工具，不是给普通技能编辑者直接堆叠的主配置。

### 3.3 `FAuraSkillPolicyConfig`

PolicyConfig 负责“这个技能什么时候能用、会阻塞谁、会被谁阻塞”。

它不直接造成伤害或治疗，而是查询状态和关系。

建议字段：

| 字段 | 作用 |
| --- | --- |
| RequiredOwnerTags | 释放者必须拥有的标签 |
| BlockedByOwnerTags | 释放者拥有这些标签时不能释放 |
| RequiredTargetTags | 目标必须拥有的标签 |
| BlockedTargetTags | 目标拥有这些标签时不能作为目标 |
| ActivationOwnedTags | 技能激活期间授予释放者的状态 |
| CancelAbilitiesWithTags | 激活时取消哪些技能 |
| BlockAbilitiesWithTags | 激活期间阻止哪些技能 |
| CooldownGroup | 共享冷却组 |
| InterruptPriority | 打断优先级，后续用于引导、连招、施法打断 |

示例：

- 沉默 Buff 给角色授予 `State.Silenced`。
- 火球术的 `BlockedByOwnerTags` 包含 `State.Silenced`。
- 生命涌动引导期间给自己授予 `State.Channeling`。
- 移动输入检测到 `State.Channeling` 后取消引导技能。

### 3.4 `UAuraCombatEntityDefinition`

战斗衍生物指非角色本体的战斗载体，例如：

- 投射物。
- 法术场。
- 召唤物。
- 陷阱。
- 光环区域。

它应该单独配置，因为它们经常有自己的生命周期和目标检测规则。

建议职责：

| 分组 | 内容 |
| --- | --- |
| Spawn | 生成类、生成位置、附着规则、初速度 |
| Lifetime | 持续时间、命中后销毁、到期销毁 |
| Movement | 投射速度、追踪、重力、弹道 |
| Detection | 检测形状、半径、碰撞通道、阵营过滤、目标数量 |
| OnSpawn | 生成时执行的 Cue / Buff / 状态 |
| OnHit | 命中时应用的 Buff / GE / Cue / 派生物 |
| OnTick | 周期检测和周期效果 |
| OnEnd | 结束时爆炸、清理 Cue、移除状态 |

这能解决“技能目标”和“衍生物目标”混在一起的问题。

例如伤害光环：

- 技能释放目标是 Self，表示生成在自己身上。
- 光环衍生物的检测目标是周围敌人。

例如火球留下火焰地面：

- 火球技能模板只负责生成投射物。
- 投射物衍生物命中后，应用命中伤害，并生成一个火焰场衍生物。
- 火焰场衍生物自己负责持续时间、范围检测、周期伤害。

### 3.5 Effect / State 第一版，Buff 层后置

这部分长期要从 GAS 的 GE 之上整理出更适合技能编辑器的语义。

建议含义：

| 层 | 含义 | 示例 |
| --- | --- | --- |
| Status | 最原子的玩法状态，通常体现为 GameplayTag | `State.CannotMove`、`State.Silenced`、`State.Invulnerable` |
| Effect | 可复用的效果模板 | InstantHeal、HoT、InstantDamage、Slow、Stun |
| Buff | 某个技能使用的具体效果实例 | VitalSurgeFullHeal、VitalSurgePartialHoT、FireballImpactDamage |

GAS 中实际修改属性、授予标签、持续时间、周期执行，仍然主要由 GE 完成。

长期可以演进为：

```text
SkillTemplate / CombatEntity
  -> Apply EffectDefinition or GE
    -> GE modifies Attribute / grants Status tags / triggers GameplayCue
  -> Later optional BuffDefinition can package skill-specific instances
```

V1 可以先保留直接引用 GE 的轻量做法，并把第一版 Buff 系统收敛为 Effect + State：Effect 表示可复用效果语义，State 表示原子玩法状态。完整 BuffDefinition 层后置。

## 4. Targeting 的边界

当前需要特别区分两种 Target：

### 4.1 技能释放目标

这是技能激活时需要知道的目标。

示例：

- Self：给自己加 Buff，或在自己身上生成光环。
- CursorPoint：火球朝鼠标点发射。
- GroundPoint：在地面目标点生成法术场。
- SelectedActor：对锁定目标释放。
- Direction：朝输入方向发射或冲刺。

这部分属于 `SkillDefinition` 或 `SkillTemplate`。

### 4.2 战斗衍生物检测目标

这是投射物、法术场、陷阱、召唤物在生成之后自己检测到的目标。

示例：

- 投射物命中的敌人。
- 火焰地面每秒检测到的敌人。
- 陷阱范围内进入的敌人。
- 召唤物 AI 选择攻击的敌人。

这部分属于 `CombatEntityDefinition`。

### 4.3 TargetRule 不应该只是 Tag

TargetRule 应该是结构化规则，Tag 只是其中一个过滤条件。

建议包含：

- Source：从谁开始检测，Caster / Entity / TargetPoint。
- Shape：Point / Line / Sphere / Box / Cone。
- Range / Radius / Angle。
- TeamFilter：Self / Friendly / Enemy / Neutral。
- MaxTargets。
- SortRule：Nearest / LowestHealth / Random / ByAngle。
- RequiredTags / BlockedTags。
- OutputSlot：输出到哪个目标槽，供后续 Buff 或 Entity 使用。

这样比单纯填 Tag 更清晰，也更适合编辑器可视化。

## 5. Montage、GameplayEvent 与长期 Skill Timeline

### 5.1 V1 继续使用 Montage Notify 触发 GameplayEvent

当前 V1 仍然采用：

```text
Montage Notify -> SendGameplayEventToActor -> AbilityTask_WaitGameplayEvent
```

原因：

- UE/GAS 对这个路径支持成熟。
- 可以在动画轨道上精确指定发射帧、命中帧。
- 现有火球术已经验证可行。

约定：

- 普通音效、脚步、纯表现可以继续用普通 AnimNotify。
- 会驱动 GA 逻辑的关键帧使用 GameplayEvent Notify。
- 技能配置中保留要等待的 EventTag，因为一个 Montage 可能有多个关键事件。

### 5.2 长期需要 Skill Timeline

Montage 的职责是动画表现，不应该长期承担所有技能逻辑时序。

长期 Skill Timeline 的职责：

- 技能阶段。
- 发射点。
- 命中窗口。
- 可取消窗口。
- 多段伤害时间点。
- Cue 时间点。
- 编辑器预览播放头。

Montage 仍然负责动画、骨骼、混合、动画通知。

长期方向：

```text
Skill Timeline = 玩法逻辑时序
Montage = 动画表现轨道
GameplayEvent = V1 阶段连接二者的桥
```

建议等 2-3 个技能原型稳定后，再开始实现最小 Skill Timeline。

## 6. GameplayCue 的边界

项目约定：GameplayCue 只做表现，不改变玩法状态。

可以做：

- Niagara。
- 音效。
- 材质变化。
- 镜头震动。
- UI 飘字。
- 命中反馈。

不应该做：

- 修改属性。
- 添加/移除 Buff。
- 授予或移除玩法状态。
- 决定是否命中。
- 决定伤害数值。

原因是多人同步和权威性必须由 GAS/服务器路径保证。Cue 可以被客户端看到，但不应该成为玩法结果来源。

## 7. Tag 命名与粒度原则

Tag 的核心原则：不要为了每个特殊技能临时造一堆只服务一次的状态 Tag。

建议命名域：

| 命名域 | 用途 | 示例 |
| --- | --- | --- |
| `Ability.*` | 技能身份、技能类别 | `Ability.Fire.Fireball` |
| `InputTag.*` | 输入绑定 | `InputTag.LMB`、`InputTag.1` |
| `State.*` | 持久或半持久玩法状态 | `State.Channeling`、`State.Silenced` |
| `Event.Montage.*` | 蒙太奇关键帧事件 | `Event.Montage.FireBolt` |
| `Event.Skill.*` | 长期技能 Timeline 事件 | `Event.Skill.Projectile.Fire` |
| `Outcome.*` | 单次技能执行的局部结果 | `Outcome.Channel.Full`、`Outcome.Channel.Partial` |
| `GameplayCue.*` | 表现事件 | `GameplayCue.Skill.Fireball.Impact` |
| `SetByCaller.*` | 运行时注入 GE 的数值键 | `SetByCaller.Damage`、`SetByCaller.Healing`、`SetByCaller.Duration` |
| `Damage.*` | 伤害类型 | `Damage.Physical`、`Damage.Magical` |

`Outcome.*` 不是角色状态，不应该长期挂在 ASC 上。它更像一次技能执行上下文里的分支结果。

补充说明：SetByCaller 不是“根据调用者自身属性自动计算”。它是 GAS 在创建 `GameplayEffectSpec` 时临时塞入数值的机制，GameplayTag 在这里是数值键，例如 `SetByCaller.Damage` 表示“这份 GE Spec 里有一个伤害数值”。这个数值可以来自技能配置、技能等级、蓄力时长，也可以先读取释放者属性后计算出来。释放者/目标属性参与最终公式时，更适合通过 Attribute Capture 或 ExecutionCalculation 处理。

属性与伤害类型的长期方向需要简化：当前 Aura 教程属性较多，后续重构时优先收敛到血量、蓝量、物理伤害、法术伤害、防御力、法术抗性、物理穿透、法术穿透、闪避率等核心属性。伤害类型第一版优先只区分 `Damage.Physical` 和 `Damage.Magical`，元素火焰/冰霜等可以等战斗模型稳定后再作为子类型扩展。

例如生命涌动：

- 完整引导产生 `Outcome.Channel.Full`。
- 提前释放产生 `Outcome.Channel.Partial`。
- 移动取消产生 `Outcome.Channel.Cancelled`。
- 后续根据 Outcome 选择应用瞬间治疗或持续治疗。

## 8. 用两个现有技能推演新结构

### 8.1 火球术

根技能：

```text
DA_Skill_Fireball
  RuntimeAbilityClass = UAuraSkillRuntimeAbility
  InputTag = InputTag.LMB
  PolicyConfig.BlockedByOwnerTags = State.Silenced, State.Stunned
  SkillTemplate = UAuraSkillTemplate_ProjectileCast
```

模板内联配置：

```text
ProjectileCastTemplate
  CastMontage = AM_Cast_FireBolt
  FireEventTag = Event.Montage.FireBolt
  CastTargetRule = CursorHitPoint
  ProjectileEntity = DA_Entity_FireballProjectile
```

投射物衍生物：

```text
DA_Entity_FireballProjectile
  Movement = ForwardProjectile
  Detection = Hit enemy actor
  OnHit:
    Apply Buff.FireballImpactDamage
    Execute GameplayCue.Skill.Fireball.Impact
    Destroy self
```

如果火球命中后留下火焰地面：

```text
DA_Entity_FireballProjectile.OnHit:
  Apply Buff.FireballImpactDamage
  Spawn DA_Entity_BurningField

DA_Entity_BurningField:
  Lifetime = 5s
  Detection = Sphere enemy targets every 1s
  OnTickDetected = Apply Buff.BurningFieldPeriodicDamage
  OnEnd = Remove persistent cue
```

### 8.2 生命涌动

根技能：

```text
DA_Skill_VitalSurge
  RuntimeAbilityClass = UAuraSkillRuntimeAbility
  InputTag = InputTag.2
  PolicyConfig.BlockedByOwnerTags = State.Stunned, State.Silenced, State.Dead
  SkillTemplate = UAuraSkillTemplate_ChannelCast
```

模板内联配置：

```text
ChannelCastTemplate
  TargetRule = Self
  FullDuration = 2.0
  CancelOnMoveInput = true
  FullOutcome = Outcome.Channel.Full
  PartialOutcome = Outcome.Channel.Partial
  CancelledOutcome = Outcome.Channel.Cancelled
  FullBuff = Buff.VitalSurgeFullHeal
  PartialBuff = Buff.VitalSurgePartialHoT
```

执行结果：

```text
按住达到 2 秒:
  Outcome.Channel.Full
  Apply Buff.VitalSurgeFullHeal
  瞬间治疗

提前松开:
  Outcome.Channel.Partial
  Apply Buff.VitalSurgePartialHoT
  回复一半总量，但以持续恢复方式生效

尝试移动:
  Outcome.Channel.Cancelled
  不治疗，结束技能
```

这说明“是否完整引导”不是写死在某个治疗技能里，而是 `ChannelCastTemplate` 的通用机制。具体应用哪个 Buff，是这个技能的配置。

## 9. 对当前代码的影响

这次复盘意味着后续可能需要改动不少地方，但不要求一次性改完。

### 9.1 需要重建或重命名

| 当前对象 | 问题 | 长期方向 |
| --- | --- | --- |
| `UAuraConfiguredActiveAbility` | 命名含糊，职责逐渐变大 | 以 `UAuraSkillRuntimeAbility` 作为新的运行时 GA 名称 |
| `UAuraSkillDefinition` | 容易继续膨胀成万能 DA | 拆成 Identity / Input / PresentationConfig / PolicyConfig / SkillTemplate / References |
| 当前 EffectEntries | 能支持原型，但长期语义不够清楚 | 第一版过渡到 Effect / State / GE，完整 BuffDefinition 后置 |
| 投射物字段 | 如果继续放在技能 DA 顶层，会混合技能目标和衍生物目标 | 移入 `UAuraCombatEntityDefinition` |
| 引导逻辑 | 已经能跑，但应从治疗特例上升为 Channel 模板 | `UAuraSkillTemplate_ChannelCast` |
| Montage Event 字段 | V1 必要，但长期不是唯一时序来源 | 保留 V1，预留 Skill Timeline |

### 9.2 可以暂时保留

| 当前对象 | 保留理由 |
| --- | --- |
| `AbilitySpec.SourceObject` 主路径 | 已验证，是配置技能连接 GAS Spec 的关键 |
| 旧教程 GA 路径 | 可作为对照和资产验证，不急于删除 |
| 火球术和生命涌动原型 | 可作为行为参考样本，但不强制保留旧配置结构 |
| Montage Notify -> GameplayEvent | V1 仍然是最稳的关键帧桥接方式 |
| 直接 GE 引用 | V1 可以作为 Effect / State 整理完成前的过渡 |

## 10. 推荐的基础架构搭建顺序

### 阶段 0：冻结口径

目标：

- 写完本总结。
- 用户确认大方向。
- 再写给 ClaudeCode 的实施 Prompt。

不改代码。

### 阶段 1：搭建运行时基础壳

目标：

- 建立 `UAuraSkillRuntimeAbility` 作为新的主动技能运行时 GA。
- 引入 `UAuraSkillTemplate` 抽象基类。
- 引入 `UAuraSkillTemplate_ProjectileCast` 和 `UAuraSkillTemplate_ChannelCast` 的最小壳。
- 不以兼容旧配置为主要目标；旧火球术和生命涌动只作为行为参考。

重点：

- 先让结构可编译。
- 不一次性追求所有旧字段复刻。

### 阶段 2：用新架构重新设计火球术

目标：

- 重新创建或重写火球术根 DA，使其只保留技能身份、输入、策略、模板引用等必要信息。
- 投射物相关参数进入 ProjectileCastTemplate 或 CombatEntityDefinition。
- 行为目标参考当前火球术：输入触发、蒙太奇关键帧发射、投射物命中、应用伤害。

### 阶段 3：用新架构重新设计生命涌动

目标：

- 将生命涌动作为 ChannelCastTemplate 的第一个验证技能。
- Full / Partial / Cancelled 通过 Outcome 进入分支。
- Full 效果和 Partial 效果由 Effect/GE 配置决定，暂不引入完整 BuffDefinition。

### 阶段 4：引入最小 CombatEntityDefinition

目标：

- 先支持 Projectile 和 Field。
- 为下一个范围场技能做准备。
- 解决“技能目标”和“衍生物检测目标”分离。

### 阶段 5：整理 Effect / State

目标：

- 第一版暂不引入完整 BuffDefinition。
- 先整理 Effect 和 State：Effect 表示可复用效果语义，State 表示原子玩法状态。
- 明确哪些 GE 直接表达 Effect，哪些 GE 授予或移除 State Tag。
- 建立 Status Tag 命名规范。

### 阶段 6：准备技能编辑器前置能力

目标：

- 能列出 SkillDefinition。
- 能识别 SkillTemplate 类型并显示不同字段。
- 能预览 Montage / GameplayCue。
- 后续再做 Skill Timeline。

## 11. 给 ClaudeCode 的实施口径

在用户确认本 RFC 前，不应让 ClaudeCode 大规模改代码。

后续给 ClaudeCode 的 Prompt 应包含：

- 不要继续扩充 `UAuraSkillDefinition` 为万能字段表。
- 新增结构必须能重新实现火球术和生命涌动的目标行为。
- 架构搭建必须分阶段，每阶段可编译、可运行、可回退。
- 每个新增 C++ 类需要解释父类、职责、生命周期、为什么这样拆。
- 涉及 GAS 的地方必须写学习型注释，尤其是 Commit、AbilityTask、GE 应用、预测/权威边界。
- GameplayCue 只做表现，不做玩法状态。
- CombatEntity 的目标检测不等同于 SkillDefinition 的释放目标。
- 不要一次性实现完整 Skill Timeline、完整 Buff 系统或完整编辑器。

## 12. 本轮已确认事项

用户已确认以下实施方向，用于下一份 ClaudeCode Prompt：

1. 可以直接推进新的 `UAuraSkillRuntimeAbility`，不以保留 `UAuraConfiguredActiveAbility` 旧结构为目标；实施时以火球术和生命涌动的行为重新设计作为验证。
2. `UAuraSkillTemplate` 第一版直接采用 `Instanced UObject` 内联编辑，不先用 `USTRUCT` 过渡。
3. `UAuraCombatEntityDefinition` 第一版只覆盖 Projectile / Field，暂不覆盖 Summon / Trap。
4. Buff 系统第一版先只做到 Effect 和 State，不引入完整 BuffDefinition 层。
5. 火球术和生命涌动直接作为新架构下的重新设计样本优先实现。
6. 不符合新设计的旧实现、旧字段、旧配置化原型代码可以删除；删除时需要同步清理引用，并在实施报告中说明删除理由。
7. Content 目录下技能资产的长期目录结构可以采用类似：

```text
/Game/AbilitySystem/Skills/
/Game/AbilitySystem/SkillTemplates/
/Game/AbilitySystem/CombatEntities/
/Game/AbilitySystem/Buffs/
/Game/AbilitySystem/Effects/
/Game/AbilitySystem/GameplayCues/
```

## 13. 本文档的结论

我们仍然坚持“少量 C++ 运行时能力 + 数据配置资产”的大方向，但需要把“数据配置资产”从一个巨大 DA 升级为分层配置：

- `SkillDefinition` 是技能入口。
- `SkillTemplate` 是同类技能流程。
- `CombatEntityDefinition` 是投射物、法术场、召唤物、陷阱等战斗载体。
- 第一版先整理 `Effect / State`；完整 `BuffDefinition` 层后置。
- `GameplayCue` 是表现层。
- `Montage` 是 V1 关键帧桥接，长期会被 Skill Timeline 承担更多逻辑时序职责。

这样做的目的不是为了炫技，而是为了避免后续每新增一个技能就临时加字段、临时加分支、临时加 Tag。我们要让常见技能能通过模板快速配置，让特殊技能也有清晰的扩展位置。
