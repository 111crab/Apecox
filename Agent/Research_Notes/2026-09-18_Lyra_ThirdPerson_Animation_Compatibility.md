# Lyra 第三人称步枪动画兼容性分析

更新日期：2026-09-18

## 结论

可以采用 Lyra 的第三人称持枪动画序列、Blend Space、Aim Offset、Montage 和 Manny Mesh/Skeleton，但不应直接迁入 Lyra 的整套 `ABP_Mannequin_Base`、`ABP_RifleAnimLayers` 与武器运行时。

Apecox 已具备双视角表现的主要骨架：第三人称完整角色 `GetMesh()`、第一人称 Arms、第一人称武器 Presentation Actor、第三人称武器 Mesh，以及由同一装备/开火事务选择 FP 或 TP 表现的代码路径。玩家始终使用第一人称操控；改造重点是给其他观察者看到的 TP 角色补齐动画图、动作表现和复制状态，不新增本地第三人称相机。

第一人称和第三人称的“同步”定义为：共享同一份 Gameplay 状态与动作事件，分别播放适合各自骨架和镜头的动画。两条 Montage 不要求逐帧相同，也不能分别修改弹药、生成子弹或结算伤害。

## 对“双视角初版启动小贴士”的判断

`Agent/99_Legacy_Context/双视角初版启动小贴士.md` 的核心原则正确，可直接采用：

- FP Arms 是本地 ViewModel，TP Character 是世界空间角色表现。
- 移动、开火、换弹、装备等状态只能有一个 Gameplay 权威来源。
- FP 与 TP 动画分别消费同一个行为语义，不复制 Pose，也不要求使用同一条 Sequence。
- Gameplay Notify 不能在 FP/TP 两边重复执行弹药、伤害和 Projectile 逻辑。
- TP Locomotion 与 UpperBody Action 适合使用缓存姿势、Slot 和 `Layered Blend Per Bone` 组合。
- Lyra 适合作为动画资产和设计参考，不适合为了几条序列迁入全部框架。

需要按 Apecox 现状修正三点：

1. Apecox 已有 FP/TP Character Mesh 和 FP/TP Weapon 表现，不需要重新建立“双 Mesh”，也不需要本地第三人称模式。现有 `OnlyOwnerSee/OwnerNoSee` 正好表达 Owner FP、Observer TP。
2. 当前只做一把步枪。现阶段直接扩展现有 `UApecoxWeaponPresentationDefinition` 比新建 GameplayTag 查询式通用 Animation Set 更简单，也能避免新的并行配置真相。
3. CMC 可以直接提供 Velocity、Direction、Falling、Crouch；Fire 已由现有 GameplayCue 分发。只对 Sprint、Prone、ADS、Lean、Reload/Inspect 等缺少远端真相的状态补复制，不复制整套动画状态或骨骼 Pose。

## Apecox 当前兼容基础

### 已经存在

- `AApecoxPlayerCharacter::GetMesh()`：第三人称完整角色 Mesh，当前设置 `OwnerNoSee=true`。
- `FirstPersonMesh`：附着在第一人称相机上的 Arms，设置 `OnlyOwnerSee=true`。
- `UApecoxEquipmentComponent::ThirdPersonWeaponMeshComponent`：由装备摘要创建并附着到 TP Mesh，设置 `OwnerNoSee=true`。
- `UApecoxCharacterAnimInstance`：FP/TP 可共用的状态读取基类，已有 GroundSpeed、VerticalSpeed、MovementDirection、Moving、Falling、Crouching、Sprinting、Prone、Lean、AimPitch 和 AnimationFamily。
- `GameplayCue.Weapon.Fire` 路径：Owning Client 播放 FP Fire，其他观察者播放 TP Character/Weapon Fire 与枪口表现。
- `UApecoxWeaponPresentationDefinition`：已有 TP Weapon Mesh、Socket、Transform、TP Character Fire Montage 和 TP Weapon Fire Montage。

### 仍然缺少

