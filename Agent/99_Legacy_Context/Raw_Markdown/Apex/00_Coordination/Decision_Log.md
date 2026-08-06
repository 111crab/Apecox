# 决策记录

生成日期：2026-07-14

这里记录新项目长期有效的决策。不要把普通讨论写进来。

## 2026-07-14 - 新项目从干净 UE C++ 项目开始

**决策：** 新项目不直接迁移 Aura 代码，从干净 C++ 项目开始构建技能系统。

**原因：** Aura 承载了教程代码、旧 GAS 路径、中间试验和资产混杂状态，不适合作为长期技能系统地基。

**影响：**
- Aura 只作为经验库。
- 新项目重新设计模块、命名、Tag、Attribute、AbilitySet、SkillDefinition。
- 旧代码只在必要时作为参考，不复制粘贴。

## 2026-07-14 - 先读 Lyra，再写核心架构

**决策：** 新项目正式写技能系统核心代码前，先结构化阅读 Lyra 或同类规范项目。

**原因：** Lyra 在 GAS、输入、AbilitySet、PawnData、GameFeature、GameplayCue 等方面有较成熟实践，适合作为设计参考。

**影响：**
- 第一阶段工作是研究和架构 RFC，不急着写技能。
- 只吸收适合小项目的部分，不照搬全套复杂体系。

## 2026-07-14 - 技能系统采用配置驱动 + 原生模板

**决策：** 长期方向为 `SkillDefinition + AbilityTemplate + CombatEntity + Effect/Buff/State`，而不是每个技能一个完全独立 GA。

**原因：** 这样更适合技能编辑器、复用常见生命周期，并保留 C++ 对网络、预测和性能的控制。

**影响：**
- 技能配置是组装入口，不是万能字段表。
- 特殊机制通过模板、任务、片段或少量 C++ 扩展实现。
- 第一版避免完整 SkillGraph / VM。

## 2026-07-14 - 文档轻量化

**决策：** 新项目只维护少量高价值文档。

**原因：** Aura 阶段文档过多，降低了沟通和执行效率。

**影响：**
- 普通讨论不新建 md。
- 只有长期决策、当前任务、执行 Prompt、实施报告、审查结论、已完成技能链路文档才落盘。

## 2026-07-14 - 非批量 UE 编辑器操作默认人工

**决策：** 单个蓝图创建、设置 Mesh、设置 AnimClass、改 GameMode、添加单个 Notify 等操作默认人工完成，不主动使用 MCP。

**原因：** Aura 阶段验证表明 MCP 对这类需要人眼确认和编辑器状态验证的操作不可靠。

**影响：**
- MCP 主要用于批量扫描、批量检查、批量报告。
- MCP 操作结果必须通过 UE 编辑器可见、可打开、可编译验证。
- ClaudeCode Prompt 必须要求失败时输出人工清单。


## 2026-07-14 - Paragon 资产作为英雄设计基础

**决策：** Apex 第一阶段全面采用 Paragon 资产包中的角色、地图、动画、Montage 和表现资源作为英雄与技能验证基础。

**原因：** 技能系统需要真实的角色动作、地图空间、镜头距离和表现资源约束。Paragon 资产能提供完整第三人称英雄动作语义，避免 Aura 阶段因为动画和表现不足导致技能设计悬空。

**影响：**
- 英雄设计优先从 Paragon 角色和动画出发。
- 技能设计尊重已有 Montage、AnimSequence、Socket、VFX 和地图空间。
- 不为抽象技能硬造不存在的动画；先从可播放的动作语义反推技能候选。
- 资产迁移、角色替换、动画蓝图绑定等非批量 UE 编辑器操作默认给人工步骤，不主动使用 MCP。

## 2026-07-14 - Apex 采用第三人称鼠标视角

**决策：** Apex 采用第三人称项目形态，鼠标负责摄像机视角转动，后续准星、目标选择、投射物、光束和镜头方向都围绕该视角设计。

**原因：** 这个视角既适合 Paragon 英雄资产，也适合验证法术、远程、近战和范围技能的通用框架。

