# Apex 技能框架：配置驱动与特殊机制扩展综合分析

生成日期：2026-07-15
状态：架构讨论稿，用于下一步 Apex GAS RFC，不代表已经实施。
输入来源：Lyra UE 5.8 源码、Lyra GAS 分析文章、漫威争锋战斗框架分享文章、Aura 阶段实践和用户最新目标校准。

## 1. 目标重新校准

Apex 的目标不是：

- 一个 DataAsset 配出所有类型的技能。
- 一个 GameplayAbility 解释所有流程。
- 把所有代码变成字段、Step 或 Tag。
- 为了未来技能编辑器，提前制造一套庞大的图执行引擎。

Apex 真正要追求的是：

```text
常见技能：复用成熟模板，主要填写配置即可完成。
中等差异：复用 Task、策略、组件或扩展钩子，不复制整套流程。
特殊技能：允许编写专用 GA / Task / CombatEntity 行为，但仍复用底层基建。
```

衡量框架好坏的标准不是“零代码技能占比”，而是：

1. 新技能需要改动的范围是否清楚。
2. 共性逻辑是否只维护一份。
3. 特殊逻辑是否有明确归属，而不是污染公共配置。
4. 多人、生命周期和清理是否由框架统一保证。
5. 开发者能否从模板开始，逐级增加复杂度，而不是一开始就理解全部系统。

## 2. 两篇文章应该怎样使用

### 2.1 Lyra GAS 深度解析文章

文件：`Agent/90_References/Lyra AbilitySystem 深度解析：GAS的正确打开方式 - 知乎.mhtml`

它对 Lyra 的以下内容总结较准确，并已由本地 UE 5.8 源码核对：

- ASC 的输入缓存与统一处理。
- ActivationPolicy / ActivationGroup。
- AbilitySet 的授予与对称撤销。
- AdditionalCost 的策略对象设计。
- Meta Attribute、DamageExecution、HealthComponent。
- TagRelationshipMapping。
- EffectContext 和 CueManager 的职责。

需要保留判断：

- “三种激活策略覆盖所有场景”是便于讲解的概括，不是所有游戏的完整分类。
- AbilitySet 数据化的是授予关系，不是具体技能的全部逻辑。
- GlobalAbilitySystem、GamePhase 和自定义 CueManager 是 Lyra 规模下的能力，不是 Apex 冷启动前置。
- 文章中的完整激活链路经过简化，具体 GA 仍决定何时 Commit、如何收集目标和如何应用 Effect。

### 2.2 漫威争锋战斗框架文章

文件：`Agent/90_References/(7 封私信 _ 68 条消息) UF2025(Shanghai)——《漫威争锋》战斗系统架构实战：从GAS框架到客户端预测的完整落地方案 - 知乎.mhtml`

它是技术分享的二次整理，缺少完整源码，因此只借鉴架构思想，不复制类名和规模。

最值得借鉴的思想：

1. 技能配置不是一张万能表，而是公共配置、特殊配置和关系配置分层。
2. AbilityTask 是机制复用单元，技能模板位于 Task 之上。
3. 普通技能走模板，完全特殊的技能允许独立开发。
4. Timeline 管逻辑时序，Montage 管动画播放，二者协作而非互相取代。
5. Combat Entity 独立承担投射物、法术场、召唤物的生命周期。
6. 检测规则同时考虑 Source 和 Target，不能把所有交互写死在技能发起方。
7. 技能编辑器是多个模块的一站式入口，不代表所有模块必须存进同一个资产。

不直接采用：

- AAA 规模的完整检测、生成、战斗数据三大管线。
- 大量 Buff / Effect / CDR 实例和复杂效果器覆盖系统。
- 完整 Timeline、TypedCue、Niagara Data Interface 和全套预测回滚框架。

这些能力应在 Apex 出现真实需求后逐步提炼。

## 3. 配置驱动不是“一个配置打所有”

“每个技能有一个根资产”和“所有技能共享一个巨大字段结构”不是一回事。

建议长期保持一个技能根入口，但让它只负责组装：

```text
SkillDefinition（根入口）
  ├─ Identity / UI / Presentation
  ├─ 选择哪一种执行模板或专用 GA
  ├─ 一个与模板匹配的内联 ExecutionConfig
  ├─ 引用 CombatEntityDefinition
  ├─ 引用 GameplayEffect / EffectDefinition
  └─ 引用 GameplayCue / Montage / Timeline
```

根资产的价值是统一检索、预览、校验、授予和未来编辑器入口，不是吞并所有子系统。

### 3.1 公共配置

