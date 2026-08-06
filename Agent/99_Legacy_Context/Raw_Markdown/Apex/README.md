# Apex Agent 协作区

生成日期：2026-07-14

这个目录用于存放 Apex 项目的协作文档。游戏源码仍放在 `Source/`，UE 资产仍放在 `Content/`，规划、Prompt、报告、审查、阶段总结和历史上下文放在这里。

## 最重要的入口

| 文件 | 用途 |
| --- | --- |
| `00_Coordination/Project_Roadmap.md` | 顶层阶段路线图，只放大阶段。 |
| `00_Coordination/Task_Backlog.md` | 跨阶段任务池，只放阶段待办和完成状态，不放当前执行细节。 |
| `00_Coordination/Current_Phase.md` | 当前正在推进的小阶段，包含执行状态、下一步、待确认、人工操作、验证标准、Git 建议。 |
| `00_Coordination/Current_UE_Manual_Steps.md` | 当前 UE 编辑器人工操作清单，每次进入新人工阶段可覆盖。 |
| `00_Coordination/Current_Code_Design.md` | 实施前代码设计预案；只放已讨论的大类、关键变量、函数、Tag、属性或配置结构。 |
| `00_Coordination/Subagent_Review_Checklist.md` | 给子代理和 Codex 共用的实施报告与审查规范。 |
| `00_Coordination/Decision_Log.md` | 长期决策。 |
| `00_Coordination/Working_Agreement.md` | 协作规则。 |
| `00_Coordination/Tooling_Guide.md` | Skill / MCP / 工具使用边界。 |

## 当前工作原则

- 当前项目默认指 `D:\UnrealProject\Apex`。
- Aura 是旧项目经验库，不再作为默认实施目标。
- 重要实现先有计划或 Prompt，再由用户确认，再执行。
- 跨阶段有哪些事看 `00_Coordination/Task_Backlog.md`；今天正在推进什么、还等你确认什么，看 `00_Coordination/Current_Phase.md`；实施前代码设计看 `00_Coordination/Current_Code_Design.md`；实施后结果看 `Reviews/`。
- Codex 负责规划、Prompt、审查和文档维护；ClaudeCode 按明确 Prompt 实施并写报告。
- 非批量 UE 编辑器操作默认人工，不主动使用 MCP。
- 学习包、Lyra、第三方示例和外部文章不是默认阅读材料；只有具体问题需要时再读取。

## 目录说明

| 路径 | 用途 |
| --- | --- |
| `00_Coordination/` | 所有活跃协作入口：路线图、当前阶段、当前代码设计、决策、任务池、协作规则、工具指南、Git 工作流、阶段摘要。 |
| `01_ProjectAudit/` | 项目事实盘点、资产盘点、工作区分类。 |
| `02_SkillSystem/` | 技能系统架构、Tag/属性/运行时模型、风险清单。 |
| `03_SkillEditor/` | 技能编辑器长期愿景和预研。 |
| `Prompts/` | 当前给 ClaudeCode 的执行 Prompt。 |
| `Reports/` | ClaudeCode 实施报告和迁移报告。 |
| `Reviews/` | Codex 审查结论。 |
| `Research_Notes/` | 参考项目、Paragon 资产等研究笔记。 |
| `90_References/` | 外部参考索引和评审。 |
| `Aura_Legacy_Archive/` | Aura `Agent/` 原文完整归档，仅作为历史上下文。 |

## 新对话建议读取顺序

1. `.agents/ue-project-context.md`
2. `Agent/README.md`
3. `Agent/00_Coordination/Working_Agreement.md`
4. `Agent/00_Coordination/Decision_Log.md`
5. `Agent/00_Coordination/Project_Roadmap.md`
6. `Agent/00_Coordination/Current_Phase.md`
7. 需要代码设计时读 `Agent/00_Coordination/Current_Code_Design.md`
8. 子代理实施后读当前任务相关 `Reviews/`

## Aura 历史上下文使用规则

`Aura_Legacy_Archive/` 保存旧项目全部 Agent 文件，包含旧方案、旧 Prompt、旧报告、旧审查和外部文章。读取时要注意：

- 其中大量类名、路径、资产名仍是 Aura，不可直接套用到 Apex。
- 其中的设计经验可以继承，但代码和资产实现不直接迁移。
- 若要从旧文档继承规则，必须先写入 Apex 当前 active 文档，不能只靠“记得以前说过”。