**影响：**
- 输入层优先建立 Move / Look 分离的 Enhanced Input 基线。
- 移动层需要明确角色朝向策略：探索期可先保留模板式移动朝向，技能瞄准期再引入准星/镜头方向约束。
- 先完成基础移动、镜头和动画可播放，再进入 GAS 技能实现。

## 2026-07-14 - Montage 驱动技能候选设计

**决策：** 第一阶段技能和英雄设计采用“先盘点 Montage / 动作语义，再设计技能原型”的流程。

**原因：** Montage 决定了释放姿态、关键帧、Notify 时机和动作可信度。先看资产再定技能，能减少无效设计和后期返工。

**影响：**
- 每个候选英雄先建立 Montage 目录。
- 技能原型要说明依赖哪个 Montage、在哪个关键帧触发 GameplayEvent、需要哪些 Socket / VFX / CombatEntity。
- 长期技能编辑器可以预留 Timeline，但早期仍以 Montage Notify / GameplayEvent 验证技能触发点。

## 2026-07-14 - Aura Agent 上下文迁移为 Apex 协作基线

**决策：** 将 Aura `Agent/` 中的重要协作规则、技能系统讨论、风险清单、技能编辑器愿景、Prompt/报告组织方式迁移到 Apex。

**原因：** Apex 是新项目，但不应丢失 Aura 阶段已经讨论出的工作流程、命名审阅习惯、学习型注释要求、MCP 使用经验和技能系统架构经验。

**影响：**
- Aura 原始上下文完整归档到 `Agent/Aura_Legacy_Archive/`。
- Apex active 文档继续以 `00_Coordination/Decision_Log.md`、`00_Coordination/Task_Backlog.md`、`00_Coordination/Working_Agreement.md`、`00_Coordination/Tooling_Guide.md` 为准。
- Aura 旧类名、旧路径、旧资产不直接迁移；只迁移经验和设计口径。
- 后续如果要继承 Aura 某个设计，必须先写入 Apex 当前文档或 RFC，再实施。

## 2026-07-14 - 参考项目按需读取，不再作为硬前置

**决策：** Lyra、学习包、外部文章和第三方示例项目不再作为每个阶段的主动前置阅读材料。只有当当前问题确实需要参考成熟实现、用户明确要求，或 Prompt 明确要求时才读取。

**原因：** Apex 当前需要先完成项目冷启动、角色基线和资产验证。过早读取大量参考项目会拖慢执行，并可能把过大框架误带入小项目。

**影响：**
- `Project_Roadmap.md` 不把 Lyra 阅读作为硬性 Phase 前置。
- 参考项目阅读必须说明目的、借鉴点和不借鉴点。
- 已有旧决策中“先读 Lyra”改为“需要时读 Lyra 或其他参考项目”。

## 2026-07-14 - 当前阶段与代码设计拆分维护

**决策：** Apex 不使用单一 `Current_Work.md` 混合记录阶段推进和代码设计，而是拆分为 `Current_Phase.md` 与 `Current_Code_Design.md`，并由 `Reviews/` 承接实施后审查。

**原因：** 阶段推进关注下一步、待确认、验证和 Git；代码设计关注实施前约定的大类、关键变量、函数、Tag、属性和配置结构；实施后出现的偏离或新增点应进入审查记录。混在一个文件里会降低日常沟通效率。

**影响：**
- 日常推进和当前待确认优先看 `Agent/00_Coordination/Current_Phase.md`。
- 实施前 C++ / Tag / 属性 / 配置命名预案看 `Agent/00_Coordination/Current_Code_Design.md`。
- `Agent/00_Coordination/Pending_Questions.md` 已删除；当前待确认事项统一写入 `Agent/00_Coordination/Current_Phase.md`。
- 子代理完成代码任务后，Codex 必须把命名表和人工 UE 操作步骤整理给用户审阅。

## 2026-07-14 - 新长期 C++ 代码采用 Public / Private 模块目录

