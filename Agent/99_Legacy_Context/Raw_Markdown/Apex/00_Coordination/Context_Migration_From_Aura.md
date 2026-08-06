# 从 Aura 迁移来的项目上下文

生成日期：2026-07-14

## 迁移目的

Aura 是一个从 UE C++ / GAS 教程项目演化出来的实验项目。它帮助我们讨论和试错了很多技能系统设计问题，但它也积累了教程代码、旧 GAS 路径、旧资产、MCP 试验痕迹和不稳定中间实现。

新项目的目标不是复制 Aura，而是重新实践这些已经确认的理念。

## 顶层目标

构建一个基于 Unreal Engine GAS 的通用技能系统，长期目标包括：

- 支持市面上常见的技能机制。
- 技能由配置资产组装和修改。
- 少量通用 C++ 能力支撑大量技能，而不是每个技能一套孤立代码。
- 支持动画、VFX、音效、UI、目标选择、多人同步。
- 长期构建一个技能编辑器，提供配置、预览、保存、运行时使用能力。

## 核心架构思想

### 技能不是简单分主动 / 被动

更有价值的划分是技能生命周期：

- Instant：瞬发效果。
- ProjectileCast：播放动画，等待关键帧，生成投射物。
- ChannelCast：按住、引导、释放、完成或取消。
- ChargeCast：蓄力，按蓄力时间改变效果。
- AreaField：生成范围场，持续检测，周期结算。
- Beam / Link：持续连接、追踪、断开。
- Combo：连段窗口和阶段切换。
- PassiveListener：监听事件并触发效果。
- WeaponFire：持续开火、弹药、后坐力、准星/射线。

### SkillDefinition 是技能组装入口

配置资产不是“万能字段表”，而是技能组装入口。它应该引用：

- 身份信息：名称、图标、描述、技能 Tag。
- 输入与激活策略。
- 成本与冷却。
- 技能模板或生命周期模板。
- 表现配置。
- 衍生物配置，例如投射物、范围场、光束。
- 效果配置，例如伤害、治疗、Buff、Debuff。

### 技能模板处理大流程

模板不是每个技能的静态代码类，而是常见生命周期的复用执行框架。

例如：

- ProjectileCast 模板：激活 -> 目标选择 -> 播放 Montage -> 等待事件 -> 生成 CombatEntity.Projectile -> 命中后应用效果。
- ChannelCast 模板：激活 -> 锁定移动策略 -> 等待释放或完成 -> 根据 Outcome 应用不同效果。
- AreaField 模板：激活 -> 选择位置 -> 生成 CombatEntity.AreaField -> 周期检测 -> 生命周期结束。

### CombatEntity 是战斗衍生物

CombatEntity 用来统一描述角色以外的战斗载体：

- Projectile：火球、箭、飞刀、能量弹。
- AreaField：火圈、治疗区域、毒雾、陷阱区域。
- Beam / Link：光束、锁链、持续连接。
- Summon / Trap：后续阶段再做。

CombatEntity 应该有自己的配置，包括生成方式、生命周期、检测规则、命中规则、表现资源和效果应用策略。

### Buff / Effect / State 三层思想

保留三层思路，但新项目重新设计：

- State：最原子的状态或限制，例如不能移动、沉默、眩晕、无敌。
- Effect：共性效果模板，例如造成伤害、治疗、加速、施加 State、驱散。
- Buff：具体技能或来源创建的实例，带生命周期、层数、周期、来源、目标和参数。

GAS 的 GameplayEffect 可以承载部分 Effect/Buff 能力，但不必把所有设计概念都强行塞进一个 GE 类。

## Montage / GameplayEvent 决策

V1 可以继续使用 Montage Notify 触发 GameplayEvent。

职责划分：

- Montage 负责动画轨道和“哪一帧发生”。
- GameplayEvent Notify 负责把关键帧事件发送给 Ability。
- SkillDefinition 负责声明“等待哪个事件 Tag，以及事件来了之后做什么”。

长期可以设计 Skill Timeline，但不要在第一阶段就做完整图编辑器或时间轴系统。

## 技能编辑器长期目标

技能编辑器类比 Montage 编辑器：

- 通过细节面板或图形界面配置技能。
- 能在预览场景中播放技能表现。
- 能保存技能配置资产。
- 进入 PIE 后技能可使用。
- 不为每个技能生成静态 C++ 类。

第一阶段不做技能编辑器。先保证运行时模型足够干净。

## 第一批值得验证的技能类型

推荐新项目第一批技能原型：

1. 火球 / 能量弹：ProjectileCast。
2. 引导治疗：ChannelCast。
3. 范围场：AreaField。
4. 蓄力射击：ChargeCast。
5. 光束 / 链接：Beam or Link。
6. 被动反击 / 护盾：PassiveListener。

每个技能单独设计、实现、验证、写运行链路文档。

## Lyra 研究重点

新项目写核心代码前，应先阅读 Lyra 或同等规范项目，重点看：

- ASC 初始化位置。
- AbilitySet 如何授予 Ability / Effect / Attribute。
- 输入如何映射到 Ability。
- GameplayTag 命名和注册方式。
- PawnData / Experience / GameFeature 的分层。
- AbilityTask 使用边界。
- GameplayCue 的组织方式。
- 多人预测和表现同步策略。

目标是吸收思想，不是照搬 Lyra 全套复杂度。

## 不迁移的内容

不要迁移：

- Aura 旧 C++ 代码。
- Aura 的旧 `UAuraConfiguredActiveAbility` 等实现细节。
- Aura 脏工作区。
- Aura MCP 插件。
- Aura 的 `.agents/ue-project-context.md`。
- 旧项目资产路径，除非后续明确用 UE Migrate 迁移具体资产。

## 保留的教训

1. 文档过多会降低沟通效率。
2. MCP 不适合单个蓝图创建、Mesh 设置、AnimClass 设置、GameMode 设置。
3. UE 资产创建是否成功，必须以编辑器可见、可打开、可编译为准。
4. 没有编译 / PIE 验证，不能把阶段标记为完成。
5. Prompt 必须包含失败时的人工操作清单要求。

