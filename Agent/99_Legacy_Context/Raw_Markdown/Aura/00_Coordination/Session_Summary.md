# 阶段上下文摘要

*最后更新：2026-07-07*

这个文件用于抵抗上下文压缩。每次阶段推进后，Codex 应该更新这里，让后续对话和其他 agent 能快速接上。

## 当前顶层目标

Aura 要从 UE C++ / GAS 教程项目，逐步演进为一个可扩展、数据驱动、可编辑器化的 GAS 技能系统。长期目标是支持大量常见游戏技能机制，并最终提供技能编辑器，通过配置资产制作和修改技能，而不是为每个技能生成静态 C++ 类。

## 当前分工

- 用户：确认方向、审核关键计划、运行必要的 UE 验证。
- Codex：高层规划、方案讨论、给 Claude 写实施 Prompt、审查产出、维护协作文档。
- Claude：按已批准 Prompt 执行具体代码、UE MCP/编辑器操作，并写实施报告。

## 当前已经确认

- 项目使用 UE 5.8。
- 现有项目已有教程阶段 GAS 基础。
- 玩家 ASC 在 `AAuraPlayerState`，复制模式为 `Mixed`。
- 敌人 ASC 在 `AAuraEnemy`，复制模式为 `Minimal`。
- 第一批技能机制讨论先偏 ARPG。
- 第一批暂不做位移类技能。
- 第一批原型确认采用：瞬发治疗/伤害、火球投射物、范围持续地面技能、蓄力射击、三段连招、受击反弹被动。
- 召唤和陷阱重要，但不进入第一批，先放到第二批候选。
- 第一批多人验证标准：先保证本地玩家技能链路，再用 Listen Server / 两客户端验证其他端表现、应同步状态、对其他角色的权威影响。
- 准许做轻量资产盘点；具体技能需要哪些资产，正式写具体技能前再确认。资产优先从 `Content/Assets` 中找 Aura 教程资源，不够再考虑官方内容、简单占位或请用户补充。
- 用户指出不能只确认第一批，还需要先总结值得抽象的技能机制总目录。已新增 `Agent/02_SkillSystem/Skill_Mechanic_Catalog.md`，等待审阅。
- 用户进一步澄清：机制分类服务于架构和技能编辑器，但具体实现应设计“像正常游戏技能”的代表性技能组合。已新增 `Agent/02_SkillSystem/Representative_Skill_Portfolio.md`。
- 用户确认代表性技能可以采用；V1-A 具体技能为火球术、复苏脉冲、烈焰领域、蓄力贯穿箭、三段裂刃、荆棘护盾。
- 用户确认每个技能要一个一个单独实现和验证。已新增 `Agent/02_SkillSystem/V1A_Implementation_Sequence.md`。
- V1-A 完成前，后续技能池先保持当前安排，不继续展开链雷、符文标记、哨卫图腾、影步突袭。
- 用户确认当前技能机制大类足够，可以进入下一步；`Skill_Mechanic_Catalog.md` 后续只做增量补充。
- 用户认可 V1 运行时采用“克制版方案 C”：少量通用 GA + DataAsset 配置 + 少量原生机制片段。已确认顺序：先写 `Skill_Runtime_Model.md`，再做轻量资产盘点，再写第一份 Claude 实施 Prompt。
- 用户已确认 `Agent/02_SkillSystem/Skill_Runtime_Model.md`：主路径采用 `UAuraSkillDefinition + UAuraConfiguredActiveAbility + AbilitySpec.SourceObject`，并先保留旧教程 GA 路径、新增配置技能路径。
- 用户确认技能编辑器可以类比 Montage 编辑器：细节面板配置，预览框播放技能效果，Play 时可使用，保存为配置资产并下次启动仍可用。已记录到 `Agent/03_SkillEditor/Skill_Editor_Vision.md`。
- 正式代码必须有学习型注释，并配套中文说明文档；如果设计思想不适合放在代码注释中，应写入 MD 文档。
- 用户确认第一份 Claude 实施只做火球术最小闭环，不一次实现全部 V1-A 机制。
- 已创建并推送 Git tag `tutorial-baseline-before-v1-skill-runtime`，作为 V1 技能运行时代码改写前的教程基线。
- 可以直接新增 `.h` / `.cpp` 文件，不强制通过 UE 编辑器添加类向导；但必须遵守模块、`.generated.h`、Build.cs 依赖和刷新/重编译规则。
- 已完成火球术最小闭环资产与代码盘点：`Agent/02_SkillSystem/Fireball_Minimal_Asset_Inventory.md`。
- 已写好第一份 Claude 实施 Prompt：`Agent/04_AgentHandoff/Claude_Prompts/2026-07-07_Fireball_Minimal_Configured_Runtime.md`。
- 用户已批准 Claude 按第一份 Prompt 执行火球术最小配置化运行时实现。

## 当前推荐路线

1. Phase 1 已收束：技能机制大类、代表技能池、V1-A 技能顺序已经确认。
2. Phase 2 运行时模型已确认：克制版方案 C + `UAuraSkillDefinition` + 通用 GA + SourceObject。
3. 火球术资产盘点已完成，第一份 Claude Prompt 已完成。
4. 下一步等待 Claude 执行 Prompt 并输出实施报告。
5. 技能编辑器放在运行时模型稳定之后设计。

## 当前待确认

当前无待确认问题。下一步等待 Claude 的实施报告，报告路径应为 `Agent/04_AgentHandoff/Claude_Reports/2026-07-07_Fireball_Minimal_Configured_Runtime_Report.md`。

见 `Agent/00_Coordination/Pending_Questions.md`。
