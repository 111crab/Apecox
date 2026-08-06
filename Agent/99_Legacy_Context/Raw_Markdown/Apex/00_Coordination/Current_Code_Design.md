# 当前代码设计：第三人称瞄准与朝向基线

更新日期：2026-07-30

## 一、阶段目标

在第二个技能开始前，统一解决三个基础问题：

1. 为本地玩家提供与瞄准射线一致的屏幕中心准星入口。
2. 将玩家角色切换为“水平朝向跟随鼠标视角”的第三人称扫射模型。
3. 明确技能使用视口中心瞄准，并让角色朝向、Montage 和投射物方向保持一致。

本阶段只搭建 C++ 入口和动画数据，不由子代理创建或修改 UMG、BlendSpace、AnimBP 等 UE 资产。

## 二、已确认方向

### 2.1 朝向模型

采用与 Lyra Shooter 角色相同的基础口径：

```cpp
bUseControllerRotationYaw = true;
Movement->bOrientRotationToMovement = false;
Movement->bUseControllerDesiredRotation = false;
```

设计含义：

- 鼠标修改 Controller ControlRotation。
- 玩家角色只跟随 ControlRotation 的水平 Yaw。
- 角色胶囊体不跟随 Pitch / Roll。
- WASD 继续按 ControlRotation.Yaw 计算世界移动方向。
- 前后左右移动变为面向准星的扫射移动。

该策略只在 `AApexPlayerCharacter` 设置，不修改通用 `AApexCharacterBase`，避免把玩家控制策略强加给其他 Character。

### 2.2 技能瞄准口径

保持当前已验证算法：

```text
视口中心反投影
-> 相机 Visibility Trace 得到 AimPoint
-> 手部 Socket 作为投射物生成位置
-> Socket 朝 AimPoint 构造飞行方向
```

准星是该规则的本地视觉表达，不参与服务器伤害判定。

角色已经持续跟随视角 Yaw，因此本阶段不新增 `EApexAbilityFacingPolicy`。以后出现冲刺锁向、蓄力跟踪或禁止转身等真实需求时，再增加技能级覆盖策略。

### 2.3 动画口径

玩家改为视角朝向后，现有只按 `GroundSpeed` 驱动的前进动画不够，需要增加相对角色朝向的移动角度：

```text
MovementDirection
0     = Forward
90    = Right
-90   = Left
180/-180 = Backward
```

Phase 已有可用资产：

```text
Idle
Jog_Fwd
Jog_Bwd
Jog_Left
Jog_Right
TurnLeft_90
TurnRight_90
TurnLeft_180
TurnRight_180
```

本阶段只让 C++ 暴露 `MovementDirection`。四向 BlendSpace 由用户在代码审查后手动配置；Turn In Place 和 Aim Offset 暂不实现。

## 三、C++ 修改设计

### 3.1 AApexPlayerController：本地 Gameplay HUD 生命周期

文件：

```text
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
```

新增成员：

```cpp
UPROPERTY(EditDefaultsOnly, Category="UI|HUD")
TSubclassOf<UUserWidget> GameplayHUDWidgetClass;

UPROPERTY(Transient)
TObjectPtr<UUserWidget> GameplayHUDWidget;
```

新增函数：

```cpp
void CreateGameplayHUD();
void RemoveGameplayHUD();
```

新增 Override：

```cpp
virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
```

生命周期：

```text
BeginPlay
-> 仅 IsLocalPlayerController()
-> GameplayHUDWidgetClass 有效且实例不存在
-> CreateWidget(this, GameplayHUDWidgetClass)
-> AddToPlayerScreen(0)

EndPlay
-> RemoveFromParent
-> 清空 GameplayHUDWidget
-> Super::EndPlay
```

约束：

- 不在 Character、GameMode 或 Level Blueprint 创建 HUD。
- 不影响现有 MobileControlsWidget。
- 未配置 GameplayHUDWidgetClass 时安全跳过，不报 Error。
- 不为静态准星提前创建 C++ Widget 子类。

### 3.2 AApexPlayerCharacter：玩家专属朝向策略

文件：

```text
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
```

在构造函数中覆盖基类旋转默认值：

```cpp
bUseControllerRotationYaw = true;

UCharacterMovementComponent* Movement = GetCharacterMovement();
Movement->bOrientRotationToMovement = false;
Movement->bUseControllerDesiredRotation = false;
```

保持：

```cpp
bUseControllerRotationPitch = false;
bUseControllerRotationRoll = false;
```

`DoMove()` 已按 Controller Yaw 计算方向，不修改输入算法。

### 3.3 UApexAnimInstance：增加 MovementDirection

文件：

```text
Source/Apex/Public/Animation/ApexAnimInstance.h
Source/Apex/Private/Animation/ApexAnimInstance.cpp
```

新增：

```cpp
UPROPERTY(Transient, BlueprintReadOnly, Category="Animation|Locomotion")
float MovementDirection = 0.f;
```

计算方式：

```text
World Planar Velocity
-> InverseTransformVectorNoScale 到角色局部空间
-> atan2(LocalY, LocalX)
-> 转为 [-180, 180] 度
```

当 `GroundSpeed` 接近零时设置为 `0.f`；`ResetLocomotionState()` 同样归零。

不引入 `UKismetAnimationLibrary` 或新的模块依赖。

