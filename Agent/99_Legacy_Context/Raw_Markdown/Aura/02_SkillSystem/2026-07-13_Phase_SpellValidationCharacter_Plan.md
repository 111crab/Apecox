# Phase 法术标准验证角色接入计划

更新时间：2026-07-13

## 背景

已确认 `ParagonPhase` 作为法术类技能的标准验证角色。它的职责不是替换项目最终主角，而是先作为“技能链路验证角色”和未来技能编辑器预览角色候选，用来验证法术动作、蒙太奇事件、发射点、VFX、命中、多人表现。

本阶段不急着修改核心技能运行时代码，也不急着把 Phase 变成正式玩家角色。先做资产预检，确认 Phase 是否能低成本接入。

## 本阶段目标

1. 确认 Phase 的关键资产真实类型：SkeletalMesh、Skeleton、AnimSequence、AnimMontage、AnimBlueprint、Particle/Niagara。
2. 确认 Phase 是否可以直接作为测试 Pawn/Character 使用，还是必须 retarget 到当前 Aura 主角。
3. 选出第一批法术动作候选：火球释放、引导治疗、光束/连线。
4. 确认手部 socket 或骨骼名，判断火球、光束、治疗 VFX 能否稳定附着/发射。
5. 输出一份可执行报告，为后续“创建 Phase 测试蒙太奇 + 配 GameplayEvent Notify + 验证火球”做准备。

## 关键目录

Phase 角色目录：

- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase`

Phase Mesh/Skeleton 候选：

- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/phase_Skeleton`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_Extents`

Phase 动作候选：

- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/Cast`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/RMB_Intro`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/RMB_Loop`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/RMB_Throw`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/Ability_Q`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/Ability_E`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/R_Ability_Intro`
- `/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase/Animations/R_Ability_Loop`

可优先搭配的 Niagara：

- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile2`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Healing`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing`

## 决策原则

### 先直接用 Phase，暂不 retarget

如果 Phase 的 SkeletalMesh、Skeleton、动作和基本预览都正常，第一条验证路线应当是“直接使用 Phase 自身骨架创建测试 Montage”。这样避免一开始就把问题混在 retarget、主角 AnimBP、技能运行时三层里。

### 只迁移精选资产

第三方原包继续留在 `/Game/ThreePartAsset`。我们自己的试验资产后续应放在 `/Game/Aura/SkillPrototype/Phase` 或类似目录，通过软引用指向原始 VFX/动作，不直接改第三方原始资源。

### GameplayEvent Notify 仍然是 V1 关键触发方式

火球术仍采用 Montage 关键帧发送 GameplayEvent 的方式。例如在释放帧发送 `Event.Montage.FireBolt` 或后续新命名的火球释放事件。音效、普通粒子、脚步这类表现通知不承担 GA 关键逻辑。

## 阶段拆分

### A. 资产预检

只读检查，不改资产：

1. 列出 Phase Mesh/Skeleton/AnimBP/动作资产的真实 UE 类型。
2. 检查是否有可直接打开的 Phase 角色蓝图或 AnimBP。
3. 检查 Phase Skeleton 上是否有手部 socket，尤其是右手、左手、武器/法术发射相关 socket。
4. 检查 `Cast`、`RMB_Throw`、`RMB_Loop` 是 AnimSequence 还是 AnimMontage。
5. 检查是否已有适合直接播放的 Montage。

输出结论：

- Phase 是否能直接作为测试角色。
- 如果不能，阻塞原因是什么。
- 是否需要 retarget 到 Aura 当前主角。

### B. 编辑器可视化预览

人工或 MCP 辅助打开资源：

1. 预览 Phase Mesh 是否正常。
2. 预览 `Cast`、`RMB_Throw`、`RMB_Loop` 动作是否符合火球/引导治疗。
3. 预览 `Free_Magic`、`Free_Spells` 的 Niagara 是否比例合适。
4. 截图或记录最适合的动作和 VFX。

### C. 最小试验资产

只有 A/B 通过后再做：

1. 在 Aura 自己目录创建测试 Montage，例如 `AM_Phase_Cast_Fireball_Test`。
2. 在释放帧添加 GameplayEvent Notify。
3. 如果需要测试治疗，引导动作可从 `RMB_Loop` 或 `R_Ability_Loop` 里选择。
4. 不直接修改第三方原始动作资产。

### D. 火球复验

用 Phase 动作 + 选中的 VFX 重新验证火球术：

1. 单人 PIE：动作、事件、投射物、命中、伤害都正常。
2. Listen Server + Client：其他端能看到动作、投射物和命中表现。
3. 日志能确认 GameplayEvent 只在正确帧触发。

## 本阶段不做

- 不重构技能系统核心代码。
- 不把 Phase 直接替换成正式玩家角色。
- 不批量 retarget 全部动画。
- 不移动或重命名第三方原始资产。
- 不为了表现资产临时引入大量硬编码路径。

## 下一份 ClaudeCode 任务

让 ClaudeCode 执行“Phase 法术验证角色资产预检”，只输出报告，除非明确必要不要创建资产。报告写入：

`Agent/04_AgentHandoff/Claude_Reports/2026-07-13_PhaseSpellValidation_Precheck_Report.md`

预检通过后，我们再决定是否执行“创建 Phase 测试 Montage + 配 GameplayEvent Notify + 火球复验”的下一阶段。
