# ClaudeCode Prompt - Character 冷启动基础类

生成日期：2026-07-14

## 你的身份与协作背景

你是 ClaudeCode，负责在明确 Prompt 下执行具体代码修改、编译验证，并输出中文实施报告。

Codex 是高层规划与审查者；用户负责最终决策、UE 编辑器人工操作和运行验证。

当前项目是新项目 `Apex`，不是旧项目 `Aura`。

项目路径：

```text
D:\UnrealProject\Apex
```

## 当前阶段目标

本次只做 Character 冷启动基础：新增长期使用的角色基础类和玩家角色类，让后续可以绑定 Paragon 英雄资产，支持基本移动输入和第三人称鼠标视角。

本次不实现技能系统，不实现 GAS，不实现 Montage 播放，不实现 AnimBP。

## 重要背景

- Apex 是用于重新构建长期可扩展 GAS 技能系统的新 UE 5.8 C++ 项目。
- 项目现有 UE 模板类 `AApexCharacter` / `AApexPlayerController` 等可以作为参考，但它们是模板代码，不是长期架构。
- 现有 `AApexCharacter` 把 Camera、Input、Move、Look 放在同一个类里。模板能跑，但长期我们要拆成 Base Character 与 Player Character。
- 本次新增我们的长期角色类，不删除、不重命名、不大改旧模板类。
- 学习包、Lyra、第三方示例项目不是本次主动参考源。除非编译或架构判断确实卡住，否则不要读取它们。
- 不使用 MCP 做单个蓝图、资产、GameMode、Mesh、AnimClass 操作。本次只写 C++ 和中文报告。

## 必须先读取

请先读取以下文件，理解当前项目事实：

```text
D:\UnrealProject\Apex\.agents\ue-project-context.md
D:\UnrealProject\Apex\Agent\Working_Agreement.md
D:\UnrealProject\Apex\Agent\Tooling_Guide.md
D:\UnrealProject\Apex\Source\Apex\ApexCharacter.h
D:\UnrealProject\Apex\Source\Apex\ApexCharacter.cpp
D:\UnrealProject\Apex\Source\Apex\ApexPlayerController.h
D:\UnrealProject\Apex\Source\Apex\ApexPlayerController.cpp
D:\UnrealProject\Apex\Source\Apex\Apex.Build.cs
```

## 本次允许新增的 C++ 文件

只允许新增以下 4 个 C++ 文件：

```text
D:\UnrealProject\Apex\Source\Apex\Character\ApexCharacterBase.h
D:\UnrealProject\Apex\Source\Apex\Character\ApexCharacterBase.cpp
D:\UnrealProject\Apex\Source\Apex\Character\ApexPlayerCharacter.h
D:\UnrealProject\Apex\Source\Apex\Character\ApexPlayerCharacter.cpp
```

同时允许新增一个中文报告文件：

```text
D:\UnrealProject\Apex\Agent\Reports\2026-07-14_Character_ColdStart_Report.md
```

除非编译必需，否则不要修改其他文件。若确实需要修改其他文件，必须在报告中说明原因和具体改动。

## 明确禁止

本次不要做以下事情：

- 不新增 AnimInstance 类。
- 不新增 Ability 类。
- 不新增 AttributeSet。
- 不新增 DataAsset。
- 不新增 GameplayTag。
- 不新增 GameplayEffect。
- 不新增 GameplayCue。
- 不播放 Montage。
- 不实现技能输入。
- 不修改蓝图资产。
- 不修改地图。
- 不修改 GameMode。
- 不删除或重命名旧模板类。
- 不主动读取学习包、Lyra 或第三方示例项目。
- 不使用 MCP 做单个 UE 编辑器可视操作。

## 类 1：AApexCharacterBase

文件：

```text
Source/Apex/Character/ApexCharacterBase.h
Source/Apex/Character/ApexCharacterBase.cpp
```

类名：

```cpp
AApexCharacterBase
```

父类：

```cpp
ACharacter
```

职责：

- 作为未来所有战斗角色的基础身体类。
- 负责基础 Capsule / Mesh / CharacterMovement 默认配置。
- 不包含玩家输入。
- 不包含 Camera / SpringArm。
- 不包含技能系统。
- 不包含 GAS。
- 不播放 Montage。
- 后续玩家、敌人、召唤物等都可以从它派生。

构造函数建议配置：

```cpp
GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

bUseControllerRotationPitch = false;
bUseControllerRotationYaw = false;
bUseControllerRotationRoll = false;

GetCharacterMovement()->bOrientRotationToMovement = true;
GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
GetCharacterMovement()->JumpZVelocity = 500.f;
GetCharacterMovement()->AirControl = 0.35f;
GetCharacterMovement()->MaxWalkSpeed = 500.f;
GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
```

不要开启 Tick，除非有非常明确的必要。本次通常不需要 Tick。

注释要求：

- 在类注释中用中文说明：为什么 Base 不放 Camera 和 Input。
- 在构造函数关键配置旁边写少量中文注释，说明这些是基础移动默认值，不是技能逻辑。

## 类 2：AApexPlayerCharacter

文件：

```text
Source/Apex/Character/ApexPlayerCharacter.h
Source/Apex/Character/ApexPlayerCharacter.cpp
```

类名：

```cpp
AApexPlayerCharacter
```

