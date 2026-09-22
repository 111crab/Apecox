# Apecox 战斗框架继承基线

更新日期：2026-08-05
状态：旧项目技术思想的完整综合，不等于 Apecox 最终代码 RFC。

## 1. 顶层目标

Apecox 要构建的是一套易理解、可复用、可扩展、可验证的多人射击战斗框架。它必须同时容纳：

- 枪械武器：开火、连射、弹药、换弹、切枪、拾取、配件。
- 英雄技能：投射物、范围场、引导、蓄力、链接、被动监听等。
- 角色状态：Health、Shield、受控、死亡、复活和未来可追加属性。
- 第一人称本地表现和第三人称远端表现。
- 服务器权威、客户端预测、复制、回滚后的表现修正和网络调试。

“快速开发技能”是目标之一，但不以一个万能 GA、一个万能 DataAsset 或一套任意 Step 解释器为成功标准。

## 2. 总体职责分层

```text
Gameplay Framework
  负责比赛规则、玩家身份、Controller、PlayerState、Pawn 与重生

GAS Core
  负责 ASC、Ability、Effect、Attribute、Tag、Cue、Task 与预测键

Grant / Loadout
  负责给谁授予哪些能力、等级和输入槽，并可完整撤销

Ability / Weapon Runtime
  负责激活后的真实流程、异步等待、Commit、Cancel、End 和运行时状态

Definition / Config
  负责技能或武器的稳定数据组装，不复制运行时生命周期

Combat Entity
  负责投射物、法术场、陷阱、召唤物等生成后独立存在的战斗载体

Effects / State / Buff
  负责数值修改、持续规则、原子状态和组合实例

Presentation
  负责第一/第三人称 Mesh、动画、GameplayCue、Niagara、音效、镜头和 UI

Editor Tools
  只编辑和预览上述运行时数据，不重新发明一套游戏规则
```

每一层必须能回答“它负责什么”和“它不负责什么”。发现某个资产不断吸收其他层字段时，应先修正边界，而不是继续增加开关。

## 3. Gameplay Framework 与 ASC 所有权

### 继承原则

玩家 ASC 倾向放在 PlayerState：

```text
OwnerActor  = PlayerState
AvatarActor = 当前 Pawn / Character
```

这使能力、装备或长期状态可以跨 Pawn 死亡和重生存在，同时 Pawn 仍是移动、动画和碰撞的表现载体。

必须实现对称生命周期：

- 服务端 Possess 或 Pawn 就绪时绑定 ActorInfo。
- 客户端 `OnRep_PlayerState` 后绑定 ActorInfo。
- 换 Pawn、死亡或离开时取消不应存活的 Ability、清输入、清 Cue、解绑 Avatar。
- 不能只写 Init 而没有 Uninit。

### Apecox 仍需决定

- PlayerState、PawnExtension 或自建 AbilitySystemComponent 的最小边界。
- Respawn 时哪些 Attribute/GE/Ability 保留。
- 是否需要轻量 Hero/Pawn Definition 聚合资产。

### 当前明确不照搬

- Lyra 完整四级 InitState。
- Experience System。
- GameFeature 动态装卸。
- ModularGameplay ComponentManager 全套。

这些可以在出现真实动态装卸需求时重新评估。

## 4. AbilitySet、InputConfig 与角色/装备聚合

### AbilitySet 的准确定位

AbilitySet 是可撤销授予包，不是函数，也不是单技能定义。它保存类似：

- GameplayAbility 类、等级、InputTag。
- 初始 GameplayEffect。
- AttributeSet。
- 授予后产生的 SpecHandle、ActiveGEHandle 和 AttributeSet 指针，以便撤销。

可能用途：

- 英雄出生能力包。
- 装备一把武器时授予开火、瞄准、换弹 Ability。
- 临时玩法模式授予的能力。

### 输入三层

```text
IMC：物理按键 -> InputAction
InputConfig：InputAction -> InputTag
AbilitySet/Loadout：InputTag -> AbilitySpec
```

因此：

- Character 不保存每个技能的 IA 成员。
- InputTag 表示槽位/意图，不是具体技能身份。
- 同一技能可在不同英雄或模式中绑定不同槽位。
- ASC 在一帧末统一处理 Pressed/Held/Released，已激活 Ability 使用 GAS Generic Replicated Event 接收输入变化。

