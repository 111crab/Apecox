# 工具与 Skill 使用指南

生成日期：2026-07-14

## 用途

本文记录新项目中 Codex / ClaudeCode / MCP / UE Skills 的使用边界。它不是完整工具手册，而是项目协作时的实践规则。

核心原则：

- 工具服务于项目判断，不替代判断。
- UE 编辑器内单个、可视、需要人眼确认的操作，默认人工完成。
- 工具返回成功不等于 UE 资产真的成功，必须以编辑器可见、可打开、可编译、可 PIE 为准。

## Codex 可用 UE Skills

以下 Skills 是 Codex 环境中的通用能力，不依赖某一个 UE 项目。新项目仍然可用。

| Skill | 适用场景 |
| --- | --- |
| `ue-project-context` | 创建或更新 `.agents/ue-project-context.md`，让 Codex 理解当前 UE 项目结构 |
| `ue-cpp-foundations` | UE C++、UCLASS、UPROPERTY、UFUNCTION、TObjectPtr、UObject 生命周期 |
| `ue-module-build-system` | Build.cs、Target.cs、模块依赖、编译错误、include / linker 问题 |
| `ue-gameplay-framework` | GameMode、GameState、PlayerController、Pawn、Character、PlayerState |
| `ue-gameplay-abilities` | GAS、ASC、GameplayAbility、GameplayEffect、AttributeSet、GameplayTag |
| `ue-input-system` | Enhanced Input、InputAction、MappingContext、输入到技能的绑定 |
| `ue-animation-system` | AnimBP、Montage、AnimNotify、GameplayEvent Notify、Locomotion |
| `ue-character-movement` | CharacterMovementComponent、移动、跳跃、旋转、Root Motion、网络预测 |
| `ue-networking-replication` | 多人同步、RPC、Replication、Listen Server、Dedicated Server |
| `ue-data-assets-tables` | DataAsset、DataTable、软引用、Asset Manager、配置资产 |
| `ue-niagara-effects` | Niagara、VFX、参数、特效生成和生命周期 |
| `ue-ui-umg-slate` | UMG、HUD、技能 UI、血条、冷却显示 |
| `ue-editor-tools` | 自定义编辑器、Editor Utility、资产编辑器、未来技能编辑器 |
| `ue-testing-debugging` | UE_LOG、日志过滤、Functional Test、调试和验证流程 |

## Skill 使用规则

### GAS 相关

涉及以下内容时，Codex 应优先使用 `ue-gameplay-abilities`：

- ASC 初始化。
- Ability 授予和激活。
- GameplayEffect / AttributeSet。
- GameplayTag 设计。
- AbilityTask。
- GameplayCue。
- 技能多人预测与同步。

### 编译和模块相关

涉及以下内容时，使用 `ue-module-build-system`：

- `Build.cs` 依赖。
- `Target.cs`。
- include 找不到。
- linker error。
- UHT / `.generated.h` 问题。
- 插件模块依赖。

### 动画相关

涉及以下内容时，使用 `ue-animation-system`：

- AnimBlueprint。
- BlendSpace。
- Montage。
- AnimNotify。
- GameplayEvent Notify。
- 动画和 GAS 的关键帧触发关系。

### 移动和视角相关

涉及以下内容时，使用 `ue-character-movement`：

- CharacterMovementComponent。
- 跳跃、下落、落地。
- 角色朝向移动方向还是控制器方向。
- 近第三人称准星视角。
- Root Motion。
- 移动预测。

### 输入相关

涉及以下内容时，使用 `ue-input-system`：

- Enhanced Input。
- InputAction。
- MappingContext。
- 输入按下、松开、长按。
- 输入 Tag 到 Ability 的路由。

### 配置资产相关

涉及以下内容时，使用 `ue-data-assets-tables`：

- SkillDefinition。
- AbilitySet。
- CombatEntityDefinition。
- Buff / Effect 配置。
- 软引用和异步加载。

### 多人相关

涉及以下内容时，使用 `ue-networking-replication`：

- Listen Server / Client 验证。
- Dedicated Server 设计。
- ReplicationMode。
- RPC。
- 服务器权威效果。
- 客户端表现同步。

## MCP 使用现实限制

Aura 阶段已经验证：MCP 对某些 UE 编辑器操作并不可靠。新项目必须更谨慎。

### 默认禁止主动使用 MCP 的场景

以下操作默认人工完成：