**决策：** Apex 后续新增的长期 gameplay C++ 代码默认使用 UE 模块目录规范：公共头文件放 `Source/Apex/Public/...`，实现文件放 `Source/Apex/Private/...`。

**原因：** 这能让“其他模块或蓝图可见的接口”和“模块内部实现”边界更清楚，也符合 UE 模块长期维护方式。旧模板文件暂时不批量搬迁，避免无意义的大改动。

**影响：**
- 新 `UCLASS` 公共头文件进入 `Public` 后应带 `APEX_API`。
- `.cpp` 引用本模块公共头时优先使用 `Character/ApexPlayerCharacter.h` 这类从 `Public` 根开始的路径。
- 子代理 Prompt 后续应明确 `Public` / `Private` 目标路径。
- 旧模板类和 Variant 示例代码只有在清理阶段再统一处理。

## 2026-07-14 - Agent 活跃文档集中到 00_Coordination

**决策：** Apex `Agent/` 根目录只保留 `README.md`，活跃协作文件统一放到 `Agent/00_Coordination/`。删除 `Pending_Questions.md` 和 `04_AgentHandoff/`，当前待确认事项统一写入 `Current_Phase.md`，子代理交接统一使用 `Prompts/Reports/Reviews`。

**原因：** 根目录散落文档、`Pending_Questions.md` 与 `Current_Phase.md` 双入口、`04_AgentHandoff/` 与 `Prompts/Reports/Reviews` 双入口，会让用户不知道该看哪里。

**影响：**
- 用户日常优先看 `Agent/00_Coordination/Current_Phase.md`。
- 实施前设计看 `Agent/00_Coordination/Current_Code_Design.md`。
- 实施后审查看 `Agent/Reviews/`。
- 长期决策、任务池、协作规则、工具指南都在 `Agent/00_Coordination/`。
- 清理报告见 `Agent/Reports/2026-07-14_Agent_Structure_Cleanup_Report.md`。

## 2026-07-15 - 第三方 Paragon 原始资产不进入普通 Git

**决策：** `Content/ParagonPhase/` 作为本机安装的第三方内容包，不纳入 Apex 普通 Git 提交；项目自建蓝图、地图、配置资产仍需版本控制。

**原因：** Paragon 原始资产体积大、可由资产来源重新安装；但项目自建资产承载角色配置和运行基线，必须进入仓库才能还原项目工作成果。

**影响：**
- `.gitignore` 明确排除 `/Content/ParagonPhase/`。
- `BP_Hero_Phase` 等项目资产可以硬引用该内容包，但文档必须说明外部依赖。
- 从 GitHub 拉取项目后，需要另行安装相同 Paragon Phase 资产才能完整显示角色。

## 2026-07-15 - GAS RFC 前先做 Lyra 定向调研

**决策：** Character 冷启动通过后，先针对 ASC 所有权、初始化生命周期、AbilitySet、InputTag 路由、Ability 激活关系和属性职责研究 Lyra，再编写 Apex GAS RFC。

**原因：** 这些属于 GAS 基础架构的长期决策，若先写代码再补充成熟项目经验，容易重复 Aura 阶段的重构成本。

**影响：**
- Lyra 阅读是当前 GAS RFC 的明确前置，但不是对整个项目永久生效的重型前置阶段。
- 第一轮只研究与 Apex 当前决策直接相关的源码，不完整复制 Experience 或 GameFeature。
- 研究结论必须说明 Apex 采用完整版、轻量版或拒绝，并由用户确认后才能进入实施。

## 2026-07-15 - Git 以功能或小阶段为提交单位

**决策：** Apex 默认按功能闭环或可验证的小阶段提交，不再为普通开发改动进行逐文件的重型分类和频繁提交。

**原因：** Git 的目标是保留可恢复节点和支持协作，不能让版本管理成本压过架构、实现与验证本身。

**影响：**
- 提交前采用一次快速状态检查和范围暂存。
- 仅对第三方大资产、生成目录、明显无关改动或来源不明内容进行额外分类。
- Push 优先安排在当日结束、阶段完成或需要远端备份时。

