# Apex 第三人称瞄准与朝向基线 ClaudeCode Prompt

## 一、身份与目标

你是 Apex 项目的 C++ 实施子代理。项目根目录：

```text
D:\UnrealProject\Apex
```

本次在第二个技能开始前，完成以下四项 C++ 基线：

1. `AApexPlayerController` 提供本地 Gameplay HUD Widget 生命周期入口。
2. `AApexPlayerCharacter` 水平朝向持续跟随 Controller / 鼠标视角。
3. `UApexAnimInstance` 暴露四向扫射 BlendSpace 所需的 `MovementDirection`。
4. 将泛化过度的 `WaitAimTargetData` 重命名为明确的 `WaitViewCenterTargetData`，保持全部 GAS 网络逻辑不变。

不要创建、修改或保存任何 UE 资产。UMG、PlayerController 蓝图配置、BlendSpace 和 AnimBP 由用户在 Codex 审查通过后手动完成。

## 二、必须先读

```text
Agent/00_Coordination/Current_Code_Design.md
Agent/00_Coordination/Current_Phase.md
Agent/00_Coordination/Subagent_Review_Checklist.md
```

重点参考当前源码：

```text
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
Source/Apex/Public/Character/ApexPlayerCharacter.h
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Public/Animation/ApexAnimInstance.h
Source/Apex/Private/Animation/ApexAnimInstance.cpp
Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.h
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
```

可按需要使用已有 UE 技能说明：

```text
ue-ui-umg-slate
ue-character-movement
ue-animation-system
ue-gameplay-abilities
```

不要主动使用 MCP。

## 三、允许修改的文件

允许修改：

```text
Source/Apex/Public/Player/ApexPlayerController.h
Source/Apex/Private/Player/ApexPlayerController.cpp
Source/Apex/Private/Character/ApexPlayerCharacter.cpp
Source/Apex/Public/Animation/ApexAnimInstance.h
Source/Apex/Private/Animation/ApexAnimInstance.cpp
Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.h
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
```

允许使用 `git mv` 重命名：

```text
Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h
-> Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.h

Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
-> Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.cpp
```

允许新建完成报告：

```text
Agent/Reports/2026-07-30_Apex_ThirdPerson_Aiming_Baseline_Report.md
```

除非编译明确证明必要，不修改其他文件。

## 四、实施要求

### 4.1 PlayerController HUD 生命周期

在 `AApexPlayerController` 中新增：

```cpp
UPROPERTY(EditDefaultsOnly, Category="UI|HUD")
TSubclassOf<UUserWidget> GameplayHUDWidgetClass;

UPROPERTY(Transient)
TObjectPtr<UUserWidget> GameplayHUDWidget;
```

新增：

```cpp
void CreateGameplayHUD();
void RemoveGameplayHUD();
virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
```

要求：

- `BeginPlay()` 调用 `CreateGameplayHUD()`。
- 只在 `IsLocalPlayerController()` 时创建。
- Class 为空或实例已存在时安全跳过。
- 使用 `CreateWidget<UUserWidget>(this, GameplayHUDWidgetClass)`。
- 使用 `AddToPlayerScreen(0)`，不要使用 Level Blueprint。
- `EndPlay()` 中先 `RemoveGameplayHUD()`，再 `Super::EndPlay()`。
- `RemoveGameplayHUD()` 调用 `RemoveFromParent()` 并置空引用。
- 不影响现有 `MobileControlsWidget` 和输入初始化。
- Class 未配置时不输出 Error；必要时使用 Verbose 或不记录。

### 4.2 玩家角色朝向

在 `AApexPlayerCharacter::AApexPlayerCharacter()` 中覆盖玩家专属设置：

```cpp
bUseControllerRotationYaw = true;

UCharacterMovementComponent* Movement = GetCharacterMovement();
Movement->bOrientRotationToMovement = false;
Movement->bUseControllerDesiredRotation = false;
```

保持 Pitch/Roll 不跟随 Controller。

不要修改：

- `AApexCharacterBase`。
- `DoMove()` 与 `DoLook()` 的输入算法。
- CharacterMovement 的速度、加速度和网络参数。

添加简短中文注释说明：玩家采用面向视角的扫射模型；通用 CharacterBase 仍保留面向移动方向的默认值。

### 4.3 AnimInstance MovementDirection

在 `UApexAnimInstance` 中新增：

