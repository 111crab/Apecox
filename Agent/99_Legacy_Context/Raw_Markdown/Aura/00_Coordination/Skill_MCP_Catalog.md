# Skill 与 MCP 目录

*最后更新：2026-07-08*

这个文件记录 Aura 项目最常用、最相关的 skills 和 MCP 工具。它不是完整清单，后续遇到新的有用工具时再补充。

## 核心 UE Skills

| Skill | 什么时候用 | 备注 |
| --- | --- | --- |
| `ue-project-context` | 创建或更新 `.agents/ue-project-context.md` | 让其他 UE skills 了解本项目事实。 |
| `ue-gameplay-abilities` | GAS、GA、GE、AttributeSet、GameplayTags、GameplayCues | Aura 技能系统运行时的核心 skill。 |
| `ue-editor-tools` | Editor 模块、资产编辑器、细节面板、自定义工具 | 未来技能编辑器的核心 skill。 |
| `ue-module-build-system` | Build.cs、Target.cs、模块边界、链接/包含错误 | 添加 Editor 模块前尤其重要。 |
| `ue-cpp-foundations` | UObject、UPROPERTY、委托、智能指针、UE C++ 风格 | 写 C++ 细节时使用。 |
| `ue-networking-replication` | ASC 复制、RPC、预测、多人同步 | 设计复杂技能前必须考虑。 |
| `ue-input-system` | Enhanced Input、InputAction、输入 Tag | 当前项目已经用 `UAuraInputConfig` 和输入 Tag。 |
| `ue-animation-system` | Montage、AnimNotify、动画任务 | 施法、攻击、受击等表现需要。 |
| `ue-niagara-effects` | Niagara VFX、参数、GameplayCue 表现 | 技能特效需要。 |
| `ue-audio-system` | 音效、衰减、MetaSound、SoundCue | 技能声音表现需要。 |
| `ue-ui-umg-slate` | UMG HUD、Slate 编辑器 UI | 运行时 UI 和编辑器 UI 都会用到。 |
| `ue-physics-collision` | Trace、Overlap、HitResult、碰撞通道 | 目标选择、投射物、范围技能需要。 |
| `ue-ai-navigation` | AI、感知、导航、EQS | 后续敌人技能或战术目标选择需要。 |
| `ue-data-assets-tables` | DataAsset、DataTable、软引用、Asset Manager | 数据驱动技能系统需要。 |
| `ue-testing-debugging` | UE 测试、日志、DebugDraw、Profiling | 验证和调试阶段使用。 |

## 常用 MCP 工具组

当前可用的 `mcp__unreal_engine_ChiR24` 工具集可以操控 UE 编辑器。只有在任务需要且编辑器状态合适时才使用。

| 工具组 | 用途 |
| --- | --- |
| `inspect` | 检查 Actor、资产、类、项目设置、场景统计。 |
| `manage_asset` | 列出、搜索、创建、修改资产和材质。 |
| `manage_blueprint` | 创建和修改 Blueprint / Widget Blueprint。 |
| `manage_gas` | 创建或查询 GAS Ability、Effect、AttributeSet。 |
| `control_editor` | PIE、控制台命令、截图、视口控制。 |
| `control_actor` | 生成、移动、检查、标记、修改关卡 Actor。 |
| `animation_physics` | Animation Blueprint、Montage、IK、物理资产。 |
| `manage_effect` | Niagara 系统和调试特效。 |
| `manage_audio` | SoundCue、AudioComponent、SoundClass。 |
| `manage_pcg` | PCG 图创建与执行。 |
| `manage_level` / `manage_level_structure` | 关卡、子关卡、流送、体积。 |
| `manage_networking` | 输入、GameMode/Controller、会话、复制辅助。 |
| `system_control` | Profiling、截图、控制台命令、UE Python。 |

## 使用原则

- 明确匹配某个领域任务时，先读对应 skill。
- UE 建议优先参考 `.agents/ue-project-context.md`。
- MCP 只在需要检查或修改 UE 编辑器/资产时使用。
- 目前不默认使用 MCP 填写自定义 `UAuraSkillDefinition`、`GameplayEffect` 等资产的细字段。实践中 MCP 可以复制/创建资产，但对项目自定义 UPROPERTY 和复杂 GE 配置不够可靠，容易留下“资产存在但字段未正确配置”的半成品。
- 自定义技能配置资产优先由人工在 UE 编辑器中配置，或由后续专门的编辑器工具/可验证的批量脚本处理。MCP 暂时更适合资产检查、复制、重命名、批量整理、PIE/日志/截图等可验证操作。
- 如果 MCP 无法安全表达某个操作，就让用户介入，或改用代码/配置路线。
- 重要的 MCP/编辑器操作要写入计划或报告。

## 当前项目 MCP 备注

- `McpAutomationBridge` 位于 `Plugins/`。
- `Config/DefaultGame.ini` 已启用 Native MCP。
- 该插件是 Editor-only，不是游戏运行时代码。
- 插件标记为 beta/experimental，重要资产操作后应验证。