## 2026-07-15 - 技能框架不以万能配置或万能 GA 为目标

**决策：** Apex 追求常见技能快速配置、局部机制可复用、特殊技能可自然扩展；不把“一个 DataAsset 配出所有技能”或“一个 GA 执行所有流程”当作架构成功标准。

**原因：** 游戏技能差异不仅是数值和资源差异，还可能改变异步时序、预测方式、持续实体和跨系统交互。强行字段化会制造万能资产，强行统一 GA 会削弱 GAS 原生 AbilityTags 和生命周期边界。

**影响：**
- 公共差异优先使用模板配置。
- 可复用异步行为使用 AbilityTask。
- 生成后持续存在的机制归 CombatEntity / Effect / Buff 子系统。
- 完全特殊的生命周期允许专用 GA，但仍复用公共基类和基建。
- Step 暂时作为分析语言和未来有限 Timeline 概念，不实现任意 Step 解释器。
- 详细分析见 `Agent/Research_Notes/2026-07-15_Skill_Framework_Extensibility_Synthesis.md`。

## 2026-07-15 - Lyra GAS 第一轮基础取舍已确认

**决策：** 用户确认第一轮 Lyra 研究的八项基础取舍，并要求在配置驱动与特殊扩展方式上继续讨论。

**影响：**
- 玩家 ASC 采用 PlayerState Owner / Pawn Avatar，AI ASC 由自身 Character 持有。
- Native Tag 使用 `namespace ApexGameplayTags`，不创建项目 Tag 单例。
- InputTag 由 AbilitySet / Loadout 分配，不作为 SkillDefinition 固定字段。
- 采用轻量 ActivationPolicy、并发组和独立关系映射思想。
- Experience、GameFeature、完整 ModularGameplay 初始化链暂不采用。
- V1 使用标准 GameplayCueManager 和标准 EffectContext。
- Spec-aware Policy 仅作为共享 GA 模板下的条件性扩展，V1 不提前实现。

## 2026-07-15 - 技能框架采用三类验证样本

**决策：** 架构验证采用“普通投射物 + 穿透/分裂局部变体 + 黑洞”。

**原因：** 三者分别验证模板基础配置、同模板局部行为复用，以及 CombatEntity 持续机制与特殊交互边界。

## 2026-07-15 - 建立 Apex 自有 AnimBP

**决策：** Apex 将建立项目自有 AnimInstance / AnimBP，不把 Paragon Phase 原有 AnimBP 作为长期动画实现；Phase 动画素材继续复用。

**影响：**
- C++ AnimInstance 负责稳定状态缓存和后续 GAS Tag 桥接。
- AnimBP 负责 Locomotion、姿势混合和 Montage Slot，不负责技能结算。
- 第一版具体范围和命名仍需用户审阅。
- 不同 Skeleton 仍需 Retarget 和对应的 AnimBP / Montage 资产，不能只靠替换配置跨骨骼复用。

## 2026-07-15 - 下次 UE 操作切换项目自有 InputAction

**决策：** `/Game/Blueprints/Input/Actions/IA_*` 是项目自有输入资产，保留并在下一次 UE 人工操作中切换使用。

**影响：** 人工清单必须同时替换角色的 InputAction 引用和 Input Mapping Context 中的映射，避免角色监听新 IA、IMC 仍触发旧 IA。

## 2026-07-15 - SkillDefinition 采用受约束的 FInstancedStruct 执行配置

**决策：** Apex 的每个技能保留一个统一 `SkillDefinition` 根资产，模板特殊配置通过一个受约束的 `FInstancedStruct` 内联；该形态与漫威争锋演讲展示的 AbilityAsset + Inline AbilityConfig 更接近。

**边界：**
- 只有一个 ExecutionConfig，不允许任意 Fragment 数组。
- Config 必须继承统一基结构，并与 GA 模板严格匹配。
- 类型匹配由 Details 过滤、Data Validation 和运行时入口共同校验。
- CombatEntity、Effect、Cue 和关系映射仍保持独立职责，不被内联结构吞并。
- 借鉴配置形态，不复制漫威争锋的 AAA 规模、类名和完整管线。

