# 协作约定

生成日期：2026-07-14

## 分工

- 用户：最终决策、UE 编辑器人工操作、运行验证、项目目标把关。
- Codex：高层规划、架构讨论、代码审查、Prompt 编写、经验沉淀。
- ClaudeCode：在明确 Prompt 下执行具体代码或批量任务，并写实施报告。

## 文档规则

默认不为普通讨论创建文档。

只在以下情况写文档：

- 长期决策：写入 `Decision_Log.md`。
- 当前阶段任务：写入 `Task_Backlog.md`。
- 要交给 ClaudeCode 执行：写入 `Prompts/`。
- ClaudeCode 执行完成：写入 `Reports/`。
- Codex 审查重要产出：写入 `Reviews/`。
- 一个技能真正完成并可验证：写入 `Skill_Runbooks/`。
- 阅读 Lyra 或参考项目：写入 `Research_Notes/`。

## Prompt 规则

ClaudeCode Prompt 必须包含：

- 目标。
- 明确范围。
- 禁止事项。
- 需要读取的文档。
- 验收标准。
- 编译 / PIE / 多人验证要求。
- 失败时必须输出人工操作清单。
- 不允许把未验证结果写成完成。

## MCP 使用规则

默认不使用 MCP 处理非批量 UE 编辑器操作。

禁止主动用 MCP 做：

- 单个蓝图创建。
- 设置单个 Mesh / AnimClass。
- 修改单个 GameMode。
- 调整 Transform。
- 添加少量 Notify。
- 任何需要人眼判断的可视操作。

允许考虑 MCP 做：

- 批量资产扫描。
- 批量类型检查。
- 批量重命名或生成报告。
- 大量配置资产的一致性检查。

MCP 结果必须通过 UE 编辑器验证。

## 完成标准

阶段完成必须满足对应验证：

- C++ 改动：至少编译通过。
- 运行时改动：至少单人 PIE 验证。
- 多人相关能力：Listen Server + Client 验证。
- UE 资产改动：编辑器可见、可打开、可保存。
- 技能完成：必须有运行链路文档。