```cpp
UPROPERTY(Transient, BlueprintReadOnly, Category="Animation|Locomotion")
float MovementDirection = 0.f;
```

在 `NativeUpdateAnimation()` 中：

1. 使用已有 `Velocity`。
2. 取平面速度。
3. 使用 `OwningCharacter->GetActorTransform().InverseTransformVectorNoScale(...)` 转为角色局部速度。
4. 使用 `FMath::Atan2(LocalVelocity.Y, LocalVelocity.X)` 并转为角度。
5. 输出范围保持在 `[-180, 180]`。
6. 当 `GroundSpeed` 接近零时，设为 `0.f`。

在 `ResetLocomotionState()` 中归零。

不要引入 `UKismetAnimationLibrary`、`AnimGraphRuntime` 或其他新模块。

### 4.4 ViewCenter TargetData Task 重命名

使用 `git mv` 移动两个文件，并同步：

```text
UApexAbilityTask_WaitAimTargetData
-> UApexAbilityTask_WaitViewCenterTargetData

FApexAimTargetDataReceivedDelegate
-> FApexViewCenterTargetDataReceivedDelegate

FApexAimTargetDataCancelledDelegate
-> FApexViewCenterTargetDataCancelledDelegate

WaitAimTargetData
-> WaitViewCenterTargetData

AimDataReceived
-> TargetDataReceived

AimDataCancelled
-> TargetDataCancelled

ConfirmAimTarget
-> ConfirmTargetData

SendAimTargetData
-> SendTargetData

CancelAimTarget
-> CancelTargetData
```

更新 `.generated.h`、include、返回类型、构造调用和 Delegate 绑定。

`UApexProjectileCastAbility` 同步重命名：

```text
AimTargetDataTask
-> ViewCenterTargetDataTask

OnAimTargetDataReceived
-> OnViewCenterTargetDataReceived

OnAimTargetDataCancelled
-> OnViewCenterTargetDataCancelled

bAimDataReceived
-> bViewCenterTargetDataReceived

CachedAimTargetData
-> CachedViewCenterTargetData
```

必须保持原逻辑逐项等价：

- 本地端在 GameplayEvent 到达时采样视口中心。
- 非本地端等待复制 TargetData。
- Viewport、Avatar、World、PlayerController 与 ASC 校验顺序不变。
- `FScopedPredictionWindow` 仍在所有前置依赖校验之后创建。
- `CallServerSetReplicatedTargetData` 与取消路径不变。
- 服务端 Delegate 消费 TargetData 的时机不变。
- ProjectileCast 的 Montage Event 与 TargetData 双路径汇合条件不变。

不要新增 CoreRedirect：项目自有资产扫描未发现旧 Task 类引用。

## 五、代码质量要求

- 保持 Public/Private 目录划分。
- 保留 UTF-8 中文注释。
- 修正触及代码附近已有的明显缩进问题，但不做无关格式化。
- 公开类、成员和函数使用本文确认的命名。
- 不添加 Tick、全局单例、GameplayTag、DataAsset 字段或 Ability Policy。
- 不修改 Energy Bolt 数值和 UE 资产。
- 不生成 `.sln`。
- 不执行 Git commit 或 push。

## 六、验证

### 6.1 静态搜索

确认源码中不再残留：

```text
ApexAbilityTask_WaitAimTargetData
WaitAimTargetData
AimTargetDataTask
bAimDataReceived
CachedAimTargetData
```

允许历史 Agent 文档保留旧名称，不要为了搜索结果修改历史 Prompt/Report。

### 6.2 编译

引擎：

```text
E:\UE_5.8
```

目标：

```text
ApexEditor Win64 Development
```

若 UE Editor / Live Coding 阻止外部构建，只记录原因，不终止用户进程。

## 七、完成报告

新建：

```text
Agent/Reports/2026-07-30_Apex_ThirdPerson_Aiming_Baseline_Report.md
```

报告必须使用中文，包含：

1. 实际修改和重命名文件。
2. 新增/重命名的类、成员、函数及其作用。
3. HUD 生命周期说明。
4. 玩家朝向设置及其作用范围。
5. MovementDirection 计算口径。
6. TargetData 重命名前后对照及“逻辑未改变”的核对结果。
7. 编译命令与结果。
8. 用户后续必须完成的 UE 资产操作。
9. 未完成事项和风险：四向 BlendSpace、Turn In Place、Aim Offset。

完成后停止，等待 Codex 审查。不要声称已经完成 PIE。
