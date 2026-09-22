# Lyra 水平瞄准与原地转身调查

更新日期：2026-09-19。

## 结论

Apecox 可以在不改变第一人称操控、角色胶囊朝向、GAS、射击方向和复制协议的前提下，加入第三人称水平瞄准与原地转身。应复用 Lyra 的算法语义和四条 Rifle 90 度转身序列，不迁入 `ABP_Mannequin_Base`、`ABP_ItemAnimLayersBase` 或 `ABP_RifleAnimLayers` 整套运行时。

这项能力是第三人称表现增强，不是玩法旋转系统。角色 Actor 继续立即跟随 Controller Yaw；动画层用 `RootYawOffset` 抵消静止时 Actor 的转角，让脚暂时留在原位，再以 `AimYaw=-RootYawOffset` 让上半身和步枪继续指向玩家瞄准方向。偏移达到阈值后播放转身动画，并用动画曲线逐帧消耗偏移。

## Lyra 的真实实现

### 角色旋转边界

`ALyraCharacter` 的原生默认值是：

- `bUseControllerRotationYaw=true`
- `bUseControllerDesiredRotation=false`
- `bOrientRotationToMovement=false`
- `RotationRate.Yaw=720`

因此 Lyra 没有让胶囊延迟跟随视角。脚不滑和原地转身来自动画补偿，不来自另一套 CharacterMovement 旋转模式。

### Root Yaw Offset

`ABP_Mannequin_Base` 在静止时把每帧 Actor Yaw 变化的反值累加到 `RootYawOffset`，并通过 `Rotate Root Bone` 抵消 Mesh 随 Actor 产生的立即旋转。该蓝图的默认值为：

- 站立限制：`-120..100`
- 蹲伏限制：`-90..80`
- `bEnableRootYawOffset=true`
- 移动等不再需要保持脚部朝向的状态使用弹簧回到 0：Stiffness `80`、Critical Damping `1`、Mass `1`、Target Velocity Amount `0.5`

`SetRootYawOffset` 同时令 `AimYaw=-RootYawOffset`。这一步很关键：Apecox 当前角色已经随控制器 Yaw 旋转，所以 `(BaseAimRotation-ActorRotation).Yaw` 通常接近 0，不能直接作为水平 Aim Offset。水平 Aim Offset 必须由动画视觉根偏移反推。

Lyra 的主图顺序是基础 Locomotion/Inertialization 后执行 `Rotate Root Bone`，随后进入 `FullBody_Aiming`，最后再做手部约束。Apecox 应保持同样的语义：根部补偿位于 Aim Offset 之前，左手 IK 仍位于 Aim Offset 之后。

### Turn-in-place

Lyra 的 `Idle -> TurnInPlaceRotation` 条件为：

`Abs(RootYawOffset) > 50.0`

Rifle 层实际选择以下四条序列：

- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_TurnLeft_90`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_TurnRight_90`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnLeft_90`
- `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnRight_90`

站立左右时长约 `1.667/1.700` 秒，蹲伏左右均约 `1.833` 秒。四条资产均启用 Root Motion、Force Root Lock，并包含：

- `RemainingTurnYaw`：从 `-90` 或 `90` 平滑归零，描述动画尚未完成的转角。
- `TurnYawWeight`：标记有效转身区段。

Lyra 不让这些资产驱动 Character 胶囊。`ProcessTurnYawCurve` 读取两条曲线，用本帧与上一帧的 `RemainingTurnYaw` 差值逐步减少 `RootYawOffset`，从而让动画中的脚步转动和视觉根偏移同步交接。转身曲线结束后进入短暂 Recovery，再回 Idle。

Lyra 内容中也有 180 度版本，但 `ABP_RifleAnimLayers` 的正常 Rifle Turn-in-place 默认没有选用它们。Apecox 首版不迁入 180 度版本。

## Apecox 当前图的兼容位置

已通过 UE 5.8 冷加载导出当前 `/Game/Blueprints/Characters/Animations/ThirdPerson/ABP_Apecox_Rifle_TP`，确认现行结构为：

`SM_Rifle_Locomotion -> UpperBody Reload 分层 -> FullBodyAdditivePreAim Fire Slot -> RifleLocomotion Cached Pose -> 站立/蹲伏 Aim Offset -> Inertialization -> Hand IK Retargeting -> Copy Bone -> Two Bone IK -> Output`

实施时应：

1. 在 `FullBodyAdditivePreAim` 与 `Save Cached Pose (RifleLocomotion)` 之间加入 `Rotate Root Bone`，Yaw 接 `RootYawOffset`。
2. 两个现有 Aim Offset 的横轴都从固定 `0.0` 改接 `AimYaw`；Pitch 保持接 `AimPitch`。
3. Turn-in-place 动作进入现有 `SM_Rifle_Locomotion` 的 Idle/Grounded 分支，不放进 Montage Slot，也不改变换弹、开火和左手 IK 的既有层级。
4. Turn 序列只负责表现，禁止用 Root Motion 推动胶囊；Actor、Projectile 和第三人称镭射的玩法方向仍由现有系统负责。

把 `Rotate Root Bone` 放在最终 IK 之后会整体旋转已经求解的手部约束，容易再次破坏左手贴枪；把它放在 Aim Offset 之后则会重复扭转瞄准姿势。上述位置能保持现有动作层的职责边界。

## 推荐实施顺序

### 验证门 A：水平瞄准与脚部保持

- 原生 AnimInstance 增加 `RootYawOffset`、`AimYaw`、上一帧 Actor Yaw 和内部弹簧状态。
- 静止、接地时累加视觉偏移；移动、腾空或初始化时平滑回零。
- AnimGraph 加入 `Rotate Root Bone`，两个 Aim Offset 接入 `AimYaw`。
- 双端验证远端角色小幅左右转向时脚不立即滑动，枪口、左手和镭射仍正确；移动、跳跃、蹲伏、开火、换弹无回归。

### 验证门 B：90 度原地转身

- 通过 Unreal 原生 Migrate 迁入上述四条 90 度序列及最小依赖，并清除 LyraGame 专用 Notify/Modifier 元数据。
- 在现有 Locomotion 状态机加入 Turn-in-place 表现；阈值首版采用 Lyra 的 `50` 度。
- 原生 AnimInstance 消费 `TurnYawWeight` 与 `RemainingTurnYaw`，移动或腾空时可立即中断转身并释放视觉偏移。
- 双端验证左右快速反向、站立/蹲伏转身、转身中开始移动，以及开火、换弹和镭射回归。

两道门应分别验收。门 A 只改变连续姿势，容易判断根部补偿符号和节点层级是否正确；门 B 再引入序列与曲线状态，出现问题时不会与 Aim Offset 混在一起排查。

## 风险边界

- 不复制 `RootYawOffset`、`AimYaw` 或逐帧曲线值。每台观察端从已经复制并平滑的 Actor Rotation 本地计算表现值，避免增加网络高频状态。
- 初始化、重生、AnimInstance 重新绑定时，上一帧 Yaw 必须先以当前 Actor Yaw 建基线，否则首帧可能产生大角度跳变。
- 转身期间继续转动鼠标时，必须同时处理新的 Actor Yaw 增量和动画曲线消耗，不能在动作开始时冻结瞄准方向。
- 当前第三人称趴姿没有配套资产；趴姿继续维持既有搁置决定，不复用蹲伏转身冒充趴姿。

## 调查证据

- Lyra 与 Apecox 冷加载导出、序列曲线和日志：`Saved/Diagnostics/ThirdPersonTurnInPlace`
- Lyra 完整蓝图导出：`Saved/Diagnostics/ThirdPersonPhase1/LyraAnimBlueprintExports`
- Apecox 当前 AnimBP 导出：`Saved/Diagnostics/ThirdPersonTurnInPlace/ApecoxCurrent/ABP_Apecox_Rifle_TP.t3d`