父类：

```cpp
AApexCharacterBase
```

职责：

- 玩家可控制角色。
- 拥有 SpringArm 和 Camera。
- 绑定 Enhanced Input 的 Move / Look / Jump。
- WASD 按控制器 yaw 对应的平面方向移动。
- 鼠标/摇杆 Look 控制 Controller Yaw/Pitch。
- 不处理技能输入。
- 不播放 Montage。
- 不添加 MappingContext。

注意：当前项目已有 `AApexPlayerController` 负责添加 MappingContext，所以 `AApexPlayerCharacter` 不要重复添加 MappingContext，避免重复输入上下文。

成员变量命名必须使用：

```cpp
CameraBoom
FollowCamera
JumpAction
MoveAction
LookAction
MouseLookAction
```

成员变量类型建议：

```cpp
TObjectPtr<USpringArmComponent> CameraBoom;
TObjectPtr<UCameraComponent> FollowCamera;
TObjectPtr<UInputAction> JumpAction;
TObjectPtr<UInputAction> MoveAction;
TObjectPtr<UInputAction> LookAction;
TObjectPtr<UInputAction> MouseLookAction;
```

UPROPERTY 建议：

- `CameraBoom` / `FollowCamera`：`VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true")`
- InputAction：`EditDefaultsOnly` 或 `EditAnywhere`, `BlueprintReadOnly`, `Category="Input", meta=(AllowPrivateAccess="true")`

函数命名尽量沿用现有模板，方便迁移理解：

```cpp
virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

void Move(const FInputActionValue& Value);
void Look(const FInputActionValue& Value);

UFUNCTION(BlueprintCallable, Category="Input")
virtual void DoMove(float Right, float Forward);

UFUNCTION(BlueprintCallable, Category="Input")
virtual void DoLook(float Yaw, float Pitch);

UFUNCTION(BlueprintCallable, Category="Input")
virtual void DoJumpStart();

UFUNCTION(BlueprintCallable, Category="Input")
virtual void DoJumpEnd();
```

输入绑定要求：

- `JumpAction` `Started` -> `DoJumpStart`
- `JumpAction` `Completed` -> `DoJumpEnd`
- `MoveAction` `Triggered` -> `Move`
- `LookAction` `Triggered` -> `Look`
- `MouseLookAction` `Triggered` -> `Look`

每次绑定前检查对应 Action 是否为空，避免蓝图未配置时崩溃。

如果 `UEnhancedInputComponent` 不存在，使用：

```cpp
UE_LOG(LogApex, Error, TEXT("..."));
```

不要新增日志类别，直接使用现有 `LogApex`。

移动逻辑：

- `Move` 的 `Value` 使用 `FVector2D`。
- X 作为 `Right`，Y 作为 `Forward`。
- `DoMove` 内使用 Controller 的 `YawRotation` 计算 `ForwardDirection` 和 `RightDirection`。
- 调用 `AddMovementInput`。
- 如果 Controller 为空则直接 return。

视角逻辑：

- `Look` 的 `Value` 使用 `FVector2D`。
- X 作为 `Yaw`，Y 作为 `Pitch`。
- `DoLook` 内调用 `AddControllerYawInput` 和 `AddControllerPitchInput`。
- 如果 Controller 为空则直接 return。

Camera 默认：

```cpp
CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
CameraBoom->SetupAttachment(RootComponent);
CameraBoom->TargetArmLength = 400.f;
CameraBoom->bUsePawnControlRotation = true;

FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
FollowCamera->bUsePawnControlRotation = false;
```

## 代码风格

- 参考现有 Apex 模板代码风格。
- 使用 `#pragma once`。
- 使用 `TObjectPtr` 作为 UPROPERTY UObject 成员。
- 不引入不必要的抽象。
- 不改 `Build.cs`，除非编译确实需要；如果改了，必须在报告里说明原因。
- 不新增 Tick。
- 需要中文注释解释关键设计原因，但不要把每行代码都注释成噪音。

## 编译验证

请尝试运行：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如果无法运行或环境问题，请说明原因。

## 报告要求

新建中文报告：

```text
D:\UnrealProject\Apex\Agent\Reports\2026-07-14_Character_ColdStart_Report.md
```

报告必须包含：

1. 修改文件列表。
2. 新增类说明。
3. 核心成员变量命名表。
4. 为什么 Base 不包含 Camera/Input。
5. 为什么 PlayerCharacter 不添加 MappingContext。
6. 编译结果。
7. 后续人工 UE 编辑器操作清单：
   - 创建 `BP_Hero_Phase`，父类选 `AApexPlayerCharacter`。
   - 设置 Mesh 为 Paragon Phase。
   - 设置 Anim Class，可先为空或使用 Paragon 自带 AnimBP。
   - 配置 InputAction 引用。
   - 在 GameMode 或关卡中指定默认 Pawn。
8. 未做事项：
   - 未实现 AnimBP。
   - 未播放 Montage。
   - 未接 GAS。
   - 未添加 GameplayTag。
   - 未改蓝图资产。

## 完成后回复

完成后请直接回复以下信息：

- 是否编译成功。
- 是否只新增了允许的 4 个 C++ 文件和 1 个报告文件。
- 是否有任何额外修改。
- 如果有失败，给出失败原因和人工处理清单。