适合所有技能稳定拥有的内容：

- PrimaryAssetId / 显示名 / 描述 / 图标。
- 使用哪个 GA 模板类或专用 GA 类。
- 技能分类和稳定的策略标签。
- 表现资源入口。
- 冷却、成本的通用引用或配置入口。

不建议把固定 InputTag 放在这里。同一个技能可以在不同英雄或 Loadout 中占用不同输入槽。

### 3.2 模板专属配置

不同模板只显示自己的字段：

| 模板 | 专属配置示例 |
| --- | --- |
| ProjectileCast | 发射 Socket、速度、数量、散布、CombatEntityDefinition。 |
| ChannelCast | 最短/完整引导时间、中断条件、完整/部分结算。 |
| AreaCast | 落点规则、范围、周期、法术场定义。 |
| ChargeCast | 蓄力曲线、阶段阈值、释放结果。 |
| MeleeCombo | Montage Sections、攻击窗口、连段输入窗口。 |

这部分可以使用“一个多态内联结构”或“模板对应的 SkillDefinition 子类”。正式 RFC 再比较实现形式。

### 3.3 独立关系配置

打断、阻塞、优先级、状态要求不应全部重复写在每个技能里。

它们应由两层组成：

- GA 模板自身的默认并发语义。
- 角色 / 规则集使用的 AbilityRelationshipMapping。

确实只属于某个技能的例外，可以存在 SkillDefinition 的 PolicyConfig 中，但不能让公共关系表变成 per-skill 字段复制。

## 4. GA、模板、Task 和配置的边界

### 4.1 `UApexGameplayAbility` 基类

负责所有能力都必须正确处理的 GAS 规则：

- 读取 AbilitySpec / SourceObject。
- ActorInfo 和 ASC 类型安全访问。
- 激活失败原因。
- ActivationPolicy / 并发关系。
- Commit、取消、结束和清理的公共保护。
- 网络执行策略的公共约束。

它不负责解释所有技能流程。

### 4.2 流程模板

模板负责一类技能的稳定主流程。例如 ProjectileCast：

```text
激活
-> 校验配置
-> 播放施法表现 / 等待时机
-> 解析施法方向或目标
-> 生成 CombatEntity
-> 结束或等待模板要求的结果
```

模板的首选落点可以是一个 GA 子类，因为 AbilityTask、预测键、Commit 和取消天然属于 GA 生命周期。

但这里不把“模板必须永远等于 GA 子类”写成教条。如果后续证明多个 GA 生命周期需要共享同一段纯流程，也可以抽出普通 C++ 协调器或策略对象；前提是它不再自造第二套 Ability 生命周期。

### 4.3 AbilityTask

Task 负责可复用、异步、需要随 Ability 自动清理的行为：

- 等待 Montage / GameplayEvent。
- 等待输入释放。
- 引导计时和中断监听。
- 本地目标收集与 TargetData 同步。
- 持续检测。
- 位移、Timeline 片段等异步过程。

Task 不等于技能配置中的每个动作，也不要求每个同步函数都包装成 Task。

### 4.4 扩展钩子

模板应只在明确的语义节点提供钩子，例如：

- 目标解析前后。
- 关键施法时机到达。
- CombatEntity 生成前后。
- TargetData 确认后。
- Commit 成功后。
- 技能结束或取消时。

钩子数量应受控。大量细碎钩子意味着模板边界可能划错，或者应拆出新的 Task / CombatEntity 行为。

### 4.5 专用 GA

完全改变执行生命周期、预测方式或状态机的技能，可以拥有自己的 GA 子类或蓝图。

专用 GA 仍应复用：

- `UApexGameplayAbility` 基类。
- 通用 AbilityTask。
- TargetData、Effect、Cue 和 CombatEntity 基建。
- 统一的成本、冷却、关系和失败原因。

因此“允许专用 GA”不等于放弃复用。

## 5. 技能差异的升级阶梯

实现新技能时，按下面顺序判断：

### 第一级：只有参数不同

例子：火球速度不同、治疗数值不同、范围不同。

处理：填写模板配置，不新增类。

### 第二级：存在可复用的局部行为

例子：穿透、反弹、命中后分裂、按命中扣费。

处理：增加可复用策略、Task、Effect 或 CombatEntity 行为组件。不要立刻给公共根资产加字段。

### 第三级：模板顺序相同，但某个语义节点特殊

例子：投射物生成前需要二次选点；命中确认后产生额外分支。

处理：覆写受保护钩子，或为该节点提供一个类型化策略。

### 第四级：整体生命周期不同

例子：传送门、变身、多角色联合施法、复杂武器持续开火。

