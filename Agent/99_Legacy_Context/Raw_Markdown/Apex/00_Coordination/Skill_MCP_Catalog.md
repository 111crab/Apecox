# Skill 与 MCP 目录

生成日期：2026-07-14
来源：继承 Aura 阶段工具目录，并以 Apex 当前 `Tooling_Guide.md` 为准。

## 当前权威文件

- 工具边界与 MCP 使用规则：`Agent/00_Coordination/Tooling_Guide.md`
- UE 项目事实：`.agents/ue-project-context.md`
- Aura 原始工具目录：`Agent/Aura_Legacy_Archive/00_Coordination/Skill_MCP_Catalog.md`

## 使用原则

- 明确匹配某个 UE 领域任务时，先读对应 Skill。
- MCP 不用于单个蓝图创建、Mesh/AnimClass 设置、GameMode 修改、少量 Notify 添加等可视操作。
- MCP 更适合批量扫描、批量检查、批量报告。
- MCP 返回成功不等于 UE 资产成功；必须以编辑器可见、可打开、可编译、可 PIE 为准。
- 如果工具能力不够，停止尝试，输出人工操作清单。

## 高频 Skill

| Skill | 适用场景 |
| --- | --- |
| `ue-cpp-foundations` | UE C++、UCLASS、UPROPERTY、TObjectPtr、反射和 UObject 生命周期 |
| `ue-module-build-system` | Build.cs、UHT、include、linker、模块依赖 |
| `ue-gameplay-framework` | GameMode、PlayerController、Pawn、Character、PlayerState |
| `ue-character-movement` | CharacterMovement、角色旋转、跳跃、Root Motion、移动预测 |
| `ue-input-system` | Enhanced Input、InputAction、MappingContext、长按/松开 |
| `ue-animation-system` | AnimBP、BlendSpace、Montage、AnimNotify、GameplayEvent Notify |
| `ue-gameplay-abilities` | GAS、ASC、GA、GE、AttributeSet、GameplayTag、GameplayCue |
| `ue-data-assets-tables` | DataAsset、软引用、Asset Manager、配置资产 |
| `ue-networking-replication` | RPC、复制、Listen Server、Dedicated Server |
| `ue-editor-tools` | 未来技能编辑器、Detail Customization、Editor Utility |
