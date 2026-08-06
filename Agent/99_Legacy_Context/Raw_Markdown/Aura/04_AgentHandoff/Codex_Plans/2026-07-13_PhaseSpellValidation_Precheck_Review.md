# Codex 审查：Phase 法术验证角色预检报告

审查时间：2026-07-13

被审查报告：

`Agent/04_AgentHandoff/Claude_Reports/2026-07-13_PhaseSpellValidation_Precheck_Report.md`

## 结论

Claude 的方向基本可用：`ParagonPhase` 确实适合作为法术标准验证角色候选，并且项目中存在 Phase 的 SkeletalMesh、Skeleton、AnimBlueprint、Character Blueprint、若干动作序列和可用 Niagara。

但报告不能原样作为下一阶段依据，主要有两处需要修正：

1. `Cast_MSA.uasset` 不是 Montage。二进制字符串中明确出现 `AnimSequence`，并且 `AdditiveAnimType=AAT_RotationOffsetMeshSpace`，它更像 Cast 的 Mesh Space Additive 变体。下一阶段仍应创建我们自己的 `AM_Phase_Cast_Fireball_Test`。
2. `PhasePlayerCharacter` 是第三方 `Character` 蓝图，不是 Aura/GAS 角色。它可以用于视觉和动画预览，但不能直接等价参与 Aura 当前技能系统验证。若要验证 GAS 火球链路，推荐创建 Aura 体系内的 Phase 测试角色蓝图：继承/复用 Aura 现有玩家角色链路，只替换 Mesh 和 AnimBP。

## 本地复核发现

### 已确认或高度可信

- `Phase_GDC.uasset` 包内包含 `SkeletalMesh`，并引用 `phase_Skeleton`。
- `phase_Skeleton.uasset` 包内包含 `Skeleton` 和 `SkeletalMeshSocket` 字符串。
- `Phase_AnimBlueprint.uasset` 包内包含 `AnimBlueprint` 和 `AnimBlueprintGeneratedClass`，目标骨架是 `phase_Skeleton`。
- `PhasePlayerCharacter.uasset` 包内包含 `Blueprint`、`BlueprintGeneratedClass`，父类是 `/Script/Engine.Character`。
- `NS_Free_Magic_Projectile1.uasset` 包内包含 `NiagaraSystem`，可以认为该类 `NS_*` 资源是 Niagara 系统。

### Socket 候选

虽然没有通过 UE 编辑器查看坐标，但 `phase_Skeleton` 中已经能看到以下疑似 socket / 骨骼命名：

- `FX_Hand_R1`
- `FX_Hand_R2`
- `FX_Hand_R3`
- `FX_Hand_R4`
- `FX_Hand_L1`
- `FX_Hand_L2`
- `FX_Hand_L3`
- `FX_Hand_L4`
- `Muzzle_01`
- `Muzzle_02`
- `Muzzle_03`
- `WeaponAttachPointR`
- `WeaponAttachPointL`
- `weapon_r`
- `weapon_l`
- `Impact`

火球释放点下一阶段优先检查 `FX_Hand_R*` 和 `Muzzle_*`。最终仍需在 UE 编辑器中打开 Skeleton Tree 确认位置和朝向。

## 审查问题

### P1：`Cast_MSA` 被误判为可能 Montage

报告中写到 `Cast_MSA` 可能是 AnimMontage，且可能直接作为施法 Montage 使用。这个判断不成立。`Cast_MSA` 的资产字符串显示它是 `AnimSequence`，且为 Mesh Space Additive 变体。

影响：如果下一阶段按报告直接尝试复用 `Cast_MSA` 作为 Montage，会浪费时间，并且无法添加我们需要的 Montage GameplayEvent Notify 流程。

修正：下一阶段创建 Aura 自己的测试 Montage，例如：

`/Game/Aura/SkillPrototype/Phase/AM_Phase_Cast_Fireball_Test`

源动作优先用 `Cast` 或 `RMB_Throw`，不是 `Cast_MSA`。

### P1：`PhasePlayerCharacter` 不能直接代表 Aura/GAS 验证角色

报告说 Phase 可以直接作为法术验证角色，这个说法需要限定范围。`PhasePlayerCharacter` 是普通第三方 Character 蓝图，可以用于视觉、动作和资源预览，但它不继承 Aura 的玩家角色链路，不天然拥有 Aura 的 ASC 初始化、属性、输入绑定、技能授予和 UI 逻辑。

影响：如果直接把 `PhasePlayerCharacter` 放进游戏流程，技能系统验证会和 Aura 现有 GAS 链路断开。

修正：推荐创建 Aura 体系内的 Phase 测试角色蓝图：

- 基类仍走 `AAuraCharacter` / `BP_AuraCharacter` 体系。
- Mesh 使用 `Phase_GDC`。
- Anim Class 使用 `Phase_AnimBlueprint`。
- 技能、属性、输入、ASC 初始化继续走 Aura 原链路。

### P2：Socket 不是完全未知，但仍需编辑器确认位置

报告说 Socket 信息无法确认，这在“位置/朝向”层面是对的；但从 `phase_Skeleton` 字符串看，Phase 已经有多个手部、Muzzle、WeaponAttachPoint 候选。

修正：下一阶段不应默认“没有 socket”。应先在编辑器里确认以下候选：`FX_Hand_R1`、`FX_Hand_R2`、`Muzzle_01`、`WeaponAttachPointR`。

## 下一阶段建议

下一阶段目标应从“是否能用 Phase”推进到“用 Aura 体系创建 Phase 火球最小验证角色”。

建议任务拆成两步：

1. 编辑器资产准备：
   - 创建或复制到 Aura 自己目录的测试 Montage：`AM_Phase_Cast_Fireball_Test`。
   - 在释放帧添加 GameplayEvent Notify，事件先沿用当前火球事件 tag，后续再统一重命名。
   - 在编辑器里确认火球发射 socket，优先 `FX_Hand_R*` 或 `Muzzle_*`。
2. Aura Phase 测试角色：
   - 创建 `BP_AuraPhaseSpellValidator`，继承/复用 Aura 现有玩家角色蓝图。
   - 替换 Mesh 为 `Phase_GDC`，AnimBP 为 `Phase_AnimBlueprint`。
   - 不改第三方 `PhasePlayerCharacter`。
   - 临时在关卡或 GameMode 中切换为该测试角色，用当前火球 DA 验证动画事件、投射物、命中和多人显示。

## 暂不建议

- 不建议 retarget 到 Aura 当前主角，除非 Phase 直接链路失败。
- 不建议直接使用第三方 `PhasePlayerCharacter` 进入 Aura GAS 验证。
- 不建议修改第三方原始动作资产。
- 不建议把 `Cast_MSA` 当 Montage 使用。
