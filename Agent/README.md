# Apecox Agent 导航

本目录保存 Apecox 的协作规则、阶段计划、技术设计、审查记录和历史知识。目标是让日常入口足够轻，同时保证 Aura/Apex 阶段的技术思想可追溯。

## 每次通常只读什么

- `00_Coordination/Current_Phase.md`：当前阶段、下一步、待确认事项的唯一入口。
- `00_Coordination/Current_Code_Design.md`：仅在准备写代码或审阅公开命名时读取。
- `00_Coordination/Current_UE_Manual_Steps.md`：仅在准备进入 UE 编辑器操作时读取，每次覆盖。
- 当前任务对应的 Prompt、Report 或 Review：只在该任务实施和审查时读取。

## 其他目录

- `00_Coordination`：活跃协作规则、顶层路线、当前阶段、决策与 Git 约定。
- `01_Project`：项目事实、产品方向和阶段性项目审计。
- `02_CombatFramework`：Apecox 战斗、武器、GAS 与技能框架的 RFC 和继承基线。
- `03_EditorVision`：运行时稳定以后再考虑的技能编辑器边界。
- `90_References`：精选旧项目研究结论和外部资料索引，不是当前决策本身。
- `99_Legacy_Context`：Aura/Apex Markdown 原文的去重只读归档，默认不读取。
- `Prompts`、`Reports`、`Reviews`、`Research_Notes`：按任务产生的临时或阶段性材料。

## 权威性顺序

- 当前用户确认和 `Decision_Log.md`。
- 当前阶段 RFC 与 `Current_Code_Design.md`。
- `Framework_Inheritance_Baseline.md` 中标记为“继承原则”的内容。
- `90_References` 与 `99_Legacy_Context` 仅供论证和追溯。

历史文件即使使用 Aura/Apex 旧命名，也不代表 Apecox 已采用其中的类名或实现。