处理：新增专用 GA 或新的流程模板。

### 第五级：特殊机制在 GA 结束后持续存在

例子：黑洞、法术场、召唤物、陷阱、持续光环。

处理：逻辑属于 CombatEntity 或 Effect/Buff 子系统，不应继续挂在施法 GA 上。

这个升级阶梯比“配置或写代码二选一”更符合实际开发。

## 6. 黑洞技能推演

需求：角色发射黑洞；黑洞落地或到点后形成区域，并吸引或吞噬飞行物。

### 6.1 GA 模板负责什么

黑洞仍可以复用 ProjectileCast 的施法主流程：

- 检查技能能否激活。
- 扣除成本和进入冷却。
- 播放施法动作。
- 获取方向或目标点。
- 生成黑洞投射物 CombatEntity。

这些步骤与普通火球有共性，不需要复制。

### 6.2 黑洞投射物负责什么

- 飞行轨迹。
- 碰撞或到点触发。
- 从 Projectile 阶段转换为 Field 阶段，或生成另一个 Field Entity。
- 自身生命周期和网络复制。

### 6.3 黑洞法术场负责什么

- 周期检测范围内目标。
- 对角色应用吸引 Effect / 位移规则。
- 对 Projectile 类 CombatEntity 应用吸收、销毁或改向行为。
- 播放持续 Cue。
- 到时清理。

### 6.4 飞行物交互怎样解耦

V1 可先定义统一接口或行为入口：

```text
Projectile 被检测到
-> BlackHoleField 请求执行 ProjectileInteraction.Absorb
-> 目标 Projectile 决定如何响应并由 Authority 执行
```

不能在黑洞里 Cast 每一种具体火球、箭矢或飞镖类。

需要特别区分：

- 有 Actor 的投射物可以被区域检测和交互。
- Hitscan / 即时射线不存在持续飞行 Actor，无法事后被黑洞扫描吸收。

若以后要求黑洞也拦截 Hitscan，就需要让所有射线进入统一 Detection Pipeline，在检测执行前询问空间拦截器。这属于管线级能力，不是给黑洞多加一个 Step 就能可靠完成。

### 6.5 黑洞是否需要特殊 Step

通常不需要把“持续吸收飞行物”做成 GA Step。GA 只需要一个“生成黑洞 CombatEntity”的语义节点，吸收机制由 Entity 自己运行。

只有黑洞施法阶段本身出现独特异步过程时，才新增 AbilityTask 或专用 GA 钩子。

## 7. Step 与 Timeline 的保留方式

Step 仍然有价值，但应分清三种含义：

1. **分析 Step**：描述技能运行链路，帮助讨论和写文档。
2. **模板内部阶段**：GA C++ 中明确的状态和函数，不一定资产化。
3. **未来 Timeline 节点**：编辑器里可视化的有限类型事件。

V1 不实现任意 Step 解释器。等 2-3 个技能稳定后，可先实现有限 Timeline Track：

- Gameplay Event Marker。
- GameplayCue Track。
- CombatEntity Spawn Marker。
- Effect Apply Marker。
- Hit / Movement Window。

Timeline 负责逻辑时间，Montage 负责动画。Timeline 可以触发模板定义好的语义事件，但不能随意调用任何 UObject 函数。

这样既支持未来技能编辑器，也避免把运行时变成难以调试的通用脚本引擎。

## 8. Spec-aware per-skill Policy 是什么

这里的 `Spec` 指 `FGameplayAbilitySpec`，不是 InputAction，也不是 IA 资产。

当多个技能共享同一个 GA 模板类时，每个已授予技能都有自己的 AbilitySpec：

```text
Fireball Spec
  AbilityClass = ProjectileCast
  SourceObject = DA_Skill_Fireball

BlackHole Spec
  AbilityClass = ProjectileCast
  SourceObject = DA_Skill_BlackHole
```

UE 原生阻塞和取消主要读取 GA 类的 AssetTags。因为两个 Spec 指向同一个 GA 类，原生系统只能看到它们共同的模板 Tag，无法自动读取两个 SkillDefinition 各自的关系配置。

Spec-aware Policy 的含义只是：

```text
在判断或登记技能关系时，不只看共享 GA 类，
还通过当前 AbilitySpec 找到它的 SkillDefinition / 动态策略标签。
```

适用示例：

- 火球和黑洞共享 ProjectileCast，但黑洞施法不能被普通攻击打断。
- 两个 ChannelCast 使用不同的移动限制。
- 同一模板下只有一个技能会取消正在运行的隐身技能。

