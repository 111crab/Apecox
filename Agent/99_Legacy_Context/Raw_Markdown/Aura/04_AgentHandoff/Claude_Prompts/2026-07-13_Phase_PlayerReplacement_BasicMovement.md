# ClaudeCode 执行任务：Phase 玩家验证角色替换与基础移动

创建日期：2026-07-13  
负责人分工：Codex 规划/审查，ClaudeCode 实施/报告

## 任务目标

把 `ParagonPhase` 接入为 Aura 项目的玩家侧技能验证角色，完成基础移动和基础动画绑定。

本阶段只要求 Phase 能作为玩家角色进入 PIE、移动、播放基础 locomotion，并且不破坏 Aura 的 GAS 初始化链路。不要开始设计新技能，不要重构技能运行时，不要做完整准星视角。

## 必读文档

执行前阅读：

1. `Agent/02_SkillSystem/2026-07-13_Phase_PlayerReplacement_MovementPlan.md`
2. `Agent/04_AgentHandoff/Claude_Reports/2026-07-13_PhaseSpellValidation_Precheck_Report.md`
3. `.agents/ue-project-context.md`
4. 如涉及动画通知或 Montage，参考 `Agent/02_SkillSystem/Montage_GameplayEvent_Timing_Decision.md`

## 背景决策

- Phase 已确认作为法术玩家验证角色。
- 推荐优先使用 Aura 现有玩家 C++/GAS 路径承载 Phase 的 Mesh 和 AnimBP。
- 不建议直接把 Phase 自带 Character Blueprint 作为最终玩家类，因为这可能绕过 Aura 现有的 `AAuraCharacter`、PlayerState ASC、输入和 UI 初始化。
- 如果需要新建测试蓝图，优先命名为 `BP_AuraCharacter_PhasePrototype`，继承当前 Aura 玩家蓝图或 Aura 玩家 C++ 类。
- 第三方原始资产保留在 `/Game/ThreePartAsset`，不要移动、重命名或批量修改。

## 建议执行顺序

### 1. 资产确认

在 UE 编辑器中确认以下资产能打开和预览：

- Phase SkeletalMesh：`/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC`
- Phase Skeleton：`/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/phase_Skeleton`
- Phase AnimBlueprint：查找并确认 `Phase_AnimBlueprint`
- Phase Character Blueprint：查找并确认 `PhasePlayerCharacter`
- 基础移动相关动画或 AnimBP 状态机是否可用

同时检查：

- Phase Skeleton 是否有手部 socket，尤其是右手、左手、武器/法术发射相关 socket。
- Phase AnimBP 是否依赖 Phase 自带 Character Blueprint 的自定义变量。

### 2. 选择最小接入方案

优先方案：

- 新建或复制一个 Aura 玩家蓝图原型，例如 `/Game/Aura/SkillPrototype/Phase/BP_AuraCharacter_PhasePrototype`。
- 继承 Aura 当前玩家角色路径，保留 Aura C++ / GAS / 输入 / UI 初始化。
- 将 Mesh 替换为 Phase SkeletalMesh。
- 将 AnimClass 设置为 Phase AnimBlueprint。
- 调整 Mesh 相对位置、旋转、缩放，使角色和 Capsule 匹配。

如果 Phase AnimBP 无法直接工作：

- 先记录原因。
- 可以创建一个最小 Phase locomotion AnimBP 或临时使用 Phase 自带蓝图验证，但必须说明这是否绕过 Aura GAS 玩家路径。

### 3. 默认玩家接入

让 PIE 能使用 Phase 原型角色。

可选方式：

- 修改当前测试 GameMode 的 Default Pawn Class 指向 Phase 原型蓝图。
- 或在当前地图/测试地图中临时指定 Phase 原型玩家角色。

要求：

- 尽量不要覆盖旧 `BP_AuraCharacter`，保留可回退路径。
- 如果必须修改已有蓝图，请在报告中明确说明修改了什么、为什么不能新建。

### 4. 基础移动验证

验证：

- 单人 PIE：Phase 出生、摄像机跟随、移动输入正常。
- Idle / Walk / Run 动画切换正常。
- Jump / Falling / Land 如果当前输入和 AnimBP 支持则验证；如果不支持，写入报告，不强行扩展。
- Capsule、Mesh、相机没有明显错位。
- Output Log 中没有新的 GAS 初始化错误。

多人基础检查：

- Listen Server + 1 Client。
- 主机和客户端都能看到 Phase。
- 远端能看到移动复制和基础动画。

### 5. 报告

写中文报告到：

`Agent/04_AgentHandoff/Claude_Reports/2026-07-13_Phase_PlayerReplacement_BasicMovement_Report.md`

报告必须包含：

- 修改/新增的资产路径。
- 修改/新增的 C++ 文件路径，如果有。
- 使用的 Phase Mesh、Skeleton、AnimBP、Blueprint 路径。
- 是否保留 Aura GAS 初始化链路。
- 单人 PIE 验证结果。
- Listen Server / Client 验证结果。
- 当前阻塞或遗留问题。
- 如果使用 MCP 操作 UE 编辑器，列出关键 MCP 操作步骤。

## 代码与注释要求

本阶段原则上优先资产/蓝图配置，不优先写 C++。

如果确实需要写 C++：

- 注释要说明为什么需要代码，而不是只说明语法。
- 不要引入大范围重构。
- 不要改技能运行时主架构。
- 新增文件必须放在正确的 `Source/Aura/Public` / `Source/Aura/Private` 路径。
- 检查 `Aura.Build.cs` 是否需要依赖变更。

## 禁止事项

- 不设计新技能。
- 不做完整准星视角。
- 不批量 retarget。
- 不重构 `UAuraSkillDefinition`、`UAuraSkillRuntimeAbility`、SkillTemplate、CombatEntity。
- 不移动第三方原始资源。
- 不删除旧 Aura 玩家蓝图。
- 不把 Phase 自带 Character Blueprint 直接定为长期玩家基类，除非报告中说明只是短期验证。

## 验收标准

任务完成时应满足：

1. Phase 可以作为玩家角色进入 PIE。
2. 基础移动输入可用。
3. 基础 locomotion 动画可见。
4. Aura 的 PlayerState ASC / 属性 / UI 初始化没有被明显破坏。
5. 至少完成单人 PIE 验证。
6. 如果多人验证失败，必须说明失败原因和下一步修复建议。