- Listen Server/Client 下远端 TP Mesh、Weapon 附着和 Anim Class 的实际验证。
- 一个真正消费 `AnimationFamily=Rifle` 的第三人称 AnimBP。当前 `ABP_Apecox_Manny` 只有模板空手 Idle/Walk/Jump，不能作为最终步枪图继续堆叠。
- TP Rifle Idle、Walk/Jog Strafe、Jump/Fall/Land、Crouch、Aim Offset、Turn In Place、UpperBody Slot 和 IK。
- `AimYaw`、ADS/Gait 等 TP AnimBP 所需缓存。
- TP Equip、Tactical Reload、Empty Reload、Inspect 表现字段和播放路径。
- Sprint、Prone、ADS、Lean、Reload/Inspect 等远端表现状态。当前这些值主要是拥有客户端本地字段；直接让 Simulated Proxy 的 AnimBP 读取会失真。
- 多人回归：Owning Player 的 FP 表现与 Remote Observer 的 TP 表现必须消费同一动作语义。

## Lyra 的实际结构与不能整套照搬的原因

Lyra 的 Manny 第三人称表现以 `ABP_Mannequin_Base` 为基础，通过 `ALI_ItemAnimLayers` 链接 `ABP_ItemAnimLayersBase` 或 `ABP_RifleAnimLayers`。武器实例通过 `EquippedAnimSet` 选择动画层。

它还依赖 Lyra 自己的 `ULyraAnimInstance`、GameplayTag 属性映射、自定义 CharacterMovement 的 GroundDistance、Animation Locomotion Library、Animation Warping、Control Rig、Foot Plant，以及 Experience/GameFeature/武器实例结构。

如果直接迁入 Lyra AnimBP，会把 Apecox 现有 Character、CMC、GAS 和 Equipment 之外的第二套运行时带进项目，并产生状态来源冲突。正确方式是保留 Apecox 运行时，只把 Lyra 的动画资产接进 Apecox 自己的 TP AnimBP。

## 推荐采用的 Lyra 资产

首批只迁移满足基础闭环的最小集合，依赖由 Unreal Editor 的 Migrate 自动带入。

### Mesh 与 Skeleton

- `/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny`
- `/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin`

### 基础持枪 Locomotion

- `MM_Rifle_Idle_Hipfire`
- `MM_Rifle_Idle_ADS`
- `MM_Rifle_Walk_Fwd`、`MM_Rifle_Walk_Bwd`、`MM_Rifle_Walk_Left`、`MM_Rifle_Walk_Right`
- `MM_Rifle_Jog_Fwd`、`MM_Rifle_Jog_Bwd`、`MM_Rifle_Jog_Left`、`MM_Rifle_Jog_Right`
- `MM_Rifle_Jump_Start`、`MM_Rifle_Jump_Start_Loop`、`MM_Rifle_Jump_Apex`、`MM_Rifle_Jump_Fall_Loop`、`MM_Rifle_Jump_Fall_Land`
- `MM_Rifle_Crouch_Idle`
- `BS_MM_Rifle_Crouch_Walk`
- `BS_MM_Rifle_Jog_Leans`

### Aim Offset

- `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/AO_MM_Rifle_Idle_Hipfire`
- `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/AO_MM_Rifle_Idle_ADS`
- `/Game/Characters/Heroes/Mannequin/Animations/AimOffsets/AO_MM_Rifle_Crouch_Idle`

### 一次性动作

- `MM_Rifle_Equip` / `AM_MM_Rifle_Equip`
- `MM_Rifle_Fire` / `AM_MM_Rifle_Fire`
- `MM_Rifle_Reload` / `AM_MM_Rifle_Reload`
- 可选的武器机械动作 `AM_Weap_Rifle_Fire`、`AM_Weap_Rifle_Reload`

起步阶段不迁移全套 Start/Stop/Pivot/Turn 动作。基础方向移动、腾空、瞄准和三项武器动作稳定后，再加入 Turn In Place 与起停过渡，避免一次引入过多变量。

