# ClaudeCode 执行任务：Phase 法术验证角色资产预检

## 背景

项目：`D:\UnrealProject\Aura`

我们已确认 `ParagonPhase` 作为法术类技能的标准验证角色。它用于验证火球、引导治疗、光束/连线、领域等法术技能链路，也可能成为未来技能编辑器的预览角色候选。

本任务是资产预检，不是代码实现任务。请尽量只读检查，不要重构代码，不要移动或重命名第三方 `.uasset`。

参考文档：

- `Agent/02_SkillSystem/2026-07-13_ThirdPartyAsset_Audit_For_Skill_Prototype.md`
- `Agent/02_SkillSystem/2026-07-13_Phase_SpellValidationCharacter_Plan.md`

## 目标

输出一份 Phase 是否适合作为法术标准验证角色的预检报告。

报告路径：

`Agent/04_AgentHandoff/Claude_Reports/2026-07-13_PhaseSpellValidation_Precheck_Report.md`

## 检查范围

Phase 角色目录：

`/Game/ThreePartAsset/ParagonPhase/Characters/Heroes/Phase`

磁盘路径：

`D:\UnrealProject\Aura\Content\ThreePartAsset\ParagonPhase\Characters\Heroes\Phase`

重点检查：

1. Mesh/Skeleton：
   - `Meshes/phase_Skeleton`
   - `Meshes/Phase_GDC`
   - `Meshes/Phase_Extents`
   - 真实 UE 类型分别是什么。
   - 哪个是可用 SkeletalMesh。
   - Skeleton 上是否有手部 socket 或发射点相关 socket。
2. 动作候选：
   - `Animations/Cast`
   - `Animations/RMB_Intro`
   - `Animations/RMB_Loop`
   - `Animations/RMB_Throw`
   - `Animations/Ability_Q`
   - `Animations/Ability_E`
   - `Animations/R_Ability_Intro`
   - `Animations/R_Ability_Loop`
   - 它们分别是 AnimSequence 还是 AnimMontage。
   - 哪些适合火球释放、哪些适合引导治疗、哪些适合光束/连线。
3. 已有 Montage：
   - 查找 Phase 动作目录中已有的 `*Montage*` 资产。
   - 判断是否有可以直接复用的 Montage。
4. AnimBP/角色蓝图：
   - 查找 Phase 是否已有 AnimBlueprint 或角色 Blueprint。
   - 如果有，记录路径和真实类型。
   - 如果没有，说明直接作为测试角色时还缺哪一步。
5. VFX 候选：
   - `Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`
   - `Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile2`
   - `Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1`
   - `Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2`
   - `Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Healing`
   - `Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing`
   - 确认它们真实类型是否是 Niagara System。

## 工具使用建议

优先使用 UE 编辑器的 Asset Registry、MCP 只读查询或编辑器内查看方式确认真实类型。可以用文件名和目录做辅助，但报告里要区分：

- “已由 UE/AssetRegistry 确认”
- “仅由文件名推断”

如果 MCP 能稳定读取资产类型，可以使用 MCP；如果 MCP 对这些资产能力有限，不要强行做大量编辑器自动化，直接记录限制并给出手动检查步骤。

## 严格限制

1. 不要移动、删除、重命名第三方资产。
2. 不要直接修改第三方原始 `ParagonPhase` 动作、Skeleton、Mesh。
3. 本任务默认不创建新资产。如果你认为必须创建测试资产，先在报告中写“建议下一阶段创建”，不要现在创建。
4. 不要改 C++ 源码。
5. 不要改现有 SkillDefinition、GA、GE、输入绑定。

## 报告格式

请用中文报告，至少包含：

1. 结论摘要：
   - Phase 是否能直接作为法术验证角色。
   - 是否必须 retarget。
   - 当前最大风险。
2. 资产类型表：
   - 路径
   - 真实类型
   - 是否已确认
   - 备注
3. 动作候选表：
   - 动作路径
   - 类型
   - 推荐用途：火球 / 引导治疗 / 光束连线 / 暂不推荐
   - 原因
4. Socket / 发射点检查：
   - 找到哪些手部或武器 socket。
   - 火球建议使用哪个 socket。
   - 如果没有 socket，下一阶段怎么补。
5. VFX 候选表：
   - 路径
   - 类型
   - 推荐用途
6. 下一阶段建议：
   - 是否创建 `AM_Phase_Cast_Fireball_Test`
   - 是否需要创建 Phase 测试 Character Blueprint
   - 是否先直接用 Phase 骨架，还是 retarget 到 Aura 主角

## 验收标准

任务完成时应满足：

1. 没有修改任何 C++。
2. 没有修改第三方原始资产。
3. 产出上述报告。
4. 报告足够让 Codex 和用户决定下一阶段是否创建测试 Montage 和测试角色。
