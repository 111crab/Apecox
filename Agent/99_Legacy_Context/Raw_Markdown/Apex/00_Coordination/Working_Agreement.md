# 协作约定

生成日期：2026-07-14

## 分工

- 用户：最终决策、UE 编辑器人工操作、运行验证、项目目标把关。
- Codex：高层规划、架构讨论、代码审查、Prompt 编写、经验沉淀。
- ClaudeCode：在明确 Prompt 下执行具体代码或批量任务，并写实施报告。

## 文档规则

默认不为普通讨论创建文档。

只在以下情况写文档：

- 长期决策：写入 `00_Coordination/Decision_Log.md`。
- 跨阶段待办：写入 `00_Coordination/Task_Backlog.md`。
- 当前局部阶段执行状态：写入 `00_Coordination/Current_Phase.md`。
- 当前 UE 编辑器人工操作步骤：写入并覆盖 `00_Coordination/Current_UE_Manual_Steps.md`。
- 要交给 ClaudeCode 执行：写入 `Prompts/`。
- ClaudeCode 执行完成：写入 `Reports/`。
- Codex 审查重要产出：写入 `Reviews/`。
- 一个技能真正完成并可验证：在 `02_SkillSystem/` 下按需创建运行链路文档。
- 阅读 Lyra 或参考项目：写入 `Research_Notes/`。

## Prompt 规则

标准任务和架构任务需要 ClaudeCode Prompt；快速任务默认由 Codex 直接实施，不单独创建 Prompt。需要 Prompt 时必须包含：

- 目标。
- 明确范围。
- 允许修改的文件路径；C++ 新长期代码应明确 `Public` 头文件路径和 `Private` 实现路径。
- 禁止事项。
- 需要读取的文档。
- 验收标准。
- 编译 / PIE / 多人验证要求。
- 失败时必须输出人工操作清单。
- 不允许把未验证结果写成完成。

## 任务分级与轻量化流程

### 快速任务

适用于文件移动、include / 导出宏修正、少量明确 Bug 修复等机械改动，通常不超过 4 个文本文件，且不涉及新架构、反射字段重命名、UE 资产、网络或 GAS 生命周期。

流程：聊天中给出简短范围并经用户确认后，由 Codex 直接修改、检查和编译，再在聊天中汇报。默认不更新 `Current_Code_Design.md`，不创建 Prompt、Report 或 Review；只在完成后按需简要更新 `Current_Phase.md`。

### 标准任务

适用于新增一个或少量 C++ 类、修改运行时行为、输入、动画或单个功能闭环。

流程：维护一份精简 `Current_Code_Design.md`，由 ClaudeCode 按短 Prompt 实施并写一份中文报告，Codex 在聊天中审查。只有出现重要风险、偏差或需要长期追踪的问题时才新增 Review 文档。

### 架构任务

适用于 GAS 基础、ASC 生命周期、GameplayTag 规范、SkillDefinition、网络预测和跨系统边界。

流程：先讨论并形成 RFC / 长期决策，再写代码设计、实施 Prompt、报告和必要的正式 Review。

### 去重原则

- Prompt 引用 `Working_Agreement.md` 和 `Subagent_Review_Checklist.md`，不重复粘贴全部通用规则。
- `Current_Phase.md` 只保留当前目标、状态、下一步、待确认和验证入口，历史细节进入 Report / Review / Decision Log。
- `Current_Code_Design.md` 只服务当前标准或架构实施批次；快速任务不占用它。
- 同一信息只维护一个权威入口，避免在 Phase、Prompt、Report 和 Review 中重复展开。

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
- 新增、移动、删除或重命名 `.h/.cpp` 后：先完成代码审查；在用户 review 通过、即将进入 UE/Rider 手动配置前，由用户默认运行 `Scripts/RegenerateProjectFiles.ps1`。Codex 只负责提醒，除非用户明确要求代为执行。
- 运行时改动：至少单人 PIE 验证。
- 多人相关能力：Listen Server + Client 验证。
- UE 资产改动：编辑器可见、可打开、可保存。
- 技能完成：必须有运行链路文档。


## 当前窗口工作模式

从 2026-07-14 起，当前 Codex 对话窗口的工作重心切换到 `D:\UnrealProject\Apex`。

- Aura 只作为旧项目经验库，不再作为默认实施目标。
- 如果用户没有特别说明，“项目”“当前项目”“新项目”默认指 Apex。
- 由于当前 Codex workspace 可能仍显示 Aura，涉及 Apex 文件读写时应使用绝对路径。
- 写入 Apex 可能需要授权；授权后仍必须遵守“不误改、不删除、不覆盖用户资产”的原则。
- `.agents/ue-project-context.md` 以 Apex 项目自身为准，不复用 Aura 的上下文。

## 参考项目 / 学习包使用规则

Lyra、学习包、示例项目和第三方实现不是默认阅读材料。

