# Apecox 战斗框架架构研究 Goal（待用户审阅）

状态：草案，尚未启动  
生成日期：2026-08-05

## 可复制的 /goal 提示词

```text
你正在为 Unreal Engine 5.8 项目 Apecox 执行一次有边界、可停止的战斗框架架构研究。

【最终目标】

基于 Apecox 已迁移的技术积累、Lyra Starter Game 实际源码、现有课程/资产内容、UE 官方能力和可靠的公开资料，设计一套适用于“小规模多人英雄射击原型”的易扩展、易开发、可验证战斗框架。

本 Goal 只完成框架级研究和一份架构设计文档，不进入 Apecox 项目类名、函数、成员字段、文件布局或代码实现。架构文档未经用户讨论和批准，不得进入详细设计或代码阶段。

【项目与产品边界】

项目路径：
- D:/UnrealProject/Apecox

本机参考：
- D:/UnrealProject/LyraStarterGame
- D:/UnrealProject/Apex
- D:/UnrealProject/Aura
- D:/UE_Resource

原型暂定：
- 多人玩家对抗，当前不设计 AI 战斗。
- 玩家出生为空手，需要在世界中拾取枪械和物品。
- 所有英雄都能使用公共枪械系统。
- 每名英雄有两个主动技能：一个小技能和一个大招；另有可能存在被动技能。
- 射击、瞄准、换弹、拾取、死亡、复活等是公共能力或公共战斗行为。
- 武器首批覆盖步枪、霰弹枪、狙击枪。
- 弹药类型包括狙击、能量、重型、轻型、霰弹；弹药类型与武器类别解耦，由具体武器定义决定。
- 支持空手、持枪、瞄准、跳跃、四向移动和武器/技能切换。
- 支持不同倍率或倍率范围的瞄具，如 1x、2-4x、6-8x；武器具备瞄具兼容规则。
- 支持不改变武器外观的操控类配件，例如腰射扩散、后坐力、弹匣；瞄具可改变可见模型和 ADS 表现。
- 玩家拥有可进化护盾。护盾受到伤害、破碎、通过电池或技能恢复；护盾上限可通过造成伤害或拾取物品进化。
- 英雄技能可能临时装备或生成弓箭、火箭筒、治疗跟踪无人机、投射物、法术场等不同战斗载体。
- 特殊技能可以拥有独立伤害语义、动画和表现，但必须能和公共武器/状态/网络框架协作。
- 本地第一人称表现和其他玩家看到的第三人称表现都必须成立。
- 滑铲和滑铲跳作为需要纳入架构边界的候选能力；不要求本 Goal 证明最终参数，但必须决定它与 CharacterMovement、GAS、动画和持枪状态的职责关系。
- 当前不做完整大逃杀、大地图、缩圈、复杂经济、排名、AI、完整英雄阵容或技能编辑器。

【本 Goal 的设计颗粒度】

用户列出的武器、弹药、配件、进化护盾、特殊技能、动画、双视角和滑铲跳等行为，主要用于检验框架的覆盖能力和扩展性，不代表本 Goal 要逐项完成详细设计。

本 Goal 应回答：
- 系统应如何分层，运行时真相由谁拥有。
- GAS、Gameplay Framework、移动、动画、装备、配置和表现之间如何协作。
- 普通情况如何快速开发。
- 特殊蒙太奇、特殊枪械配件、临时武器、无人机或特殊技能出现时，应通过哪一类扩展机制接入，而不破坏公共流程。
- 哪些问题已经有足够证据可以推荐方向，哪些必须留给后续专题讨论。

本 Goal 不应回答：
- Apecox 具体 C++ 类名、父类、函数签名和成员变量。
- 最终 GameplayTag 列表或 DataAsset 字段表。
- 每一个玩法的完整时序、网络 RPC 和动画节点实现。
- 尚无证据的问题的唯一答案。

可以引用 Lyra 和 UE 现有类/函数来解释证据，但不能把它们直接转换成 Apecox 已批准的 API。

【必须先读取】

优先读取并遵守：
- D:/UnrealProject/Apecox/.agents/ue-project-context.md
- D:/UnrealProject/Apecox/Agent/README.md
- D:/UnrealProject/Apecox/Agent/00_Coordination/Decision_Log.md
- D:/UnrealProject/Apecox/Agent/02_CombatFramework/Framework_Inheritance_Baseline.md
- D:/UnrealProject/Apecox/Agent/01_Project/Product_And_Portfolio_Direction.md
- D:/UnrealProject/Apecox/Agent/01_Project/Legacy_Knowledge_Migration_Report.md
- D:/UnrealProject/Apecox/Agent/90_References/Inherited_Framework 中与 Lyra、GAS、AttributeSet、动画、技能扩展和运行时风险有关的精选资料

历史文件只提供证据，不自动决定 Apecox 的类名或实现。

【研究证据等级】

按以下优先级形成结论：
1. UE 5.8 / Lyra 的实际源码和引擎头文件。
2. Epic 官方文档、官方示例、官方演讲。
3. 已保存的高质量架构文章和课程源码。
4. 社区成熟实践和技术文章。
5. Apex Legends 等商业产品的可观察行为。

必须区分：
- 源码中已经证实的事实。
- 从产品行为或资料中作出的工程推断。
- Apecox 为控制规模而作出的自主取舍。

引用技术资料时优先原始来源。网络搜索技术问题时只依赖官方文档、源码、论文或原作者材料形成关键事实；社区资料只能作为交叉验证。Apex Legends 用于玩法语义和体验目标，不假定了解其内部代码。

【必须定向解构的 Lyra 部分】

以实际源码为准定位类和调用链，不只阅读文章：
- Gameplay Framework：GameMode、GameState、PlayerController、PlayerState、Pawn/Character 的职责和网络存在范围。
- ASC Owner/Avatar、初始化、解绑、死亡和 Respawn。
- Lyra AbilitySystemComponent、GameplayAbility 基类、AbilitySet、InputConfig、AbilityTagRelationshipMapping。
- PawnData、PawnExtension/HeroComponent 中值得轻量化采用的聚合与初始化思想。
- Inventory、InventoryItemDefinition/Instance、EquipmentManager、EquipmentDefinition/Instance、QuickBar 或等价槽位系统。
- WeaponInstance、RangedWeaponInstance、Ranged Weapon Ability、TargetData、Spread、Aim/ADS、Ammo/Reload 的实际边界。
- HealthSet、CombatSet、HealthComponent、Damage/Heal Execution、Death Ability。
- GameplayCue、动画层、Montage/GameplayEvent、摄像机或第一/第三人称表现。
- 客户端预测、PredictionKey、服务器验证、复制条件、GameplayMessage 或事件分发。

对每个 Lyra 设计都要标记：
- Apecox 直接采用的思想。
- Apecox 轻量化采用的思想。
- 因规模过重暂不采用的部分。
- 不能满足 Apecox 需求、需要自建设计的部分。

【必须研究并形成框架边界的系统】

对每个系统使用统一表达：
- 已确认的产品需求。
- 推荐的框架方向。
- 推荐理由和它解决的问题。
- 至少一个可行替代方向及其代价。
- 对普通玩法提供的复用方式。
- 对特殊玩法预留的扩展方式。
- 证据不足或影响较大的待讨论问题。

不要为了让文档看起来完整而强行选择。无法确认的部分必须明确标记“待讨论”，说明为什么现在不能决定，以及后续需要什么信息。

1. Gameplay Framework
- 最小 GameMode、GameState、PlayerController、PlayerState、Character/Pawn 体系。
- 各类在 Dedicated Server、Listen Server、Owning Client、Remote Client 的存在和职责。
- 玩家加入、出生、死亡、复活、重新绑定 ASC 和装备清理的顺序。

2. GAS 基础
- ASC 放置、ReplicationMode、ActorInfo 初始化/解绑。
- AbilitySet 的授予和撤销。
- InputAction -> InputTag -> AbilitySpec 路由。
- GameplayAbility 基类、激活策略、并发/排他关系和 TagRelationship Policy。
- AttributeSet、GE、Execution、GameplayCue、GameplayEvent、AbilityTask 的边界。
- 共享 GA 的类级 AssetTags 与每技能策略差异如何处理。

3. 拾取、库存、装备与槽位
- 玩家出生为空手。
- 世界拾取物、库存条目、装备实例和当前手持物的区别。
- 武器槽数量是否属于角色/模式配置。
- 拾取、装备、卸下、切换、丢弃、死亡清理和复活初始状态。
- 权威所有权、复制粒度和客户端预测边界。
- 第三方资产数据与运行时实例分离。

4. 武器框架
- WeaponDefinition、WeaponInstance、WeaponPresentation、WeaponAbilitySet 等候选职责。
- 步枪、霰弹枪、狙击枪如何共享基础流程，又允许射击模式、弹丸/HitScan、散布、装填和伤害不同。
- 射击、连射、单发、ADS、换弹、切枪、检视等行为哪些使用 GA，哪些属于武器实例/组件。
- 弹匣当前状态、备用弹药、射速、散布、后坐力、热量等状态放在哪里。
- 枪械行为和英雄技能临时武器如何复用同一套接口。

5. 弹药与配件
- AmmoType 与 WeaponType 解耦。
- 弹匣内弹药、背包备用弹药、世界弹药拾取的定义和复制。
- 瞄具兼容、倍率/可变倍率、ADS FOV、第一人称瞄具显示和第三人称可见模型。
- 操控类配件如何通过类型化 Modifier/Stat Aggregation 影响腰射扩散、后坐力、弹匣、换弹等参数。
- 避免把所有配件效果写成无限 GameplayTag 或直接篡改 Definition。
- 明确哪些数值适合 GAS Attribute，哪些只是 WeaponInstance 运行时属性。

6. 进化护盾、伤害和恢复
- Health、Shield、MaxShield、ShieldTier/EvolutionProgress 的数据归属。
- 伤害如何先结算护盾，再结算生命。
- Shield Break、恢复、电池使用、技能恢复和进化升级的事件链。
- 造成伤害与护盾进化积分之间如何解耦。
- 死亡、复活后护盾等级和资源是否保留，作为模式策略而不是硬编码。
- V1 简单伤害与未来抗性、爆头、距离衰减、武器/技能伤害类型的扩展入口。

7. 英雄技能与特殊战斗载体
- 每名英雄小技能、大招、被动和公共能力的授予来源。
- SkillDefinition、类型化 ExecutionConfig、GA 流程模板、AbilityTask、虚钩子和专用 GA 的扩展阶梯。
- 临时弓箭/火箭筒属于临时装备、特殊武器实例还是技能专用 CombatEntity，需要给出判定规则。
- 无人机、投射物、法术场、陷阱等 CombatEntity 的 Definition、运行时 Actor、检测、命中、生命周期和来源上下文。
- 技能激活时如何中断/收起武器，结束后如何恢复原装备与姿态。

8. 动作仲裁与动画
- 空手、持枪、ADS、换弹、切枪、使用技能、使用消耗品、跳跃、下落、滑铲、死亡如何组合。
- 不用一个巨大枚举或一个巨大 AnimBP 硬编码所有组合。
- GameplayTag、Ability 并发组、Montage Slot、Linked Anim Layer/Anim Layer Interface、Upper/Lower Body Layer、AimOffset、IK 和 Motion/Orientation Warping 的合理边界。
- 哪些动画事件驱动玩法，哪些只是表现。
- 换枪、技能、跳跃、滑铲对当前 Montage/Ability 的取消、排队、替换和恢复策略。
- 输出最低资产需求清单：第一人称与第三人称分别需要哪些 Locomotion、持枪、ADS、开火、换弹、切枪、拾取、消耗品、滑铲和特殊技能资产；哪些可复用或重定向。

9. 第一人称与第三人称表示
- 比较单全身 Mesh、第一人称 Arms + 第三人称 Body、双完整 Mesh 等方案。
- 本地与远端的 Mesh 可见性、动画实例、Montage、武器附着、Socket、枪口、摄像机和阴影。
- 第一人称 Trace/准星目标与第三人称枪口方向的一致性。
- 本地立即反馈、服务端真相、远端第三人称表现如何避免重复播放。
- 明确选定 Apecox V1 方案和以后升级路径。

10. 移动与滑铲跳
- 基础移动、四向移动、跳跃、空中控制、蹲伏、滑铲、滑铲跳的 CharacterMovement 边界。
- 滑铲物理和网络预测是否需要自定义 CharacterMovementComponent、MOVE_Custom、FSavedMove/NetworkPrediction 扩展。
- GAS 只负责状态门控、消耗、打断或触发，不能用 Ability 每 Tick 重新实现移动物理，除非有充分证据。
- 持枪/ADS/换弹/技能期间能否滑铲或跳跃，应进入统一动作关系策略。
- V1 是否先只预留接口而后实现完整预测滑铲，必须给出判断。

11. 网络、预测和反作弊边界
- 对每条核心链路给出 Owning Client、Server、Remote Client 的执行顺序。
- 开火、TargetData、射速、弹药、换弹、拾取、切枪、护盾恢复、技能、移动分别决定是否预测。
- HitScan 与 Projectile 的命中权威、客户端表现和服务器验证。
- 是否以及何时需要 Lag Compensation/Server-Side Rewind；V1 可以延后，但必须预留边界且不虚假承诺。
- 复制条件、FastArray/子对象复制、Actor Relevancy、Dormancy 或 Iris 的使用建议。
- Dedicated Server 和 Listen Server 差异。
- 网络延迟、丢包、重复输入、取消和校正的测试方法。

12. 配置、资产和编辑工作流
- HeroDefinition/PawnData、AbilitySet、InputConfig、WeaponDefinition、AmmoDefinition、AttachmentDefinition、SkillDefinition、CombatEntityDefinition、PresentationConfig 的关系。
- 每个配置回答一个问题，避免万能 DataAsset。
- 明确 DataAsset、PrimaryDataAsset、USTRUCT/FInstancedStruct、UObject 实例和 Actor 的选择标准。
- 软引用、Asset Manager、Data Validation 和异步加载边界。
- 第三方内容与 /Game/Apecox 项目自有内容隔离。
- 不为未来技能编辑器提前设计任意 Step VM。

13. UI、表现、调试和测试
- HUD 至少覆盖准星、武器、弹匣/备用弹药、瞄具、Health、Shield/Tier/Evolution、技能和交互提示。
- GameplayCue、Niagara、音效、镜头反馈与权威玩法分离。
- 日志分类、Debug Draw、网络角色显示、Ability/Tag/Attribute/Weapon 状态调试入口。
- 单人、Listen Server、Dedicated Server 的验证矩阵。
- 建议的 Functional Test、自动化测试和网络模拟用例。

【必须执行的压力测试场景】

架构草案完成后，至少逐条推演：
1. 玩家出生为空手，拾取步枪，装备、开火、换弹、丢弃。
2. 步枪使用轻型弹药，另一把步枪使用能量弹药，证明 AmmoType 不依赖 WeaponType。
3. 步枪切霰弹枪，再切狙击枪；切换过程中处理开火、换弹、跳跃和技能打断。
4. 拾取 2-4x 瞄具和弹匣配件，ADS 与武器参数更新，但 Definition 不被运行时修改。
5. 护盾受伤、破碎、使用电池恢复、造成伤害积累进化、升级 MaxShield。
6. 使用大招临时掏出火箭筒，结束后恢复之前武器、弹药、姿态和输入。
7. 使用治疗跟踪无人机，GA 结束后无人机继续独立运行并正确记录来源。
8. 持枪滑铲、滑铲跳、空中开火或被规则阻止，远端动画与移动同步正确。
9. 玩家死亡、装备掉落/清理、ASC 解绑、复活为空手并恢复模式规定的数据。
10. 在高延迟和丢包下开火、命中、换弹、切枪、技能和移动不发生双重结算。

每个场景都必须指出：
- 输入入口。
- 主要运行时对象。
- Authority/Prediction。
- 状态与 Tag 变化。
- 动画/表现。
- 复制数据。
- 失败、取消和清理。

【工作阶段】

阶段 A：证据盘点
- 读取 Apecox 当前文档。
- 定向检索 Lyra 源码和 UE 5.8 引擎接口。
- 盘点 D:/UE_Resource 中课程内容是源码参考还是仅美术资产。
- 搜索官方和可靠公开资料。
- 建立“事实、推断、Apecox 取舍”证据表。

阶段 B：架构候选比较
- 对关键分叉至少给出 2 个可行方案。
- 比较复杂度、网络风险、内容工作量、扩展性和求职展示价值。
- 证据充分时给出 Apecox V1 推荐方案、理由和升级条件。
- 证据不足时保留候选方案，不替用户猜测。

阶段 C：一致性压力测试
- 使用上面 10 个场景验证所有权、生命周期、配置、动画和网络链路。
- 修复职责冲突、循环依赖、万能配置、Tag 膨胀和重复生命周期。
- 压力测试只需证明框架拥有合理职责和扩展路径，不展开到类、函数和逐帧实现。
- 最多进行 3 轮内部架构审校，不无限迭代。

阶段 D：文档输出
- 只写一份总架构 RFC。
- 在 RFC 末尾集中列出仍需用户确认的问题、推荐理由和未验证风险。
- 不生成实现细节文档、自审文档或代码实施 Prompt。

【输出文件】

主文档：
D:/UnrealProject/Apecox/Agent/02_CombatFramework/Apecox_Combat_Framework_Architecture_RFC.md

必须包含：
- 执行摘要和非目标。
- 产品能力边界。
- 总体分层图。
- Gameplay Framework 网络存在/所有权矩阵。
- 核心运行时职责和配置层关系图。
- 武器、技能、护盾、移动、动画和双视角的职责边界。
- GAS 与非 GAS 系统边界。
- 核心状态/中断/切换模型。
- 网络权威与预测策略。
- 数据驱动和扩展策略。
- 普通玩法与特殊玩法分别如何复用、扩展的原则。
- 10 个压力测试场景暴露的架构问题和扩展性结论。
- 分阶段实施路线和每阶段可验证里程碑。
- 采用/轻量化采用/延后/拒绝的 Lyra 设计。
- 每项重要推荐的理由、代价、替代方案和改变推荐所需的条件。
- 后续专题讨论清单，并按依赖顺序排列。
- 风险、未知项和待用户确认问题。

【质量门槛】

只有同时满足以下条件，Goal 才算完成：
- 上述 13 个系统都有明确职责边界、运行时真相归属和扩展方向。
- 10 个压力测试场景都能说明公共框架如何容纳需求、特殊部分由哪类扩展机制承担，以及仍需讨论什么。
- 没有两个系统被设计为拥有同一份运行时真相。
- WeaponType 与 AmmoType 解耦得到数据和运行时模型支持。
- 公共武器能力与英雄特殊武器/CombatEntity 能复用明确接口。
- Health/Shield/Evolution 与伤害/恢复链路无职责冲突。
- 第一人称和第三人称方案明确，至少说明本地、服务端、远端分别运行什么。
- 滑铲移动明确归属 CharacterMovement，GAS 只承担合理的状态/规则职责。
- 预测能力没有把客户端结果当最终真相。
- 没有万能 GA、万能 DataAsset、任意 Step VM 或为每个特例增加 Tag 的倾向。
- 所有 Lyra 借鉴都能指出源码证据或明确标注推断。
- 每项重要推荐都解释“为什么推荐”，并说明替代方案和代价。
- 主 RFC 能被用户独立阅读，不依赖实现细节文档。
- 未解决问题被集中列出，不隐藏在多个文件中。
- 文档没有出现未经讨论的 Apecox 类名、函数、字段或最终 Tag 设计。

【停止条件】

满足以下任一条件时停止继续迭代：
1. 唯一的架构 RFC 已完成，并通过最多 3 轮内部审校，质量门槛全部满足。
2. 遇到无法从源码、官方资料、现有资产或合理假设解决的根本产品分叉；记录证据、候选方案和影响后，将其列为待用户确认，不继续猜测。
3. 某个研究方向连续两轮没有产生新的架构影响；停止扩展该方向。

Goal 完成后必须：
- 汇报最重要的 5-10 个架构结论。
- 链接唯一的架构 RFC。
- 显式列出待用户确认的问题。
- 明确说明“未进入类/函数详细设计，未修改业务代码和 UE 资产”。
- 停止工作并等待用户逐项讨论，不生成详细设计或实施 Prompt，不开始 Phase 1 代码。

【禁止事项】

- 不修改 Source、Config、Content、uproject 或插件。
- 不迁移 Aura/Apex 业务代码。
- 不创建 Apecox 候选类名、函数签名、成员变量、文件布局、字段表或代码伪实现。
- 不直接照搬 Lyra 完整 Experience、GameFeature、ModularGameplay。
- 不把课程代码当权威最佳实践。
- 不为追求完整而设计大逃杀、AI 或商业级内容管线。
- 不通过堆叠类、组件、DataAsset 或 GameplayTag 假装实现扩展性。
- 不把所有射击逻辑都塞进 GAS，也不把 GAS 降级成只播放动画的壳。
- 不把用户列出的每个玩法行为都写成当前阶段已经确定的具体方案。
- 无法确认的内容不猜测，必须转为有理由、有候选方案的后续讨论项。
- 不无限研究；最多 3 轮内部审校，达到停止条件后必须结束。
```

## 审阅提示

这份 Goal 获批后才会启动。启动时可以原样作为 `/goal` 的 objective，不设置额外 token budget。它只产出框架 RFC，后续类设计、配置字段、Tag、动画方案和网络实现分别讨论。
