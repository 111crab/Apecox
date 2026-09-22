# Lyra AI 移动与空手动画调查

更新日期：2026-09-17。参考工程：`D:\UnrealProject\LyraStarterGame`，目标工程：`D:\UnrealProject\Apecox`。

## 结论

Lyra 没有用“空手走路蒙太奇”驱动机器人持续移动。它把三个职责分开：AI Controller 与行为树决定目标；CharacterMovementComponent 执行路径移动；第三人称 AnimBP 根据角色的实际速度、加速度、落地状态和朝向持续选取 Locomotion 动画。Montage 只适合出生、开火、换弹等离散动作。

Apecox 不需要迁移 Lyra 的完整 AI、Experience、GameFeature 或整套 Linked Anim Layer。近期最稳妥的做法是保留现有随机漫游控制器，统一机器人实际使用的 Anim Class，并让状态机读取 Apecox 原生 AnimInstance 已提供的速度变量。Lyra 的空手动画可在这一闭环稳定后作为第二阶段表现升级，按需重定向少量 AnimSequence，而不是把它们当 Montage 播放。

## Lyra 的 AI 控制链

1. `ULyraBotCreationComponent` 在服务器上按配置生成 Bot Controller，经 GameMode 初始化并重生 Pawn。
2. `B_AI_Controller_LyraShooter` 的 `bStartAILogicOnPossess=false`。BeginPlay 先等待 `WaitForExperienceReady`，完成后才调用 `RunBehaviorTree`，避免 Pawn、Ability 和 Experience 尚未初始化时提前运行 AI。
3. 控制器引用 `/ShooterCore/Bot/BT/BT_Lyra_Shooter_Bot`。Blackboard 主要使用 `TargetEnemy`、`MoveGoal`、`OutOfAmmo`；行为树通过 AI Perception 与 EQS 找目标、找移动点，再以 `MoveTo` 执行移动。
4. `ALyraPlayerBotController` 本身主要处理 PlayerState、Team、Restart 和 ASC Avatar 生命周期；寻路意图在行为树里，不在该 C++ Controller 中硬编码。
5. Lyra Manny 的 CMC 基线包括 `MaxWalkSpeed=600`、`MaxAcceleration=1200`、`bRequestedMoveUseAcceleration=true`。AI 路径请求因此生成可供动画系统读取的加速度。

这套行为树适合完整对战 Bot。Apecox 当前只需要自由漫游靶标，现有 `AApecoxWanderAIController` 的 `GetRandomReachablePointInRadius → MoveToLocation` 已足够，不应为修复走路动画引入 Lyra 的感知、EQS、武器搜索和 Experience 依赖。

## Lyra 的动画链

Lyra Hero Mesh 使用：

- 主动画蓝图：`/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base`
- Linked Layer 接口：`/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ALI_ItemAnimLayers`
- 空手层：`/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/ABP_UnarmedAnimLayers`
- 空手 BlendSpace：`/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Unarmed/BS_MM_Unarmed_Jog_Walk`

`ABP_Mannequin_Base` 的 Locomotion 状态机包含 Idle、Start、Cycle、Stop、Pivot、JumpStart、JumpApex、Fall/Land 等状态，并通过 Linked Anim Layer 按装备族提供动作。空手层引用的是 `MM_Unarmed_Idle_Ready`、四向 Walk/Jog、Start/Stop/Pivot、Jump 和 Crouch 等 AnimSequence。`RootMotionMode=RootMotionFromMontagesOnly`，持续位移仍由 CMC 控制。

因此直接迁移一个所谓“AI 空手蒙太奇”不能复刻 Lyra。完整迁移还会带入 Anim Layer 接口、Control Rig、动画枚举、通知及大量动作依赖，范围远大于 Apecox 当前需求。

## Apecox 实际配置与此前未生效的原因

2026-09-17 通过 UE 5.8 Commandlet 读取资产 CDO，确认：

