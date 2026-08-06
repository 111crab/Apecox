# Apex 阶段上下文摘要

生成日期：2026-07-14
用途：抵抗上下文压缩，让新对话或其他 agent 快速接上。

## 当前顶层目标

Apex 是从干净 UE 5.8 C++ 项目开始的新技能系统实践项目。长期目标是构建基于 GAS 的、配置驱动的、可扩展的技能系统，并在运行时稳定后设计技能编辑器。

Aura 是旧教程/试验项目，现在只作为经验库和历史上下文，不再作为默认实施目标。

## 当前设计方向

- 资产与英雄验证基于 Paragon 资产包。
- 项目采用第三人称鼠标视角。
- 技能设计先尊重可用角色动作、Montage、Socket、VFX 和地图空间。
- 第一阶段不急于写技能系统代码，先建立角色、移动、视角、资产验证基线。
- 技能系统长期方向仍是 `SkillDefinition + SkillRuntimeAbility + SkillTemplate + CombatEntity + Effect/State + GameplayCue`。
- 技能编辑器长期方向类比 Montage 编辑器：配置、预览、保存配置资产，Play 时可使用，不生成每技能 C++ 类。

## 当前分工

- 用户：最终决策、UE 编辑器人工操作、运行验证、项目目标把关。
- Codex：高层规划、架构讨论、Prompt 编写、代码审查、上下文维护。
- ClaudeCode：在明确 Prompt 下执行具体代码或批量任务，并写中文报告。

## 当前已确认

- Apex 不直接迁移 Aura 代码。
- Aura Agent 历史上下文已迁移到 `Agent/Aura_Legacy_Archive/`。
- 学习包、Lyra、第三方示例和外部文章不是默认阅读材料，需要时再读。
- 非批量 UE 编辑器操作默认人工，不主动使用 MCP。
- 当前 Character 冷启动只新增 `AApexCharacterBase` 和 `AApexPlayerCharacter`。
- 新长期 C++ 代码采用 `Source/Apex/Public/...` 和 `Source/Apex/Private/...` 目录。
- 第一版 Character 不新增 AnimInstance，不播放 Montage，不接 GAS，不加 GameplayTag。
- `AApexPlayerCharacter` 的相机臂成员命名为 `CameraSpringArm`，不使用模板习惯里的 `CameraBoom`。
- 新增 C++ 文件、核心类名、关键成员变量、GameplayTag 命名需要用户审阅。
- 当前阶段待确认事项合并到 `Agent/00_Coordination/Current_Phase.md`；`Pending_Questions.md` 已删除。

## 当前任务状态

`Agent/Prompts/2026-07-14_Character_ColdStart_ClaudeCode_Prompt.md` 已生成并执行完成，ClaudeCode 报告为 `Agent/Reports/2026-07-14_Character_ColdStart_Report.md`。

Codex 已将新 C++ 文件整理为 `Source/Apex/Public/Character/*.h` 和 `Source/Apex/Private/Character/*.cpp`。之后用户要求将 `CameraBoom` 改为 `CameraSpringArm`，已再次确认 `ApexEditor Win64 Development` 编译通过。当前工作区中仍有 `Content/ParagonPhase/` 等未跟踪资产改动，可能来自用户或 ClaudeCode。Codex 不应覆盖这些改动，后续审查时需要先读取再判断。

## 当前推荐下一步

1. 用户决定默认 Pawn 是临时使用现有模板 GameMode，还是现在新建 Apex 专用 GameMode。
2. 用户在 UE 编辑器中人工创建/配置 `BP_Hero_Phase`。
3. PIE 验证移动、视角和角色显示。
4. 再进入 Paragon Phase 动画/AnimBP 验证。
