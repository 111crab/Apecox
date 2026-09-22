# 工具与 Skill 使用指南

更新日期：2026-09-19

## 原则

- 工具服务于判断，不替代 UE 编辑器、人眼验证和多人 PIE。
- 单个蓝图、动画、Notify、Mesh、GameMode 或 Details 配置默认由用户手工完成。
- MCP 适合批量扫描、批量检查和批量报告；返回“成功”不等于资产可见、可打开、可编译。
- 第一次发现 MCP 能力不足就停止，改为详细人工步骤。
- 用户当前自行处理 Rider 项目文件刷新，Codex 不在每轮小改后生成解决方案。
- 对 Blend Space、AnimBP、Niagara、材质等包含编辑器派生数据或编译数据的二进制资产，不能把 `set_editor_property + save` 当作完成。自动化前先确认对应原生编辑器的构建/编译流程；无法从脚本可靠触发时，直接让用户手工迁移或配置。
- 自动创建或修改上述资产后，必须在不打开该资产编辑器的全新 UE 进程中完成冷加载、运行时求值检查，并保留一次最小 PIE 视觉验证。打开资产后才恢复正常，视为资产生成失败。
- 少量复杂动画资产优先让用户通过 Content Browser 的 Migrate、Duplicate 或原生资产编辑器完成；自动化主要用于盘点、依赖检查、重复操作和已有可靠验证路径的资产。

## 高频 UE Skills

| Skill | 使用场景 |
| --- | --- |
| `ue-project-context` | 维护 `.agents/ue-project-context.md` |
| `ue-cpp-foundations` | UCLASS、UPROPERTY、UFUNCTION、UObject 生命周期 |
| `ue-module-build-system` | Build.cs、Target.cs、UHT、include 和链接错误 |
| `ue-gameplay-framework` | GameMode、GameState、Controller、PlayerState、Pawn |
| `ue-gameplay-abilities` | ASC、GA、GE、AttributeSet、Tag、Task、Cue |
| `ue-input-system` | Enhanced Input、InputTag、按下/保持/释放 |
| `ue-animation-system` | AnimBP、BlendSpace、Montage、Notify、IK |
| `ue-character-movement` | CMC、跳跃、朝向、Root Motion、移动预测 |
| `ue-networking-replication` | Authority、RPC、复制、预测、多人验证 |
| `ue-data-assets-tables` | DataAsset、InstancedStruct、软引用和 Asset Manager |
| `ue-physics-collision` | Trace、碰撞、命中判定 |
| `ue-niagara-effects` | 武器、命中和技能表现 |
| `ue-ui-umg-slate` | HUD、准星、生命护盾、弹药和技能 UI |
| `ue-testing-debugging` | UE_LOG、Functional Test、Insights 和验证工具 |
| `ue-editor-tools` | 长期技能编辑器和资产校验工具 |

## MCP 默认不使用的操作

- 创建或修改单个蓝图。
- 设置 SkeletalMesh、AnimClass、Socket 或 Transform。
- 创建单个 Montage Notify / GameplayEvent Notify。
- 修改单个 GameMode、地图或 Widget。
- 需要肉眼判断动画、特效、手持位置、瞄准或 UI 的操作。

## 可以考虑 MCP 的操作

- 盘点大量动画、枪械、Niagara、DataAsset 路径。
- 批量检查配置字段和丢失引用。
- 批量生成无歧义的简单资产，并由用户抽检。
- 生成资产清单和迁移报告。

## 子代理 Prompt 固定工具条款

```text
工具限制：
- 非批量 UE 编辑器操作默认不使用 MCP。
- 如果单个资产操作无法可靠自动化，停止尝试并输出中文人工操作清单。
- MCP 返回成功不等于任务完成；必须标注是否经过编辑器可见、可打开、可编译和 PIE 验证。
- 不得自行扩大任务范围、修改无关资产或清理工作区。
```
