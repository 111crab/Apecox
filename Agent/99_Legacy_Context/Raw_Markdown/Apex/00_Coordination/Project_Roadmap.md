# Apex 顶层路线图

生成日期：2026-07-14
维护规则：这里只维护顶层阶段，不记录每天的细碎任务。局部阶段和下一步看 `Current_Phase.md`。

## 设计原则

- 顶层阶段回答“项目大方向推进到哪里”。
- 局部阶段回答“眼前这一批工作要做什么”。
- 临时调研、想法、审查结果不塞进路线图；它们进入对应 Research / Review / Report。
- 参考项目、学习包、外部文章按需读取，不再作为硬前置阶段。

## Phase 0 - 项目与协作基线

目标：让 Apex 成为可持续协作的新项目，而不是 Aura 的混乱延续。

状态：基础编译、项目自建输入资产、Phase 角色与动画 PIE 基线均已通过；待在合适的小阶段提交并形成 Git 基线。

完成项：

- 新建 UE C++ 项目。
- 初始化 Git。
- 建立 `.agents/ue-project-context.md`。
- 建立 `Agent/` 文档结构。
- 迁移 Aura Agent 历史上下文。
- 生成工具与协作规则文档。

未完成项：

- 初始项目基线 commit / tag 的最终确认。

## Phase 0.5 - 角色、Paragon 资产与第三人称基线

目标：先把“人能动、镜头能转、角色能显示、资产可验证”做稳，再进入 GAS。

状态：Character 与 Phase 最小动画基线均已完成并通过 PIE 验证。

包含：

- `AApexCharacterBase` / `AApexPlayerCharacter` 基础 C++ 类。
- Paragon Phase 作为第一名验证英雄候选。
- 第三人称鼠标视角。
- 基础移动输入。
- 后续人工 UE 蓝图配置与 PIE 验证。

## Phase 1 - Paragon 英雄与动画验证

目标：基于 Paragon 角色和动画资源，建立可用于技能验证的英雄基础。

状态：第一名英雄 Phase 的项目自有 AnimInstance、BlendSpace、Locomotion 状态机和输入基线已经通过。Montage、Socket 与 VFX 不做无目标的全量盘点，后续随具体技能逐项确认。

包含：

- 确认 Paragon 资产可见、可打开、可播放。
- 选择第一批英雄原型方向。
- 盘点第一名英雄的 Montage / AnimSequence / Socket / VFX。
- 形成“动作语义 -> 技能候选”的映射。

## Phase 2 - Apex GAS / 技能系统 RFC

目标：写 Apex 专用技能系统 RFC，继承 Aura 经验但不复制 Aura 代码。

包含：

- 模块与目录边界。
- GameplayTag 命名规范。
- Attribute 最小集合。
- ASC 初始化和 Ability 授予方式。
- SkillDefinition / SkillRuntimeAbility / SkillTemplate / CombatEntity / Effect / State 的关系。
- 第一批技能原型与 Paragon 资产关系。

参考项目或学习包只在具体问题需要时读取。

## Phase 3 - GAS 基础运行时闭环

目标：让角色拥有 ASC、Attribute、输入激活 Ability 的最小闭环。

包含：

- 玩家角色 ASC 初始化。
- AttributeSet 初始化。
- 输入触发 Ability。
- 一个最小 Ability 激活和结束。
- 单人 PIE 验证。
- Listen Server + Client 基础验证。

## Phase 4 - 第一批技能原型

目标：逐个实现代表性技能，每个技能单独设计、实施、验证、审查。

候选方向：

- ProjectileCast：火球 / 能量弹。
- ChannelCast：引导治疗。
- AreaField：范围持续场。
- ChargeCast：蓄力射击。
- Beam / Link：光束或链接。
- PassiveListener：被动反击或护盾。

## Phase 5 - 技能编辑器预研

目标：运行时稳定后，再定义技能编辑器边界。

长期方向：

- 类比 Montage 编辑器。
- 细节面板配置技能。
- 预览场景播放动画、Cue、实体生成和效果表现。
- 保存为配置资产，不生成每技能 C++ 类。
