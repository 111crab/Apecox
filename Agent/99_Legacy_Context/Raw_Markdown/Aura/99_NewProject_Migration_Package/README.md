# 新项目迁移包使用说明

用途：把 Aura 长对话中已经沉淀下来的有效经验，迁移到一个全新的 UE 技能系统项目中。

## 使用方式

1. 新建 UE 项目并确认能编译、能 PIE。
2. 在新项目根目录创建或覆盖 `Agent/` 文件夹。
3. 把本目录下的 `Agent/` 文件夹复制到新项目根目录。
4. 在新 Codex 对话窗口中，把 `Agent/Startup_Prompt_For_New_Codex_Window.md` 的内容发给 Codex。
5. 让 Codex 先读取 `Agent/Context_Migration_From_Aura.md`、`Agent/Decision_Log.md`、`Agent/Working_Agreement.md`。
6. 新项目创建后重新生成 `.agents/ue-project-context.md`，不要复用 Aura 的项目上下文。

## 重要原则

- 迁移理念，不迁移 Aura 代码。
- 新项目先读 Lyra / 规范项目，再写架构代码。
- 文档保持轻量，只记录长期决策、当前任务、执行 Prompt、审查结论和已完成技能链路。
- 非批量 UE 编辑器操作默认人工，不主动使用 MCP。