- `BP_ApecoxPlayerCharacter.CharacterMesh0` 使用 `ABP_Apecox_Manny`。
- `BP_ApecoxBotCharacter.CharacterMesh0` 实际使用 `/Game/ThirdParty/Epic/UE58_FirstPerson/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed`。
- 两者 Mesh 都是 `SKM_Manny_Simple`，目标骨架均为项目内 Epic UE5 Manny 骨架。
- `ABP_Apecox_Manny` 的父类已经是 `UApecoxCharacterAnimInstance`，但机器人并未引用它。因此此前修改该动画蓝图父类不会改变机器人的运行结果。
- 机器人实际使用的模板 `ABP_Unarmed` 将 `ShouldMove` 计算为 `GroundSpeed > 0.01 AND GetCurrentAcceleration() != (0,0,0)`。如果路径跟随直接写速度或加速度在某帧归零，角色可以继续位移，而状态机可能回到 Idle，视觉上就是滑行。
- `AApecoxBotCharacter` 当前已在构造和 BeginPlay 强制 `bRequestedMoveUseAcceleration=true`，CDO 也确认该值为 true。这能满足模板图的加速度门槛，但并未解决“修改了另一个 AnimBP”的配置错位，也仍让持续移动状态依赖瞬时加速度。
- `UApecoxCharacterAnimInstance` 已提供更适合 Apecox 的事实：`GroundSpeed=Velocity.Size2D()`、`bIsMoving=GroundSpeed>3`、`MovementDirection`、`bIsFalling`。这些值对玩家输入和 AI MoveTo 都成立。

调查证据保存在 `Saved/Diagnostics/M1D/LyraAI/lyra` 与 `Saved/Diagnostics/M1D/LyraAI/apecox`；两次 Commandlet 均成功退出，无资产修改。

## 推荐实施顺序

### 第一阶段：先恢复稳定漫游

1. 让 `BP_ApecoxBotCharacter.CharacterMesh0` 明确使用一个 Apecox 自有的第三人称 AnimBP。可清理并复用 `ABP_Apecox_Manny`，也可从它复制为 `ABP_Apecox_Bot_Unarmed`；不得继续同时维护 `ABP_Unarmed` 与 `ABP_Apecox_Manny` 两套互不一致的移动真相。
2. AnimGraph 只使用 `UApecoxCharacterAnimInstance` 的 `GroundSpeed`、`MovementDirection`、`bIsMoving`、`bIsFalling`，删除或停用模板 EventGraph 中重复的 `GroundSpeed_0`、`ShouldMove` 和加速度门槛。
3. 地面 Cycle 继续使用项目现有 `BS_Idle_Walk_Run`。它已经包含 Idle、八方向 Walk/Jog，且与当前 `SKM_Manny_Simple` 使用同一 Skeleton，无需先迁移 Lyra 资产。
4. 保留 `bRequestedMoveUseAcceleration=true`，用于平滑 AI 起停；动画是否进入移动状态以真实水平速度为准。
5. 将 Bot 的 `WalkSpeed` 与玩家当前地面常规速度统一为 400 cm/s，再重新开启 `bEnableWandering`，完成 NavMesh、起步、持续走、转向、停步、受伤、死亡与重生验证。

### 第二阶段：按需提升为 Lyra 风格

若基础闭环通过但观感仍不足，只迁移并重定向下列 Lyra AnimSequence：Idle、Walk/Jog 四向、Start、Stop、Pivot、Jump/Fall/Land。先把它们放入 Apecox 自有的 BlendSpace/状态机；不要引入 ShooterCore 行为树，也不要把持续移动改成 Montage。

完整的 `ABP_Mannequin_Base + ALI_ItemAnimLayers + ABP_UnarmedAnimLayers` 只有在项目准备支持多种第三人称装备动画族、转身、起停和 Pivot 时才值得迁移。当前机器人只是自由移动与射击靶标，这一成本没有必要。