它不是当前必须创建的新系统。使用原则：

1. 模板级关系足够时，只使用 GAS 原生 AbilityTags。
2. 生命周期明显不同就拆成不同 GA 类，不用 Policy 强行粘合。
3. 只有“流程应当共享，但技能关系确实不同”时，才增加 Spec-aware 关系求值。

因此 V1 只预留概念，不提前实现。

## 9. 配置形态候选

### 方案 A：每个模板一个 SkillDefinition 子类

```text
UApexSkillDefinition
  -> UApexProjectileSkillDefinition
  -> UApexChannelSkillDefinition
```

优点：类型安全、细节面板清楚、校验简单。
缺点：DataAsset 类数量增加，跨模板复用字段需要维护继承关系。

### 方案 B：根 SkillDefinition + `FInstancedStruct`

根资产保存一个多态内联 ExecutionConfig，例如 ProjectileConfig 或 ChannelConfig。

优点：接近漫威争锋分享的思路，根资产统一，模板特殊字段可内联，适合未来统一编辑器。
缺点：需要严格验证“GA 模板类与 Config Struct 类型是否匹配”，编辑器定制和错误提示要求更高。

### 方案 C：Instanced UObject / Fragment 列表

优点：可包含虚函数和可复用行为，扩展灵活。
缺点：顺序、依赖、冲突和生命周期容易失控，很容易重新变成任意 Step 系统。

### 方案 D：根资产引用多个子 DataAsset

优点：子配置可跨技能共享，职责非常清楚。
缺点：资产数量和跳转成本增加。

### 当前建议

#### 用三个技能看两种方案

假设第一批存在三个根技能资产：

- `DA_Skill_Fireball`
- `DA_Skill_PiercingOrb`
- `DA_Skill_ChannelHeal`

方案 A 中，项目只为每一种流程模板声明一个原生 DataAsset 类，而不是为每个技能声明一个类：

```text
UApexSkillDefinition
  -> UApexProjectileSkillDefinition
  -> UApexChannelSkillDefinition
```

资产实例对应关系：

| 技能资产 | 资产类 | 模板专属字段示例 |
| --- | --- | --- |
| `DA_Skill_Fireball` | `UApexProjectileSkillDefinition` | 发射 Socket、速度、数量、火球 CombatEntity。 |
| `DA_Skill_PiercingOrb` | `UApexProjectileSkillDefinition` | 同一组投射字段，CombatEntity 额外引用可复用的穿透行为。 |
| `DA_Skill_ChannelHeal` | `UApexChannelSkillDefinition` | 最短/完整引导时间、中断条件、完整/部分结算。 |

运行时的 ProjectileCast GA 可以直接要求 SourceObject 是 `UApexProjectileSkillDefinition`。类型不匹配时，Data Validation 和 C++ Cast 都能明确报错。

新增第 20 个投射物技能时，只创建新的 DataAsset 实例，不创建第 20 个 C++ DataAsset 类。只有出现新的稳定流程模板，例如 `ChargeCast`，才新增 `UApexChargeSkillDefinition`。

方案 B 中，三个技能资产都使用同一个 `UApexSkillDefinition`，但在 Details 中为 `ExecutionConfig` 选择不同结构体：

```text
DA_Skill_Fireball
  ExecutionConfig = FApexProjectileExecutionConfig

DA_Skill_PiercingOrb
  ExecutionConfig = FApexProjectileExecutionConfig

DA_Skill_ChannelHeal
  ExecutionConfig = FApexChannelExecutionConfig
```

运行时必须同时验证：

```text
AbilityClass = ProjectileCast
ExecutionConfig 类型 = ProjectileExecutionConfig
```

如果编辑者误选了 ChannelConfig，资产在序列化层面仍然合法，但 ProjectileCast 无法使用它。项目需要额外的 Data Validation、Details 过滤和错误提示来防止这种组合。

黑洞在两种方案中都不应把“吸收投射物半径、吸力、可吞噬类型”等字段加入 ProjectileCast 公共配置。这些字段属于黑洞法术场的 CombatEntityDefinition。只有黑洞的施法阶段本身改变了 ProjectileCast 生命周期，才需要新的模板配置或专用 GA。

#### 最终选择：受约束的方案 B

用户于 2026-07-15 确认：Apex 采用与漫威争锋演讲展示更接近的“根资产 + InlineStruct”形式。

```text
UApexSkillDefinition（每个技能一个根资产实例）
  |- CommonConfig
  |- AbilityClass / Template 标识
  |- TInstancedStruct<FApexSkillExecutionConfig> ExecutionConfig
  |- CombatEntity / Effect / Cue 等独立资产引用
  `- Policy / Presentation 入口
