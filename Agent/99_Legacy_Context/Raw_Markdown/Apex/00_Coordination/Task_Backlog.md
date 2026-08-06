# 任务池

生成日期：2026-07-14

维护规则：这里是跨阶段任务池，只记录“有哪些事”和“是否完成”。不要在这里写当前执行状态、人工 UE 步骤、验证标准、Git 建议或待确认细节；这些放到 `Agent/00_Coordination/Current_Phase.md`。

## Phase 0 - 新项目基线

目标：创建干净 UE 项目，建立最小工程和协作基线。

- [x] 新建 UE C++ 项目。
- [x] 项目能编译。
- [ ] 项目能 PIE。
- [x] 初始化 Git。
- [x] 提交初始项目基线：`Initial Apex project baseline`。
- [ ] 启用最小插件：GameplayAbilities、GameplayTasks、EnhancedInput、Niagara。
- [ ] 提交 `Enable core gameplay plugins`。
- [x] 生成 `.agents/ue-project-context.md`。
- [x] 创建 `Agent/` 文档结构。
- [x] 写入工具与 Skill 使用指南 `Tooling_Guide.md`。
- [x] 清理 `Agent/` 冗余结构：活跃文档集中到 `00_Coordination/`，移除 `Pending_Questions.md` 与 `04_AgentHandoff/`。
- [x] 当前 Codex 窗口工作重心切换到 Apex。
- [x] 迁移 Aura Agent 历史上下文与协作组织。
- [x] 建立顶层路线图、当前阶段、当前代码设计的拆分维护方式。

## Phase 0.5 - Paragon 资产与第三人称验证基线

目标：先把项目体验和资产基础定稳，再进入 GAS / 技能系统 RFC。

- [x] Character 冷启动：新增 `AApexCharacterBase` / `AApexPlayerCharacter`。
- [x] Codex 审查 Character 冷启动实现。
- [ ] 用户审阅新增类名、成员变量、函数命名和职责。
- [ ] 用户人工创建/配置 `BP_Hero_Phase`。
- [x] 编译通过。
- [ ] 单人 PIE 验证移动、跳跃、鼠标视角。
- [ ] 确认 Paragon 资产包在 Apex 中可见、可打开、可播放。
- [ ] 盘点 Paragon 角色、地图、动画、Montage、VFX 的第一批可用资产。
- [ ] 选择第一批英雄原型方向：法术 / 远程 / 近战 / 坦克或辅助。
- [ ] 为第一名英雄建立 Montage 目录，并记录每个 Montage 可能对应的技能语义。
- [ ] 从 Montage 目录反推 3-5 个第一批技能候选。

## Phase 1 - Paragon 英雄与动画验证

目标：基于 Paragon 资源建立可用于技能验证的英雄基础。

- [ ] 确认第一名验证英雄。
- [ ] 确认是否使用自带 AnimBP 或临时僵硬移动。
- [ ] 盘点 Montage / AnimSequence / Socket / VFX。
- [ ] 输出第一名英雄的技能候选草案。

## Phase 2 - Apex GAS / 技能系统 RFC

目标：写出新项目第一版技能系统架构，不急着实现。

- [ ] 定义模块边界。
- [ ] 定义 GameplayTag 命名规范。
- [ ] 定义 Attribute 最小集合。
- [ ] 定义 ASC 初始化和 Ability 授予方式。
- [ ] 定义 SkillDefinition / SkillRuntimeAbility / SkillTemplate / CombatEntity / Effect / State 的关系。
- [ ] 定义第一批技能原型。
- [ ] 用户审阅并确认 RFC。

## Phase 3 - GAS 基础运行时闭环

目标：建立角色、输入、ASC、属性、技能激活的最小闭环。

- [x] 玩家角色拥有 ASC。
- [x] AttributeSet 可初始化。
- [x] 输入可触发 Ability。
- [x] 一个最小 Ability 可激活和结束。
- [x] 单人 PIE 验证。
- [x] Listen Server + Client 基础验证。

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