## 2026-07-16 - 项目自建 UE 资产统一放在 /Game/Blueprints

**决策：** Apex 自行设计和创建的 UE 内容统一放在 `/Game/Blueprints/`；第三方原包保持各自目录，不直接修改。

**当前落点：**
- Phase 动画资产：`/Game/Blueprints/Characters/Phase/Animation/`。
- 项目自有 InputAction：`/Game/Blueprints/Input/Actions/`。
- 基础移动 Mapping Context：`/Game/Blueprints/Input/IMC_Apex_BaseMove`。

**原因：** 让项目自建内容与 `/Game/ParagonPhase/`、模板内容和其他第三方资源保持清晰边界，便于 Git、迁移和后续替换。

## 2026-07-21 - 保留并重构现有 Apex Gameplay Framework 外壳

**决策：** 保留项目已经存在的 `AApexPlayerController` 与 `AApexGameMode` 类名，不再创建职责重复的原生类；两者在 GAS Framework 首批实施时按长期目录和职责重构。

**同时确认：**
- 玩家英雄采用 `PlayerState` 持有 ASC、Character 作为 Avatar；AI Character 同时作为 ASC Owner 与 Avatar。
- 新增项目自有 `AApexPlayerState`，具体成员和文件路径在实施前单独审阅。
- `BP_Apex_PlayerController` 与 `BP_Apex_GameMode` 在 GAS 基础 RFC 通过后的首个 Gameplay Framework 实施批次创建，不提前复制当前模板蓝图。
- PlayerController 负责本地输入、UI/InputMode 和技能输入意图转交，不保存战斗状态。
- GameMode 负责服务端生成、规则与 Framework 类选择，不承载技能逻辑。

**原因：** 当前两个原生类已经提供可继续演进的类型身份；提前创建新蓝图只会重复模板配置，并在 PlayerState、ASC 和 InputTag 链确定后再次返工。

## 2026-07-23 - AttributeSet 按资源结算与战斗参数拆分

**决策：** Apex 采用无业务属性的 `UApexAttributeSet` 公共基类、负责生命与法力结算的 `UApexVitalAttributeSet`，以及后续按正式伤害模型创建的 `UApexCombatAttributeSet`。

**同时确认：**

- 第一批只实现 `Health`、`MaxHealth`、`Mana`、`MaxMana`。
- 第一批使用 C++ 默认值 100 验证 GAS 和网络链路，Startup GE 后续再引入。
- `Health/MaxHealth` 向所有相关客户端复制；`Mana/MaxMana` 只向拥有者复制。
- `MaxHealth` 下限为 1；`MaxMana` 下限为 0，以兼容无蓝量英雄。
- 不预先加入力量、智力等主属性；物理强度、法术强度、护甲、抗性和穿透在伤害模型确定后进入 `UApexCombatAttributeSet`。
- 单次最终伤害不作为持久 Attribute；未来通过 ExecCalc 和 `IncomingDamage/IncomingHealing` 等 Meta Attribute 进入资源结算。

**原因：** AttributeSet 应按稳定的数据不变量、复制边界和结算职责拆分，而不是按技能或临时字段分类。该方案吸收 Lyra 的来源参数/目标结算边界，同时保持适合 Apex 规模的简洁结构。

## 2026-07-23 - AbilitySet 定位为技能授予清单

**决策：** Apex 的 AbilitySet 是面向 ASC 的只读授予清单，不是技能定义、英雄定义或运行时状态容器。

**分层：**

```text
HeroDefinition / 装备 / 模式
    -> AbilitySet
        -> SkillDefinition
            -> UApexGameplayAbility 及其模板子类
```

**同时确认：**