### 射击项目扩展

武器不应只被当作一个 SkillDefinition。更合理的候选边界是：

- WeaponDefinition：静态资产、武器族、动画/表现入口、弹药和基础参数。
- WeaponInstance：当前弹匣、散布、热量、配件、持有者等运行时状态。
- Equipment/Inventory：持有、装备、槽位、拾取和丢弃。
- AbilitySet：装备时授予开火、ADS、换弹等 GA。
- GameplayAbility：执行输入驱动、预测、TargetData 和 Commit。

这些名称仍需 Phase 1/3 RFC 审阅，原则先保留。

### 技能关系层

Lyra 的 TagRelationshipMapping 值得保留为独立思想：按 Ability 类别集中定义“阻塞、取消、额外要求和额外阻止”，避免每个 GA 重复维护眩晕、死亡、装填、排他施法等关系。

它和 GameplayEffect 不重合：

- GE/State 表达角色当前拥有什么状态。
- Relationship Policy 表达某类 Ability 遇到这些状态或其他 Ability 时如何响应。
- AbilitySet 只负责授予，不负责关系规则。

## 5. SkillDefinition 与多配置层

### 根结论

SkillDefinition 的定位是“每个技能的组装入口”，不是“所有技能逻辑字段化”。

建议的长期关系：

```text
SkillDefinition
  |- Identity / UI / 资源入口
  |- AbilityClass 或流程模板标识
  |- 一个类型化的 ExecutionConfig
  |- Policy/Relationship 入口
  |- CombatEntityDefinition 引用
  |- GameplayEffect / EffectDefinition 引用
  `- Presentation/Cue 入口