- 创建单个蓝图。
- 复制单个蓝图。
- 设置单个 SkeletalMesh。
- 设置单个 AnimClass。
- 调整 Mesh Transform。
- 修改单个 GameMode。
- 添加少量 AnimNotify / GameplayEvent Notify。
- 打开资源肉眼检查动画、特效、模型朝向。
- 任何需要视觉判断、编译确认、PIE 验证的操作。

原因：

- MCP 可能只创建磁盘文件，但 UE 编辑器不显示。
- MCP 可能报告成功，但资产不可打开或不可编译。
- MCP 不擅长判断 Mesh 朝向、脚底高度、动画是否自然。

### 适合考虑 MCP 的场景

以下操作可以考虑 MCP，但仍需验证：

- 批量扫描资产类型。
- 批量列出路径。
- 批量检查 DataAsset 字段。
- 批量生成报告。
- 批量创建大量简单配置资产。
- 批量重命名或整理前的预检查。

### MCP 使用硬规则

1. 先判断是否真的需要 MCP。
2. 单个低复杂度编辑器操作优先人工。
3. MCP 第一次失败或能力不足时，立即停止，不反复尝试。
4. ClaudeCode 必须输出人工替代步骤。
5. MCP 结果必须经过 UE 编辑器验证。
6. 未经 UE 编辑器可见、可打开、可编译确认，不得写“完成”。

## ClaudeCode Prompt 必须包含的工具条款

以后交给 ClaudeCode 的 Prompt 应包含类似条款：

```text
工具使用限制：
- 非批量 UE 编辑器操作默认不使用 MCP。
- 如果需要创建或修改单个蓝图、设置 Mesh、设置 AnimClass、调整 Transform、修改 GameMode，请优先输出人工操作清单。
- MCP 只可用于批量扫描、批量检查、批量报告，或用户明确授权的批量编辑。
- MCP 返回成功不等于任务完成，必须以 UE 编辑器可见、可打开、可编译、可 PIE 为准。
- 如果无法验证，请明确写“未验证”，不得写成完成。
```

## 新项目最小验证标准

### 文档 / 规划完成

- 用户已阅读并认可。
- 决策已写入 `Decision_Log.md`。
- 下一步已写入 `Task_Backlog.md`。

### C++ 改动完成

- 编译通过。
- 如果新增、移动、删除或重命名 `.h/.cpp`，在用户 review 通过并准备进入 UE/Rider 手动配置前，已运行 `Scripts/RegenerateProjectFiles.ps1`。
- 修改点有必要注释。
- 有中文实现报告或审查说明。

### Rider / 项目文件刷新

当新增、移动、删除或重命名 C++ 文件后，Rider 可能不会立刻显示这些文件。这个动作不需要在每一次讨论或小改后立刻执行；推荐在用户 review 通过、即将进入 UE 编辑器或 Rider 做手动配置前统一执行。

推荐命令：

```powershell
powershell -ExecutionPolicy Bypass -File "D:\UnrealProject\Apex\Scripts\RegenerateProjectFiles.ps1"
```

底层调用：

```powershell
E:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe -ProjectFiles -project="D:\UnrealProject\Apex\Apex.uproject" -game -engine -progress
```

Codex 后续修改 C++ 文件时，不应在中途频繁执行该脚本；应在“代码审查通过，准备交给用户进 UE/Rider 配置”这个节点执行一次。

注意：UE 5.8 的 UBT 会临时生成 `Automation_Apex.sln` / `Automation_Apex.slnx`。这些是 Automation/C# 辅助方案，不是项目主方案。脚本会默认删除它们，Rider 应打开 `D:\UnrealProject\Apex\Apex.sln`。

### UE 资产改动完成

- 编辑器可见。
- 可打开。
- 可编译或可保存。
- 必要时已 PIE 验证。

### 技能完成

- 单人 PIE 验证。
- 如果涉及多人，Listen Server + Client 验证。
- 有运行链路文档。
- 记录输入、Ability、目标选择、动画、CombatEntity、Effect、UI / 表现链路。

## 推荐新项目第一步

新 Codex 窗口进入 Apex 项目后：

1. 读取 `Agent/00_Coordination/Context_Migration_From_Aura.md`。
2. 读取 `Agent/00_Coordination/Decision_Log.md`。
3. 读取 `Agent/00_Coordination/Task_Backlog.md`。
4. 读取 `Agent/00_Coordination/Working_Agreement.md`。
5. 读取本文。
6. 生成新的 `.agents/ue-project-context.md`。
7. 汇报当前项目事实基线。

不要在事实基线确认前写技能代码。
