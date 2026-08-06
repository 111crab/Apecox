# 任务池

生成日期：2026-07-14

## Phase 0 - 新项目基线

目标：创建干净 UE 项目，建立最小工程和协作基线。

- [ ] 新建 UE C++ 项目。
- [ ] 项目能编译。
- [ ] 项目能 PIE。
- [ ] 初始化 Git。
- [ ] 提交 `Initial clean UE project`。
- [ ] 启用最小插件：GameplayAbilities、GameplayTasks、EnhancedInput、Niagara。
- [ ] 提交 `Enable core gameplay plugins`。
- [ ] 生成 `.agents/ue-project-context.md`。
- [ ] 创建轻量 `Agent/` 文档结构。

## Phase 1 - Lyra / 规范项目研究

目标：先理解成熟项目结构，再设计自己的小型版本。

- [ ] 获取 Lyra 或参考项目代码。
- [ ] 阅读模块结构。
- [ ] 阅读 ASC 初始化路径。
- [ ] 阅读 AbilitySet / 输入 / GameplayTag / GameplayCue 组织。
- [ ] 输出 `Research_Notes/Lyra_Reading_Notes.md`。
- [ ] 总结适合新项目采用的设计。
- [ ] 总结不适合直接采用的复杂设计。

## Phase 2 - 新技能系统架构 RFC

目标：写出新项目第一版技能系统架构，不急着实现。

- [ ] 定义模块边界。
- [ ] 定义 GameplayTag 命名规范。
- [ ] 定义 Attribute 最小集合。
- [ ] 定义 ASC 初始化和 Ability 授予方式。
- [ ] 定义 SkillDefinition / AbilityTemplate / CombatEntity / Effect / Buff / State 的关系。
- [ ] 定义第一批技能原型。
- [ ] 用户审阅并确认 RFC。

## Phase 3 - 基础运行时闭环

目标：建立角色、输入、ASC、属性、技能激活的最小闭环。

- [ ] 玩家角色拥有 ASC。
- [ ] AttributeSet 可初始化。
- [ ] 输入可触发 Ability。
- [ ] 一个最小 Ability 可激活和结束。
- [ ] 单人 PIE 验证。
- [ ] Listen Server + Client 基础验证。

## Phase 4 - 第一批技能原型

目标：逐个实现代表性技能，每个技能单独设计、实现、验证。

- [ ] ProjectileCast：火球 / 能量弹。
- [ ] ChannelCast：引导治疗。
- [ ] AreaField：范围持续场。
- [ ] ChargeCast：蓄力射击。
- [ ] Beam / Link：光束或链接。
- [ ] PassiveListener：被动反击或护盾。

## Phase 5 - 技能编辑器预研

目标：运行时稳定后，再讨论技能编辑器。

- [ ] 定义技能编辑器边界。
- [ ] 定义预览场景。
- [ ] 定义配置资产保存方式。
- [ ] 定义 Montage / Timeline / GameplayCue 预览关系。