## Skeleton 方案

Lyra 的 `SK_Mannequin` 与 Apecox 当前模板路径下的 `SK_Mannequin` 是两个不同的 Skeleton 资产。骨骼体系虽然都是 UE5 Manny，但不能仅凭同名认为动画可以直接播放。

首选方案是迁移 Lyra 的 `SKM_Manny`、`SK_Mannequin` 和精选动画，让新的 `ABP_Apecox_Rifle_TP` 直接以 Lyra Skeleton 为 Target Skeleton。FP RAR Arms 完全不受影响。这样风险最低，也最接近 Lyra 动画的原始效果。

如果必须保留当前 TP Mesh，则需要在编辑器内显式配置 Compatible Skeleton 或使用 IK Retargeter 生成 Apecox 副本，并逐项检查 Root、IK、Weapon Socket、曲线和 Montage Slot。该方案工作量更高，不作为第一版默认路径。

## Apecox 改造结构

### 1. 观察者可见性基线

- 不新增 SpringArm、第三人称 Camera、视角枚举或切换输入。
- Owning Player 始终显示 FP Arms/FP Weapon，并通过 `OwnerNoSee` 隐藏自己的 TP Manny/TP Weapon。
- 其他客户端始终显示该 Pawn 的 TP Manny/TP Weapon，并通过 `OnlyOwnerSee` 看不到其 FP ViewModel。
- 射击瞄准、Projectile 发射和服务器命中继续使用现有 Gameplay 逻辑，不复制第二套武器事务。

### 2. TP AnimBP

- 新建 `ABP_Apecox_Rifle_TP`，父类使用 `UApecoxCharacterAnimInstance`，Target Skeleton 使用迁移后的 Lyra `SK_Mannequin`。
- Base Locomotion 先覆盖 Idle、Walk/Jog、Jump/Fall/Land、Crouch。
- Rifle/Unarmed 由现有 `AnimationFamily` 选择；当前只有一把步枪，不先实现 Lyra Linked Anim Layer 框架。
- 在 Base Pose 上应用 Aim Offset，再通过 UpperBody Slot 合成 Fire/Reload/Equip。
- 左手 IK、脚 IK 和 Turn In Place 位于动作链末端，后续分批加入。

### 3. 共享动画状态

- 保留 CMC 派生的 GroundSpeed、Direction、VerticalSpeed、Falling、Crouch。
- 在共享 AnimInstance 增加标准化 `AimYaw`、ADS Alpha/Gait 等表现值。
- 不让 AnimBP 自己决定能否开火、是否完成换弹或应扣多少子弹。
- 清理/替换 `ABP_Apecox_Manny` 中自建且与原生属性重名的变量，避免 Blueprint 继续缓存旧状态。

### 4. 一次性动作

- Fire 继续沿用现有 GameplayCue，补齐 TP AnimBP Slot 即可。
- Equip、Reload 在现有 `UApecoxWeaponPresentationDefinition` 中增加 TP Montage 字段，由现有 Equipment/Reload 生命周期触发。
- Inspect 当前可先仅保留 FP；若需要让远端看到，再增加 TP Inspect 动作。Lyra 没有与 RAR 检视一一对应的现成动作，不能用 Reload 冒充。
- Gameplay 时点只由 GAS/Equipment/Weapon 事务决定；TP Montage Notify 只负责声音、弹匣可见性等表现。

### 5. 复制边界

- Velocity、Falling、Crouch：继续依赖 CMC/Character 原生复制。
- 装备类型：继续依赖 `EquippedWeaponState`。
- Fire：继续依赖 `GameplayCue.Weapon.Fire`。
- Sprint、Prone、ADS、Lean：增加紧凑的角色表现复制状态。
- Reload/Equip/Inspect：复制动作开始与必要的分支/序号，或使用既有服务器动作事件；不能依赖 OwnerOnly 的武器实例状态让远端猜测。

## FP 与 TP 动画时长不同的处理