```

旧项目最终倾向“根资产 + 受约束的单个 FInstancedStruct”：

- 根资产提供统一入口。
- `ExecutionConfig` 只允许一个类型化配置，不是任意 Fragment 数组。
- 所有执行配置继承统一基结构。
- 每个 GA 流程模板声明接受哪种配置。
- Data Validation 和激活入口双重检查 AbilityClass/Config 类型匹配。
- 特殊衍生物行为不塞回施法配置。

这个结构借鉴了漫威争锋演讲中的三层思路，但不照抄名称：

| 演讲概念 | Apecox 候选映射 | 作用 |
| --- | --- | --- |
| AbilityAsset | SkillDefinition 的 Common/Identity/Presentation 入口 | 技能共性和资源组装 |
| AbilityConfig | 单个类型化 `FInstancedStruct ExecutionConfig` | 当前流程模板的特殊参数 |
| AbilityBP/关系配置 | GA 类 + 独立 Policy/Relationship 配置 | 生命周期、类型标识、阻塞/取消/要求关系 |

“类似漫威方案”指配置维度分层和可扩展内联结构，不代表 Apecox 必须复制其资产数量、编辑器规模或全部运行时基建。

### 为什么不是一个大表

目标选择、衍生物移动、命中检测、Buff 生命周期、武器弹匣和动画表现拥有不同所有权与生命周期。把它们合进一个资产会产生：

- 大量对某类技能无意义的空字段。
- 组合非法但资产仍可保存。
- 修改一个系统导致所有技能定义变化。
- 预览工具难以判断哪个字段真正生效。

### 为什么也不是每个技能一串资产继承

每个技能仍可以只有一个根 SkillDefinition 实例。它引用或内联职责明确的配置；共享配置只有在确实复用时独立成资产。

不是每个火球都创建一套 DataAsset C++ 子类。只有稳定流程族出现时，才增加新的执行配置类型或 GA 模板。

## 6. GameplayAbility 的模板与扩展

### 两个正交维度

GA 生命周期与技能流程不是同一概念。

激活策略回答“何时尝试激活”：

- 按下触发。
- 按住期间尝试。
- Avatar 就绪自动激活。

并发关系回答“可否和其他 Ability 同时运行”：

- Independent。
- Exclusive Replaceable。
- Exclusive Blocking。

流程模板回答“激活后如何推进”：

- Projectile Cast。
- Channel/Hold Release。
- Area Field。
- Charge。
- Beam/Link。
- Passive Listener。
- 射击、换弹、ADS 等武器行为。

### 推荐扩展阶梯

1. 只改配置参数：复用同一 GA 模板。
2. 局部行为可复用：使用 AbilityTask、Target Resolver、CombatEntity 行为或 Effect。
3. 总流程相同但某个语义节点特殊：覆写受保护虚函数钩子。
4. 整体生命周期不同：新增稳定流程模板 GA。
5. 生成后长期独立存在：转交 CombatEntity/WeaponInstance/Component，不让 GA 一直承载。

产生一个专用 GA 并不是失败。只有当每个技能都因为模板缺少合理扩展点而被迫复制完整流程时，架构才失败。

### 受控虚函数钩子

流程模板可以提供少量语义钩子，例如：

- ValidateConfig。
- ResolveTarget / BuildTargetData。
- OnCommitSucceeded。
- OnCastTimingReached。
- CreateCombatEntity。
- OnTargetDataReady。
- OnAbilityCancelled。
- OnAbilityEnded。

钩子由真正特殊的派生 GA 实现。它们必须围绕稳定语义命名，不能变成 `CustomStep1` 一类无边界入口。

### Shared GA 与 GAS 原生 AbilityTags

旧项目已经确认一个容易被“万能 GA”掩盖的问题：GAS 原生阻塞、取消和按 Tag 查找主要依赖 GA 类/CDO 的 AssetTags。同一个 GA 类承载多个 SkillDefinition 时，它们天然共享这组类级标签，不能假定每个 AbilitySpec 的来源标签会自动等价参与所有原生关系判断。

因此长期口径是：

- GA 类 AssetTags 先承担模板级、生命周期级的粗关系。
- SkillDefinition 可以保存技能语义标签，但不能假装它们自动成为原生 Ability AssetTags。
- 真正需要“同一 GA 模板下，不同技能拥有不同取消/阻塞关系”时，再实现经过多人验证的 Spec-aware Policy，从 AbilitySpec 的 SourceObject/动态标签读取当前技能语义。
- V1 不用遍历所有 Spec、Loose Tag 或临时补丁伪装这层能力。

## 7. AbilityTask 与 Step/Timeline

### AbilityTask 适合什么

AbilityTask 是 GA 内部可复用的异步操作：

- 等待 Montage 或 GameplayEvent。
- 等待输入释放。
- 客户端采集 TargetData 并等待服务器。
- 引导计时和中断监听。
- 持续目标扫描。
- 等待 CombatEntity 或外部系统回调。

Task 的价值是：

- 绑定 OwningAbility。
- 在 `Activate` 中开始异步工作。
- 在 `OnDestroy` 中清理定时器、委托和预测状态。
- 随 GA 结束进入统一清理。

同步读取配置、简单应用 Effect、普通 SpawnActor 不必机械地包装成 Task。

### Step 的正确位置

Step 可以用于讨论和可视化技能生命周期，但 V1 不保存一串任意 Steps 让解释器执行。

```text
GA 模板 = 固定、可读、可测试的流程骨架
AbilityTask = 骨架中的可复用异步节点
配置结构 = 模板与 Task 的参数
虚函数/事件 = 特殊技能扩展点
```

未来 Skill Timeline 可在 2-3 个代表技能后预研，用于编排动画、事件、Cue 和预览；它不能替代 GAS 的 Commit、预测、取消和结束语义。

## 8. 目标、检测与战斗衍生物

目标至少分三层：

| 层 | 回答的问题 |
| --- | --- |
| Cast Target | 这次技能围绕谁/哪里/什么方向释放 |
| Spawn Rule | 投射物、场或召唤物从哪里、以何朝向生成 |
| Detection Rule | 衍生物生成后检测谁、使用何种形状、阵营和过滤 |

例如伤害光环：

- Cast Target：Self。
- Spawn Rule：附着角色。
- Detection Rule：范围内敌人。

TargetRule 不只是 GameplayTag。它通常需要类型化字段：

- 模式枚举：Self、Actor、GroundPoint、ViewDirection 等。
- 距离、半径、角度、Trace Channel。
- 阵营/关系过滤。
- GameplayTag Requirements。
- 是否允许无目标、是否需要服务器复核。

GameplayTag 只表达稳定语义和资格，不替代空间查询规则。

### CombatEntity

投射物、法术场、陷阱、召唤物、可吸收黑洞等属于战斗衍生物。它们应拥有独立配置与运行时生命周期：

- Actor/Class。
- Spawn。
- Movement。
- Collision/Detection。
- Target Filter。
- Hit/Overlap 行为。
- Lifetime。
- OnHit/OnExpire Effect 与 Cue。
- 来源 Skill、Instigator、Source ASC 和施法实例信息。

火球命中后遗留火焰场时，ProjectileCast 仍完成通用施法；火球投射物在命中时生成 AreaField CombatEntity。无需把“遗留火场”塞进所有投射物模板。

CombatEntity 的身份优先使用 Definition/PrimaryAssetId 或明确类型。只有当其他玩法系统需要按类别查询、免疫、阻塞或筛选时，才增加稳定的 `CombatEntity.*` Tag；不要为每个投射物资产机械创建身份 Tag。

## 9. 网络权威、预测与 TargetData

### 继承路径

对本地瞄准的射击或技能：

1. 本地控制端从相机/准星执行 Trace。
2. 封装 `FGameplayAbilityTargetDataHandle`。
3. 使用当前 AbilitySpecHandle 和 ActivationPredictionKey 发给服务器。
4. 服务器验证目标、射速、弹药、状态和命中合理性。
5. 服务器 Commit 并产生最终 GameplayEffect/伤害。
6. 本地预测表现被确认或纠正。
7. 远端客户端接收第三人称表现和玩法结果。

### 完成标准

“其他端看到动画/投射物”还不够。至少验证：

- Owning Client：输入响应和预测表现及时。
- Server：资源、冷却、弹药、命中和伤害是最终真相。
- Remote Client：姿态、枪口、投射物/Cue、受击和死亡正确。
- 取消、丢包、延迟或目标无效时不产生双重结算。

后续作品阶段应加入网络模拟参数和可视化日志，而不是只用零延迟 Listen Server。

V1 先使用标准 GameplayEffectContext。只有 SkillDefinition、CombatEntity 来源、命中序号、施法实例 ID 或表面/命中区域等数据确实需要跨 GE 和网络传递时，才扩展 EffectContext 并由自定义 AbilitySystemGlobals 分配。

## 10. GameplayTag 规范

### 稳定性原则

- Tag 表达跨系统、可查询、可组合、需要 GAS 关系匹配的稳定语义。
- 封闭选项优先 UENUM 或类型化配置。
- 临时调试键、未来必删字段和单个特殊案例不预注册 Tag。
- 不因“来了一个特殊状态”就新增 Tag；先判断是否需要被其他系统查询、阻塞、授予或复制。

### 来源

- C++ 直接引用、启动早期必须存在的 Tag：Native Gameplay Tags。
- 只在内容资产中使用、允许内容迭代的 Tag：Config Tags。
- 不创建 `FApecoxGameplayTags::Get()` 式项目单例；使用命名空间和 UE Native Gameplay Tags 宏。

### 稳定域候选

正式命名仍需 RFC 审阅：

- `InputTag.*`：输入槽或输入意图。
- `Ability.*`：参与关系匹配的能力类别，不是资产数据库 ID。
- `Ability.Activation.*` 或枚举：激活策略。
- `State.*`：可被系统查询的原子状态，如 Dead、Stunned、Reloading。
- `GameplayEvent.*`：关键玩法事件。
- `GameplayCue.*`：表现入口。
- `Damage.*`：稳定伤害类别或通道，待伤害 RFC 决定。
- `Weapon.*`：需要参与规则匹配的武器类别，不替代 WeaponDefinition。

以下旧候选默认不作为 Tag：

- Full/Partial/Cancelled 等一次结算结果：优先用结果枚举/结构体；只有需要跨系统监听 GameplayEvent 时才配事件 Tag。
- 物理/法术等封闭 Damage Channel：在伤害 RFC 前优先用枚举或类型化 DamageSpec。
- 单个 CombatEntity 资产身份：优先 Definition/PrimaryAssetId。

### SetByCaller

`SetByCaller.*` 是 GE 运行时数值键，不是角色属性、状态或技能分类。

- 只有真实动态注入需求时才创建。
- 正式伤害可读取 Skill/Weapon 配置、等级、Source/Target Attribute 或 ExecutionCalculation。
- 不为“先塞一个临时伤害值”预建将来必删的 Tag。

## 11. AttributeSet、伤害、Health 与 Shield

### 多 AttributeSet 原则

不要把所有属性塞进一个万能 AttributeSet，也不要在纯基类放具体业务属性。

候选分层：

- 基础 `AttributeSet`：访问器、Clamping 辅助、通用回调工具。
- Vital：Health、MaxHealth、Shield、MaxShield 等资源。
- Combat：攻击参数、护甲/抗性或武器计算输入。
- 可选 Movement/Ammo 等只在真实需求出现时增加。

副属性可以后续新增：

- 独立设计时，加入职责最匹配的 AttributeSet。
- 由主属性派生时，优先在 GE/Execution 中计算，不必所有派生值都永久复制。
- 需要复制、被 UI 观察或被规则查询的结果，才考虑成为 Attribute。

### 数值链路

```text
Source 参数 / Weapon 或 Skill 配置
  -> GameplayEffectSpec
  -> ExecutionCalculation 或 Modifier
  -> IncomingDamage / IncomingHealing 等 Meta Attribute
  -> PostGameplayEffectExecute
  -> Shield/Health 结算与事件
  -> Health/Combat Component 处理死亡等角色流程