```

其中 `ExecutionConfig` 必须受以下约束：

1. 只允许一个类型化执行配置，不提供任意 Fragment 数组。
2. 所有结构都继承统一的 `FApexSkillExecutionConfig` 基结构。
3. Details 选择器只显示允许的派生配置类型。
4. 每个 GA 模板声明自己接受的 Config 类型，并在 Data Validation 与激活入口双重校验。
5. ProjectileConfig 不吸收黑洞法术场、穿透、追踪等生成后行为；这些仍归 CombatEntityDefinition。

采用 B 的原因不是追求万能资产，而是保持统一技能入口，同时允许不同模板拥有独立内联结构。方案 A 的类型安全优势由上述限制和校验补回。

这里借鉴的是演讲展示的配置形态，不照搬其 AAA 规模：文章正文只明确了基础配置、技能特殊配置和关系配置三态；InlineStruct 内联关系来自演讲截图。具体 C++ 名称和字段仍由 Apex RFC 定义。

## 10. 动画基线的区别

### 使用 Paragon Phase 原有 AnimBP

优点：

- 最快获得现成 Locomotion、BlendSpace 和角色动作。
- 适合当前优先验证 GAS 和技能逻辑。

缺点：

- 状态机、变量和 Slot 由第三方设计，学习和改造成本可能较高。
- 后续技能状态、Linked Anim Layer、项目命名和调试工具不完全受我们控制。

### 创建 Apex 自有 Locomotion AnimBP

这里的“自有”不要求自己制作动画素材。可以继续使用 Paragon Phase 的 Skeleton、AnimSequence 和 Montage，但状态机、变量、Slot 和动画层由 Apex 创建。

优点：

- 命名、状态、GAS Tag 读取和 Montage Slot 都由项目控制。
- 更适合长期教学、扩展和多英雄规范。

缺点：

- 前期需要重新搭 Locomotion、空中状态和 BlendSpace。
- 会暂时分散 GAS 架构工作的注意力。

### 建议

短期使用 Phase 原有 AnimBP 完成基础动画和技能验证；长期创建 Apex 自有 AnimBP 或 Linked Anim Layer。这个决策不阻塞 GAS RFC，等进入最小动画阶段再确认具体迁移方式。

## 11. 对未来技能编辑器的影响

技能编辑器不要求所有内容存进一个 SkillDefinition。

它可以是一个统一入口：

- 左侧选择 SkillDefinition 和模板。
- Details 编辑公共配置与模板专属 ExecutionConfig。
- Timeline 预览 Montage、关键事件和 Cue。
- 子面板编辑或跳转 CombatEntity、Effect、Detection 配置。
- 预览场景运行可预览的表现与实体。
- 复杂服务器规则仍通过 PIE / 多人测试验证。

框架越明确，编辑器越容易知道：

- 哪些字段应该显示。
- 哪些资产可以预览。
- 哪些错误可以静态校验。
- 哪些机制必须进入运行时测试。

因此当前只需保证配置和运行时有稳定边界，不必提前实现编辑器。

## 12. RFC 前当前共识

已确认：

1. 不把“一个配置覆盖所有技能”作为目标。
2. 不把“一个万能 GA”作为目标。
3. SkillDefinition 是组装入口，可以引用或内联不同职责的配置。
4. 普通技能优先走模板，特殊技能允许专用 GA / Task / Entity 行为。
5. Task 位于可复用异步行为层，模板位于 Task 之上。
6. Step 暂时保留为分析语言和未来有限 Timeline 概念。
7. AbilitySet / Loadout 负责授予和 InputTag，不等于 SkillDefinition。
8. CombatEntity 独立承担生成后的持续机制。
9. Spec-aware Policy 只在共享模板且技能关系确有差异时使用，V1 不预实现。
10. 未来技能编辑器是统一入口，不要求单一资产或单一运行时解释器。

## 13. 下一步建议

进入正式 RFC 前，先完成两个小讨论：

1. SkillDefinition 的模板专属配置选择方案 A（子类）还是方案 B（`FInstancedStruct`）。
2. 选择三个架构验证样本：一个普通模板技能、一个同模板局部变体、一个依赖特殊 CombatEntity 行为的技能。

推荐样本组合：

- 普通投射物：验证模板和基础配置。
- 穿透或分裂投射物：验证可复用局部行为。
- 黑洞：验证 CombatEntity、检测交互和特殊扩展边界。

这三个样本比单独实现十个简单技能更能检验框架是否真的可复用和可扩展。