- 只有在用户明确要求、Prompt 明确要求，或当前问题确实需要参考成熟实现时，Codex / ClaudeCode 才读取这些内容。
- 读取前应说明目的：是为了解决架构问题、编译问题、动画问题、GAS 问题，还是资产迁移问题。
- 读取后必须说明借鉴了什么、没有借鉴什么，不能盲目照搬。
- 普通冷启动任务、简单 C++ 骨架、单个 UE 编辑器人工配置，不主动打开学习包。

## 从 Aura 继承的硬规则

以下规则来自 Aura 阶段，已迁移为 Apex 当前协作基线：

- 重大实现开始前必须先有计划或 ClaudeCode Prompt，并由用户确认方向。
- Codex 写 Prompt 时必须列明目标、范围、允许修改文件、禁止事项、验证方式、报告路径。
- ClaudeCode 实施后必须写中文报告，说明修改文件、关键设计、验证结果、未验证内容和后续人工步骤。
- Codex 审查时优先看是否符合 Prompt、是否符合 UE/GAS 生命周期、是否有资产副作用、验证是否足够。
- 正式 C++ 代码要服务学习：非显然生命周期、权威/预测/复制边界、解耦原因、阶段性折中需要写有价值的注释或中文文档。
- 可以直接新增 `.h` / `.cpp` 文件，不强制通过 UE 编辑器添加类向导；但必须保证反射宏、`.generated.h`、模块依赖和编译验证正确。
- 直接新增、移动、删除或重命名 `.h` / `.cpp` 后，不在中途频繁刷新项目文件；等用户 review 通过、准备进入 UE/Rider 手动配置时，再由用户统一重新生成项目文件。Codex 不主动代为运行。
- 需要用户审核、讨论、决策的文档尽量使用中文。
- 新增核心类名、关键成员变量、GameplayTag、重要资产命名时，先列命名表给用户审阅。

## 当前阶段与代码设计分离

后续不使用一个混杂的 `Current_Work.md` 承载所有内容，而是拆成两份：

- `Agent/00_Coordination/Task_Backlog.md`：跨阶段待办池，只记录任务和完成状态。
- `Agent/00_Coordination/Current_Phase.md`：当前局部阶段、执行状态、下一步、待用户确认、人工 UE 操作、验证标准、Git 建议、风险。
- `Agent/00_Coordination/Current_Code_Design.md`：实施前代码设计预案，记录已经讨论的大类、关键变量、函数、Tag、属性或配置结构；子代理实施后新增或偏离预案的内容写入 `Reviews/`。

Codex 每次推动新局部阶段时，应先判断是否需要维护代码设计页；纯讨论、纯验证或纯资产人工配置不强制更新 `Current_Code_Design.md`。

## Git 轻量化规则

- 默认以“一个功能闭环”或“一个可验证的小阶段”为 commit 单位，不要求每次讨论、每份文档或每天机械提交。
- 提交前只做一次快速状态检查，按本阶段涉及的目录或文件组暂存，不对所有普通文件逐个做耗时审计。
- 只有第三方大资产、生成目录、明显无关改动、来源不明文件或高风险二进制资产需要单独分类。
- 未完成的小改动可以暂存于工作区，等功能验证通过后与本阶段成果一起提交。
- Push 默认安排在当日结束、阶段完成或需要远端备份/协作时进行。
- Git 服务于可恢复和可协作，不应挤占架构讨论、实现与验证的主要时间。
- Git 可以委派给子代理做机械执行，但提交边界仍由 Codex 根据当前讨论决定。Codex 必须给出明确的暂存路径、排除项、commit message 和是否 push。
- 子代理执行顺序固定为：`status -> add 指定路径 -> diff --cached/stat -> commit -> push -> 报告 commit hash 与剩余工作区`；不得自行扩大范围、清理文件、回退改动或创建 Tag/Release。
- 简单提交由 Codex 直接完成更快时，不强制为了 Git 单独开启子代理；改动较多、批量检查或用户已开启实施会话时再委派。

## 子代理实施后的反馈规则

ClaudeCode 或其他子代理完成代码任务后，Codex 反馈给用户时必须包含：

- 是否符合 Prompt 范围。
- 新增 / 修改 C++ 类名、父类、文件路径和作用。
- 成员变量名、类型和作用。
- 函数名、参数和作用。
- 是否新增 GameplayTag、Attribute、GA、GE、DataAsset、Cue。
- 是否需要用户审阅命名或职责。
- 人工 UE 编辑器操作步骤。
- 验证成功标准。
- Git 分组建议。

这些内容优先参考 `Agent/00_Coordination/Subagent_Review_Checklist.md`。

## 待确认问题维护规则

`Agent/00_Coordination/Current_Phase.md` 是当前阶段待确认问题的唯一活跃入口。

- 不再维护独立 `Pending_Questions.md`，避免与 `Current_Phase.md` 重复。
- 每次进入实施前，Codex 应显式告诉用户当前相关待确认问题，并同步写入 `Current_Phase.md`。
- 用户确认后，Codex 应及时更新 `Current_Phase.md` 的状态。
- 不能把重要待确认点只写在聊天里或只写在 Prompt / Report 里。
