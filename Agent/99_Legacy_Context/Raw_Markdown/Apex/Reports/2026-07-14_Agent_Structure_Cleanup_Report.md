# Agent 结构清理报告

生成日期：2026-07-14

## 目标

减少 Apex `Agent/` 目录中的重复入口，让用户日常只需要看少数文件，避免 `Current_Phase.md` / `Pending_Questions.md`、`04_AgentHandoff/` / `Prompts-Reports-Reviews` 这类双入口造成混乱。

## 最终原则

- `Agent/README.md`：唯一根入口。
- `Agent/00_Coordination/`：所有活跃协作规则和阶段推进入口。
- `Agent/Prompts/`：给 ClaudeCode 的执行 Prompt。
- `Agent/Reports/`：ClaudeCode 或迁移/整理报告。
- `Agent/Reviews/`：Codex 对实施结果的审查。
- `Agent/Aura_Legacy_Archive/`：旧 Aura 原文归档，不参与日常阅读。

## 已迁移文件

| 原路径 | 新路径 |
| --- | --- |
| `Agent/Decision_Log.md` | `Agent/00_Coordination/Decision_Log.md` |
| `Agent/Task_Backlog.md` | `Agent/00_Coordination/Task_Backlog.md` |
| `Agent/Working_Agreement.md` | `Agent/00_Coordination/Working_Agreement.md` |
| `Agent/Tooling_Guide.md` | `Agent/00_Coordination/Tooling_Guide.md` |
| `Agent/Context_Migration_From_Aura.md` | `Agent/00_Coordination/Context_Migration_From_Aura.md` |
| `Agent/Startup_Prompt_For_New_Codex_Window.md` | `Agent/00_Coordination/Startup_Prompt_For_New_Codex_Window.md` |

## 已移除冗余

| 路径 | 原因 |
| --- | --- |
| `Agent/00_Coordination/Pending_Questions.md` | 待确认事项已合并到 `Current_Phase.md`，保留两个入口会冲突。 |
| `Agent/04_AgentHandoff/` | 与 `Prompts/Reports/Reviews` 重复，且只有 README 占位。 |
| `Agent/90_References/External_Articles/` | 空目录；外部文章已在 `Aura_Legacy_Archive/` 中完整归档。 |
| `Agent/Skill_Runbooks/` | 空目录；未来真正完成技能后再按需在 `02_SkillSystem/` 下创建运行链路文档。 |

## 用户日常阅读顺序

1. `Agent/00_Coordination/Current_Phase.md`
2. 需要看实施前代码设计时读 `Agent/00_Coordination/Current_Code_Design.md`
3. 子代理完成后读对应 `Agent/Reviews/*.md`
4. 迷路或阶段切换时再读 `Agent/00_Coordination/Project_Roadmap.md`

## 保留但不日常阅读

- `Agent/00_Coordination/Decision_Log.md`：长期决策。
- `Agent/00_Coordination/Task_Backlog.md`：跨阶段待办池。
- `Agent/00_Coordination/Working_Agreement.md`：协作规则。
- `Agent/00_Coordination/Tooling_Guide.md`：工具和 MCP 边界。
- `Agent/Reports/*.md`：历史报告。
- `Agent/Prompts/*.md`：历史 Prompt。
