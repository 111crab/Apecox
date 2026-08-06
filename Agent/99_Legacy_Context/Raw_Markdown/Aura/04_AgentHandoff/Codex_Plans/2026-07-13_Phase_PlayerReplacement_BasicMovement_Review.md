# Codex 审查：Phase 玩家验证角色替换与基础移动

审查日期：2026-07-13  
审查对象：`Agent/04_AgentHandoff/Claude_Reports/2026-07-13_Phase_PlayerReplacement_BasicMovement_Report.md`

## 总结

Claude 本轮报告称创建了 `/Game/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype`，但用户在 UE 编辑器中未能看到该蓝图。磁盘上虽存在同名 `.uasset` 文件，但它不能被视为 UE 编辑器内已成功创建、可加载、可编译的蓝图资产。

因此，本轮 MCP 结果只能视为“不可信的磁盘文件产物”，不能视为已完成原型蓝图创建。

但本阶段验收目标是“Phase 能作为玩家角色进入 PIE、移动、播放基础 locomotion”。根据报告，这个目标尚未完成：

- Phase Mesh 未绑定到原型角色。
- Phase AnimClass 未绑定到原型角色。
- `Phase_AnimBlueprint` 仍有编译错误。
- GameMode / 地图默认 Pawn 未切换到 Phase 原型。
- 单人 PIE 未验证。
- Listen Server / Client 未验证。

因此当前状态应标记为：**原型蓝图已创建，Phase 玩家基础移动尚未验收通过**。

## 审查发现

### P0：MCP 报告的蓝图创建结果未被 UE 编辑器验证

用户反馈 UE 编辑器中看不到 `BP_AuraCharacter_PhasePrototype`。这意味着 Claude 报告中的“duplicate_asset 成功”不能作为阶段完成依据。

后续处理：

1. 不再把该资产视为已创建成功。
2. 不要继续基于这个不可见/不可验证资产推进 Phase 接入。
3. 由用户在 UE 编辑器内手动复制 `BP_AuraCharacter`，重新创建 `BP_AuraCharacter_PhasePrototype`。
4. MCP 后续不得用于这种单个蓝图、单个组件属性配置、需要编辑器可视确认的操作。

### P1：阶段验收未完成

报告明确写到 MCP 无法设置 `SkeletalMeshComponent::SkeletalMesh` / `AnimClass`，且 PIE 未执行。因此不能把“Phase 玩家替换与基础移动”标记为完成。

后续必须在 UE 编辑器中补齐：

1. 打开 `BP_AuraCharacter_PhasePrototype`。
2. 设置 Mesh 为 `Phase_GDC`。
3. 设置 Anim Class。
4. 配置 GameMode 或测试地图使用该 Pawn。
5. 完成单人 PIE 验证。
6. 至少做一次 Listen Server + Client 基础移动观察。

### P1：不建议直接修复第三方原始 `Phase_AnimBlueprint`

报告建议直接打开并修复 `/Game/ThreePartAsset/.../Phase_AnimBlueprint`。这可能违反我们“第三方原始资产尽量不直接修改”的原则。

推荐改法：

- 优先在 `/Game/Aura/SkillPrototype/Phase` 下创建或复制我们自己的 `ABP_AuraPhasePrototype`。
- 让原始第三方资源保持可回退。
- 如果必须修改第三方原始 AnimBP，报告中必须明确原因，并在 Git 中单独识别这类资产修改。

### P2：GameMode 修改要避免全局污染

不要优先直接修改长期使用的 `BP_AuraGameModeBase`。更稳的方式是：

- 新建测试地图或复制测试 GameMode，例如 `BP_AuraGameMode_PhasePrototype`。
- 在测试地图 World Settings 中指定 Phase 原型 Pawn。

这样旧 Aura 角色可以随时回退，也不会影响后续对比验证。

### P2：Mesh Rotation / Location 不能照抄，需要编辑器实测

报告里写 `Location Z = -97`、`Rotation Z = 0`，但这只是候选值。实际值必须在编辑器里观察：

- 角色脚底是否贴近地面。
- Capsule 是否合理包住模型。
- 移动时角色朝向是否和输入方向一致。
- 相机是否能看清角色正面/背面。

记录最终使用的 Transform 值。

## 建议下一步

下一步不需要再让 Claude 盲目用 MCP 自动设置蓝图组件属性。这个阶段应该由用户在 UE 编辑器里手动完成，或者让 Claude 只写“人工操作清单 + 验证清单”。

推荐人工操作顺序：

1. 打开 `/Game/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype`。
2. 先只设置 Phase Mesh，不设置 AnimClass，确认模型能显示。
3. 如果 `Phase_AnimBlueprint` 仍坏，先创建一个 Aura 自己目录下的最小 Phase AnimBP；不要优先修第三方原始 AnimBP。
4. 新建或复制一个 Phase 测试 GameMode / 测试地图覆写 Default Pawn。
5. 单人 PIE 验证出生、移动、摄像机、ASC/UI。
6. 通过后再处理 locomotion 动画细节。

## 阶段状态

- 文档和原型资产：部分完成。
- Phase 玩家可运行验证：未完成。
- 是否可以进入新技能设计：暂不建议。

等 Phase 能在 PIE 中正常移动并播放基础动画后，再进入“Phase 火球 Montage + GameplayEvent Notify + 火球复验”。