- `UApexGameplayAbility` 是所有 Apex GA 的抽象根基类，不是万能技能实现。
- `UApexSkillDefinition` 是单个技能的配置入口，第一版只保存 `AbilityTemplateClass`。
- AbilitySet 条目引用 SkillDefinition，并在授予时指定 AbilityLevel 与可选 InputTag。
- SkillDefinition 作为 AbilitySpec SourceObject，使同一 GA 模板能够服务多个技能定义。
- InputTag 属于装配槽位，不属于技能固有身份。
- AbilitySet 资产不保存 Handle；运行时 `GrantedHandles` 由授予对象持有并负责撤销。
- 第一版 AbilitySet 只授予 GA，不动态授予 GE 或 AttributeSet。
- 普通攻击、小技能一、小技能二和大招使用语义 Tag；Q/E/X 只存在于 IMC 物理映射。

**来源与取舍：**

- 借鉴 Lyra 的 AbilitySet 授予、Handle 回收、ActivationPolicy 和输入队列。
- 借鉴漫威争锋 AbilityAsset/AbilityConfig 的“稳定根配置 + 模板专属配置”思想。
- 保留 Apex 已确认的 AbilityTemplate、AbilityTask、CombatEntity 和特殊 GA 扩展边界。
- 不复制 Lyra 的完整 PawnData/GameFeature/Experience，也不实现任意 Step 解释器。

## 2026-08-05 - Energy Bolt 动画修正与第三人称手感优化方向

**已验证事实：** Phase Energy Bolt 的角色胶囊朝向、手部 Socket 生成位置、ViewCenter TargetData、投射物飞行方向和命中结果均正确。当前问题仅存在于动画表现：Montage 播放时上半身看起来向角色右侧施法。

**动画处理决策：**

- 不修改角色 Yaw、Socket、TargetData 或投射物方向来补偿动画偏斜。
- 优先在项目自有 `AM_Apex_Phase_EnergyBolt` 中更换为正面施法动作，候选依次检查 `Cast`、`Ability_Q`、`Primary_Attack_A_InMotion_Medium`。
- 保持 `UpperBody` Slot 和现有 GameplayEvent 驱动方式，在新动作的真实出手帧重新放置 `GameplayEvent.Ability.Execute`。
- 如果源动画正面正常、进入 `ABP_Apex_Phase` 后才偏斜，再检查 `Layered Blend Per Bone` 的 `spine_01`、连接方向和 Mesh Space Rotation Blend。
- Phase 已有 `Attack_AO`，但 AimOffset 只作为上述处理无效后的下一层方案，不用于掩盖错误 Montage。

**移动手感首轮基线：**

| 参数 | 目标值 |
|---|---:|
| `MaxWalkSpeed` | 500 |
| `MaxAcceleration` | 3000 |
| `GroundFriction` | 8 |
| `BrakingFrictionFactor` | 1 |
| `BrakingDecelerationWalking` | 2000 |
| `GravityScale` | 1.6 |
| `JumpZVelocity` | 580 |
| `AirControl` | 0.45 |
| `BrakingDecelerationFalling` | 200 |

该组数值是第一轮可验证基线，不是永久常量。调优重点是启动、换向、停止、跳跃高度、滞空时间和空中动量；继续使用默认 `UCharacterMovementComponent` 的网络预测，不提前创建自定义移动组件。

**摄像机首轮基线：**

| 参数 | 目标值 |
|---|---:|
| `TargetArmLength` | 380 |
| `TargetOffset` | `0, 0, 60` |
| `SocketOffset` | `0, 65, 0` |
| `FollowCamera FOV` | 85 |
| `Do Collision Test` | true |
| `ProbeSize` | 12 |
| `ProbeChannel` | Camera |
| Camera Location / Rotation Lag | false / false |
| `ViewPitchMin / ViewPitchMax` | `-65 / 60` |

摄像机继续采用 SpringArm。后续实施时可新增薄层 `AApexPlayerCameraManager` 管理 FOV、Pitch 边界和未来 Camera Shake 入口；当前不复制 Lyra 的完整 CameraMode Stack，等蓄力缩放、锁定或技能镜头出现真实需求后再扩展。

**阶段状态：** 上述优化方向已获用户批准，但尚未实施。当前先收口已验证的第三人称瞄准基线，随后切换到项目级讨论；手感优化在项目级结论明确后再恢复。