FP 与 TP Montage 不需要总时长一致，但必须对齐 Gameplay 关键阶段。以 Reload 为例，`ReloadConfig` 中的统一事务时间继续作为真相：

- `StartTime`：服务器确认进入 Reload。
- `CommitTime`：弹药真正转移，是唯一允许修改弹药的时点。
- `FinishTime`：解除 Reload 阻塞，重新允许开火。

FP 和 TP Montage 只负责覆盖这条时间线。接入每个动作时记录各自原始长度，并计算播放倍率，使关键姿势落在 `CommitTime` 附近；如果单一倍率会破坏观感，则把 Montage 分成 Start/Loop/End 或 Remove/Insert/Finish Section，分别对齐阶段。动画结束 Notify 只能处理声音、弹匣显示等表现，不能提交弹药或结束服务器 Reload。

Fire 由逐发 GameplayCue 驱动，不需要按 Montage 总长同步；Equip 和 Inspect 同样由统一动作开始/结束事件控制。客户端迟到收到事件时可以用服务器开始时间计算 Montage 起播位置，而不是从第 0 帧追赶整段动画。首轮 Locomotion 是连续状态混合，没有动作时长对齐问题。

## 当前第一人称 AnimBP 的复杂度判断

当前 `ABP_Apecox_FirstPersonArms` 使用状态机、姿势混合、Additive、Slot、FABRIK 和 IK 节点组合第一人称姿势，这是 Unreal 动画系统的正常实现方式。AnimGraph 本来就应该负责“如何组合 Pose”，节点较多本身不是架构问题。

当前职责边界也基本正确：`UApecoxCharacterAnimInstance` 与 `UApecoxFirstPersonAnimInstance` 在 C++ 中缓存 Gameplay/CMC 状态、ADS、转向、后坐和左手握点变换；AnimBP 读取结果并组合 RAR 动画，不在图中扣弹、生成 Projectile 或判定伤害。

后续应遵守以下边界，避免继续失控：

- EventGraph 不维护第二份 Gameplay 状态，不通过 Tick 猜测 Reload、Fire 或装备结果。
- 重复使用的 Locomotion、Aim、Recoil、IK 分支先 `Save Cached Pose`，再在明确的层次中组合。
- 连续姿态使用 State Machine/Blend Space；一次性动作使用 Montage/Slot；骨骼修正使用 IK/Control Rig，各自不要互相替代。
- 运算和跨组件坐标转换留在原生 AnimInstance；AnimGraph 只处理 Pose 选择、混合和骨骼节点。
- TP 新建独立 `ABP_Apecox_Rifle_TP`，不向已经复杂的 FP AnimBP 继续加入第三人称分支。二者共享原生状态基类，不共享同一张 AnimGraph。

因此当前 FP AnimBP 不需要为了第三人称重构。只在后续维护中逐步整理缓存姿势和图内分区；第三人称从一张职责清晰的新 AnimBP 开始。

## 对“后续动画组织建议”的判断

`Agent/99_Legacy_Context/后续动画组织建议.md` 可以作为后续维护约束，以下内容与当前项目一致并予以采用：

- 保留第一人称的 Pose Composition，不因为图较大就推翻重做。
- 连续状态由 State Machine、Blend Space 和 Additive 组合；Fire、Reload、Equip 等一次性动作继续使用 Montage/Slot。
- Recoil 先作用于武器/手臂链，左手 IK 在其后重新贴合握点；Reload、Equip、Inspect 期间继续用 Alpha 关闭 IK。
- EventGraph/AnimInstance 只准备动画数据，不能处理扣弹、伤害、Ability 激活和换弹许可。
- TP 使用独立 AnimBP；FP/TP 共享 Gameplay 状态，不共享最终 Pose、动画序列和 IK 图。
- 当前只有 `Unarmed/Rifle` 两个 AnimationFamily，不为了形式提前引入 Linked Anim Layer。以后出现多种真正不同的武器 Pose 家族并产生重复图时再模块化。
- Cached Pose 只用于同一 Pose 被多个下游分支消费的场合；Comment Box、变量命名和失效节点清理可以随每次相关修改轻量完成。