### 3.4 TargetData Task：明确 ViewCenter 语义

现有名称过于宽泛：

```text
UApexAbilityTask_WaitAimTargetData
```

重命名为：

```text
UApexAbilityTask_WaitViewCenterTargetData
```

文件移动：

```text
Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
-> Public/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.h

Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
-> Private/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.cpp
```

公开命名：

```cpp
FApexViewCenterTargetDataReceivedDelegate
FApexViewCenterTargetDataCancelledDelegate

WaitViewCenterTargetData(...)
TargetDataReceived
TargetDataCancelled
ConfirmTargetData()
```

内部 Helper：

```cpp
SendTargetData()
CancelTargetData()
OnTargetDataReplicated(...)
OnTargetDataReplicatedCancelled()
```

`UApexProjectileCastAbility` 同步改名：

```cpp
ViewCenterTargetDataTask
OnViewCenterTargetDataReceived(...)
OnViewCenterTargetDataCancelled()
bViewCenterTargetDataReceived
CachedViewCenterTargetData
```

必须保持不变：

- 视口中心反投影算法。
- Visibility Trace。
- FScopedPredictionWindow 边界。
- TargetData 客户端发送与服务器消费。
- Montage GameplayEvent 与 TargetData 的双路径汇合。
- 服务器权威投射物生成。

已扫描 `/Game/Blueprints`，没有资产序列化引用旧 Task 类，因此本阶段不增加 CoreRedirect。

## 四、UE 资产设计

代码审查通过后由用户手动创建：

```text
/Game/Blueprints/GameFramework/BP_ApexGameMode
/Game/Blueprints/GameFramework/BP_ApexPlayerController
/Game/Blueprints/UI/HUD/WBP_Apex_Crosshair
/Game/Blueprints/UI/HUD/WBP_Apex_GameplayHUD
```

项目框架入口配置：

```text
/Game/Blueprints/GameFramework/BP_ApexGameMode
DefaultPawnClass = BP_Hero_Phase
PlayerControllerClass = BP_ApexPlayerController

/Game/Blueprints/GameFramework/BP_ApexPlayerController
DefaultMappingContexts[0] = IMC_Apex_BaseMove
GameplayHUDWidgetClass = WBP_Apex_GameplayHUD
```

`BP_ThirdPersonGameMode` 和 `BP_ThirdPersonPlayerController` 作为官方示例资产保留，但不再作为 Apex 正式运行入口。

更新：

```text
/Game/Blueprints/Characters/Phase/Animation/BS_Apex_Phase_Locomotion
```

BlendSpace 轴：

```text
Horizontal = MovementDirection，-180 到 180
Vertical   = GroundSpeed，0 到 500
```

## 五、本阶段不做

- 不新增 GameplayTag、GE、GA、SkillDefinition 字段或 AbilitySet。
- 不新增技能级 FacingPolicy。
- 不实现 Turn In Place、Aim Offset、Orientation Warping 或 IK。
- 不修改 Phase 第三方源资产。
- 不改变 Energy Bolt 的伤害、成本、冷却、Montage、Cue 或网络逻辑。
- 不使用 MCP 创建单个 UE 资产。

## 六、命名审阅表

| 类型 | 名称 | 作用 |
|---|---|---|
| Widget 资产 | `WBP_Apex_Crosshair` | 32x32 屏幕中心准星 |
| Widget 资产 | `WBP_Apex_GameplayHUD` | 本地 Gameplay HUD 根布局 |
| GameMode 资产 | `BP_ApexGameMode` | Apex 项目默认规则与默认 Pawn/Controller 配置入口 |
| PlayerController 资产 | `BP_ApexPlayerController` | Apex 本地输入和 Gameplay HUD 配置入口 |
| PC 配置 | `GameplayHUDWidgetClass` | HUD Widget 类配置 |
| PC 实例 | `GameplayHUDWidget` | 当前本地 HUD 实例 |
| PC 函数 | `CreateGameplayHUD` | 幂等创建本地 HUD |
| PC 函数 | `RemoveGameplayHUD` | 移除并清理 HUD |
| Anim 数据 | `MovementDirection` | 相对角色朝向的平面移动角度 |
| AbilityTask | `UApexAbilityTask_WaitViewCenterTargetData` | 生成并同步视口中心 TargetData |
| Task Factory | `WaitViewCenterTargetData` | 创建 Task |
| Task 函数 | `ConfirmTargetData` | 在技能执行时采样并发送中心瞄准数据 |

## 七、代码验证标准

- `ApexEditor Win64 Development` 编译成功。
- 没有新的 UHT、弃用或模块依赖警告。
- HUD 只在本地 PlayerController 创建且可安全销毁。
- 玩家角色朝向设置只落在 `AApexPlayerCharacter`。
- `MovementDirection` 在前后左右速度下分别接近 `0 / 180 / -90 / 90`。
- 旧 Task 文件、类名和 include 不再残留。
- TargetData 网络路径与 Energy Bolt 行为没有逻辑改写。

## 八、PIE 验证重点

- 准星始终位于窗口中心。
- 移动鼠标时，角色水平朝向跟随视角。
- W/S/A/D 分别播放前进、后退、左移、右移动画。
- Energy Bolt 从手部朝准星目标飞行，Montage 不再明显横着释放。
- Listen Server 与 Client 均能看到对方的正确朝向、Montage 和投射物。