```

AttributeSet 保存和约束数值真相，不负责播放死亡、销毁 Pawn 或生成 UI。

### 当前范围

复杂护甲、穿透、暴击、闪避和公式后置。第一条闭环先证明：

- Shield 优先吸收。
- Health 归零。
- 服务器结算。
- UI/Cue 观察。
- Death/Respawn 生命周期正确。

## 12. State、Effect 与 Buff

继承的概念层：

- State：最小稳定状态语义，如不能移动、不能开火、死亡、装填中；通常由 Tag 和少量规则表达。
- Effect：可复用的增益/减益或数值/状态行为模板。
- Buff：某个技能或物品使用 Effect 组合形成的具体实例/包，包含持续、周期、层数、来源等。

在 UE GAS 中 GameplayEffect 已经覆盖大量 Effect/Buff 职责，因此 Apecox 不应马上再造三套平行运行时。

V1 先采用：

- GameplayEffect 表达 Duration、Period、Stack、Modifier、GrantedTags。
- Native/Config Tag 表达稳定 State。
- 只有当多个 GE 组合、驱散关系、来源实例和编辑需求真正复杂时，再增加项目级 EffectDefinition 或 BuffDefinition。

## 13. GameplayEvent、Montage、Cue 与 Timeline

### Montage GameplayEvent

需要驱动 GA 关键逻辑的动画帧使用 GameplayEvent Notify：

- 发射。
- 命中窗口。
- 消耗确认。
- 组合技窗口。

音效、脚步、纯粒子等不影响 GA 结算的内容使用普通 Notify 或动画表现层。

无论配置中是否填写等待 Tag，都仍需在 Montage 时间轴放置关键 Notify。配置中的 EventTag 用来让共享 GA 明确等待哪个事件，并支持同一 Montage 多事件。

V1 继续使用 Montage + GameplayEvent；每个技能手工标关键帧是正常内容制作流程。

### GameplayCue

Cue 负责：

- Niagara。
- 音效。
- 材质。
- 镜头反馈。
- HUD/飘字等表现。

Cue 不负责权威命中、伤害、治疗、目标选择或 Buff 规则。Execute/OnActive/WhileActive/Remove 只按表现生命周期使用。

### Timeline

长期技能 Timeline 用于：

- 在统一界面编排 Montage、关键事件、Cue 和预览。
- 降低多个资产间跳转。

它不应成为第二套 Ability 生命周期，也不绕过 Montage Notify、GameplayEvent、GAS Commit 和网络预测。

## 14. 动画与第一/第三人称表示

### 动画基础

- AnimInstance 计算动画所需的稳定数据，如 GroundSpeed、MovementDirection、IsFalling。
- AnimBP/StateMachine 表达 Locomotion 状态转换。
- BlendSpace 根据速度、方向等连续参数混合 Idle/Jog/Strafe。
- Montage 表达攻击、换弹、切枪、技能等一次性动作。
- Animation Layer/Linked Anim Graph 可用于武器族或角色差异，但真实需求出现前不搭建重型层级。

### 资产复用

- 重定向可以把其他兼容骨骼动画转到目标角色。
- AnimNotify/Montage 数据可能随复制或重定向保留，但 Socket、帧时机、手部接触和 GameplayEvent 必须复查。
- 第一人称和第三人称可以使用不同动画精度；关键语义必须一致。
- 第三人称按 Rifle/Pistol/Heavy 动作族复用，利用 Socket、Left Hand IK、Aim Offset 和小量覆写适配武器。

### Apecox 已确认的表示边界

- 玩家固定使用第一人称操控，不提供运行时第三人称视角切换。
- Owning Player 使用 FP Arms/Weapon；其他客户端使用 TP Manny/Weapon 世界表示。
- 两套表现共享装备、射击、弹药、伤害和技能状态，不要求共享 AnimBP、Montage 或最终姿势。
- Weapon Mesh、枪口锚点和手部 IK 的具体资产契约在第一把步枪表现收口中逐项确认，不改变上述视角边界。

## 15. 技能编辑器边界

长期目标是一个类似 Montage 编辑器的技能开发入口：

- Details 配置 SkillDefinition 及关联资产。
- 预览角色、Montage、GameplayCue 和 CombatEntity。
- Timeline 查看关键事件。
- 保存为配置资产，不生成每技能 C++ 静态文件。

它能保证预览和游戏尽量共用同一运行时定义，但不能完整模拟：

- 真正网络复制与预测。
- GameMode、PlayerState 和多人状态。
- 复杂服务器验证。
- 所有真实碰撞、地形和跨系统交互。

因此编辑器预览用于内容制作和局部验证，PIE/多人测试仍是最终真相。

## 16. 已明确放弃或延后的旧方向

- 一个万能 RuntimeAbility 解释所有技能。
- 一个 DataAsset 填满所有施法、衍生物、Buff、表现和输入字段。
- V1 任意 Step/Fragment 数组解释器。
- 冷启动阶段完整 SkillGraph/IR/VM。
- 用 GameplayTag 表示每一个临时分支或数值键。
- 把 AbilitySet 误当 SkillDefinition。
- 把 GameplayCue 当玩法逻辑。
- 把 AttributeSet 当死亡流程控制器。
- 为了“没有每技能 GA”而拒绝合理专用 GA。
- 在没有 2-3 个稳定原型前实现技能编辑器。

## 17. 必须保留的工程经验

- Soft Object Pointer 的 `IsValid` 只表示已加载，非空资产引用应先检查 `IsNull`，加载后检查结果。
- AbilitySpec 必须通过明确 Handle/ASC 查找，不能依赖错误的当前 Spec 假设。
- EffectSpecHandle、Source ASC、TargetData 与 HitResult 在使用前验证有效性。
- 不安全的 TargetData `static_cast` 应改用虚接口/类型检查。
- 伤害、消耗和等级使用实际 Ability Level，不偷用默认配置等级。
- Authority 和预测分支必须成对；客户端不能擅自移除服务端 GE。
- AbilityTask、Delegate、Timer、Montage 和 GameplayEvent 等待必须在结束/取消时清理。
- 发射时机事件可能在 Task 绑定前到达；激活顺序和事件缓存必须验证。
- CombatEntity 生成成功日志不等于可见、移动、命中或复制成功。
- 每个完整技能/武器动作都应有运行链路文档：输入到激活、Commit、目标、动画、生成/命中、GE、Cue、UI 和网络观察点。

完整历史检查表见 `90_References/Inherited_Framework/Skill_Runtime_Risk_Checklist.md`。

## 18. Apecox 下一轮 RFC 必须回答

- 第一/第三人称角色表示与动画职责。
- Gameplay Framework 最小类集合和所有权。
- PlayerState ASC 初始化/解绑和 Respawn。
- WeaponDefinition、WeaponInstance、Equipment/Inventory 与 AbilitySet 边界。
- 第一把步枪的预测和服务器验证口径。
- Health/Shield AttributeSet 与伤害入口。
- InputAction、InputTag、AbilitySpec 路由。
- 第一批稳定 Native GameplayTag 命名。
- 第三方资产目录、骨骼和动画重定向策略。

这些问题按阶段讨论，不在同一轮把所有类一次性写死。