以下内容需要作为原则而不是固定接线模板：

- 文档中的 Pipeline 是语义分层示意，不能据此假定当前二进制 AnimGraph 的每个节点已经严格按该顺序存在。
- `AnimationFamily` 更适合先选择 Rifle/Unarmed 的基础 Pose 或子图，再叠加可共享的移动、Aim、Montage 和 IK；不应固定放在全部 Aim/Air 层之后。
- Additive 顺序取决于 Local Space/Mesh Space 类型、动画作者的基准 Pose 和目标骨骼。Aim、Lean、Airborne 的顺序必须以资产预览和实际握持结果验证，不能建立全项目唯一的机械顺序。
- Montage Slot 的位置取决于动作范围。Fire 通常是 UpperBody；含明显全身重心变化的动作可能需要 FullBody。不能规定所有 Montage 都处于同一个层级。
- “三套以上武器才拆 Linked Layer”是有用的经验线，不是硬阈值；真正判断标准是重复量、修改遗漏风险和公共/武器特有职责是否已经难以分开。

本阶段据此只做一项结构决定：新建职责简单的 `ABP_Apecox_Rifle_TP`，不重构已通过验收的 FP AnimBP，也不提前搭建多武器 Animation Layer 框架。

## 建议实施顺序

1. 在 Listen Server + Client 中确认 Owner FP、Observer TP、远端武器附着和装备动画族复制基线。
2. 迁移 Lyra Manny 与最小 Rifle Idle/Walk/Jog/Jump 资产，建立新的 TP AnimBP。
3. 只接入 Rifle Idle、四向移动和 Jump/Fall/Land，完成第一轮持枪移动同步验证。
4. 基础移动通过后，接入 Crouch、Aim Offset、Sprint、Prone、Lean 与 Turn In Place。
5. 接入 Fire、Equip、普通/空仓 Reload 的 TP Montage 和 UpperBody 混合。
6. 增加左手 IK、脚 IK以及 Reload/Inspect 等远端动作事件，并做完整双端回归。
7. 最后让静止 Bot 复用相同 TP AnimBP；自由移动 AI 仍按当前范围暂缓。

## 首轮验收边界

首轮只要求：玩家 1 与玩家 2 都保持第一人称操控；各自拾取步枪后，另一端看到正确附着的 TP Rifle；站立、前后左右移动和行走/快速移动能随 CMC 状态正确切换；Owner 的 FP 表现不回归。

Jump/Fall/Land 已迁入但留到第二轮接入。Aim、Fire、Reload、Equip、Turn In Place、IK 和更完整的远端动作事件按后续轮次分别验收，避免把骨架、Locomotion、Montage 和复制问题同时混在一轮排查。
## 2026-09-18 资产落地结果

- 迁入 13 条 Rifle 动画后，删除了 LyraGame 专用 Anim Notify 轨道和已应用 Animation Modifier 元数据，保留原始骨骼关键帧、曲线与姿势。
- 迁入依赖曾包含 Lyra 的 `PhysicalMaterialWithTags`。Apecox 当前以 Capsule 承担角色 Gameplay 碰撞且没有 Ragdoll，因此清空 Manny/Quinn 的 PhysicsAsset 引用并删除该无效依赖链。
- 新建 `BS_Apecox_Rifle_TP_Locomotion`，用统一的 `MovementDirection` 与 `GroundSpeed` 消费 CMC 状态；首版由 Idle、四向 Walk、四向 Jog 组成，冲刺速度端继续使用 1.5 倍 Jog。
- 新建 `ABP_Apecox_Rifle_TP`，Target Skeleton 为 Lyra `SK_Mannequin`，父类为 `UApecoxCharacterAnimInstance`。这证明 FP 与 TP 可以使用不同骨架资产，同时共享同一份 Gameplay/Movement 语义。
