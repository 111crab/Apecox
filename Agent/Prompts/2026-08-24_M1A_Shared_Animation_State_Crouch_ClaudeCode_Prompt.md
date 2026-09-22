# Claude Code 实施 Prompt：M1-A 共享动画状态基础与蹲伏输入

你是 Apecox 项目的具体实施子代理。当前 Claude Code 对话是新窗口，必须先理解职责与范围，再实施代码。

项目根目录：

```text
D:/UnrealProject/Apecox
```

## 你的职责

- 只实施本 Prompt 已批准的 C++ 改动。
- 为教学目的添加简洁中文注释：既解释代码做什么，也解释关键设计为什么这样分层。
- 完成后执行完整 Development Editor 构建并提交中文报告。
- 不操作 UE 编辑器资产，不使用 MCP，不执行 Git，不生成 Rider/VS 解决方案。
- 遇到范围外问题时写入报告并停止，不自行扩展架构。

## 开始前必须阅读

```text
Agent/README.md
Agent/00_Coordination/Current_Phase.md
Agent/00_Coordination/Current_Code_Design.md
Agent/00_Coordination/Subagent_Review_Checklist.md

Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h
Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Public/Weapons/ApecoxWeaponDefinition.h
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Public/Input/ApecoxInputConfig.h
Source/Apecox/Public/Input/ApecoxInputComponent.h
Source/Apecox/Apecox.Build.cs
```

先检查工作区现状，理解已有改动，不覆盖或回退用户与其他代理的修改。

## 已批准的架构边界

```text
Character / CharacterMovement / Equipment（玩法真相）
                         |
                         v
             UApecoxCharacterAnimInstance
                         |
             +-----------+-----------+
             |                       |
     FP Arms AnimBP             TP Manny AnimBP
```

- C++ AnimInstance 只读取和缓存状态，不选择动画资产，不编写状态机，不播放 Montage。
- CMC 是移动、跳跃、蹲伏的唯一状态来源。
- EquipmentComponent 的公开装备摘要是动画族来源，不新增第二份复制状态。
- 蹲伏使用 CMC 原生客户端预测，不新增 RPC、GA、GE 或 `State.Crouching` Tag。

## 一、新增动画族枚举

新增头文件：

```text
Source/Apecox/Public/Animation/ApecoxAnimationTypes.h
```

定义：

```cpp
UENUM(BlueprintType)
enum class EApecoxCharacterAnimationFamily : uint8
{
    Unarmed,
    Rifle
};
```

要求：

- 为枚举和两个值添加简洁中文注释或 `UMETA(DisplayName=...)`，使蓝图选择器可理解。
- 不增加尚未接入资产的 Pistol、Shotgun、Bow、Launcher 等占位枚举值。
- 这是表现层有限选择，不改成 GameplayTag。

## 二、扩展 Weapon Presentation Definition

修改：

```text
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
```

包含 `Animation/ApecoxAnimationTypes.h`，并在合适的 Animation 分类新增：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
EApecoxCharacterAnimationFamily EquippedAnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;
```

语义：武器声明装备后角色与 FP Arms 应使用的动画族。默认 `Unarmed` 是安全中性值；后续用户会在 `DA_WeaponPresentation_Rifle` 中手工设置为 `Rifle`。

不要把该字段放入 WeaponInstance、FireConfig、EquipmentComponent 或 GameplayAbility。

## 三、新增共享 AnimInstance

新增：

```text
Source/Apecox/Public/Animation/ApecoxCharacterAnimInstance.h
Source/Apecox/Private/Animation/ApecoxCharacterAnimInstance.cpp
```

类：

```cpp
UApecoxCharacterAnimInstance : UAnimInstance
```

### 3.1 生命周期覆写

```cpp
virtual void NativeInitializeAnimation() override;
virtual void NativeUpdateAnimation(float DeltaSeconds) override;
```

两者必须先调用对应的 `Super`。

`NativeInitializeAnimation()` 调用 `CacheCharacterReferences()`。

`NativeUpdateAnimation()`：

1. 如果缓存引用无效，或 `TryGetPawnOwner()` 已不是缓存角色，则重新缓存。
2. 没有合法 `OwningCharacter` 或 `CharacterMovement` 时调用 `ResetAnimationState()` 并返回。
3. 依次调用 `UpdateLocomotionState()`、`UpdateEquipmentState()`、`UpdateAimState()`。

不要在 `NativeThreadSafeUpdateAnimation()` 中访问 UObject；本轮全部 UObject 读取都在游戏线程 `NativeUpdateAnimation()` 完成。

### 3.2 缓存成员

```cpp
UPROPERTY(Transient)
TObjectPtr<AApecoxPlayerCharacter> OwningCharacter;

UPROPERTY(Transient)
TObjectPtr<UCharacterMovementComponent> CharacterMovement;
```

`CacheCharacterReferences()` 使用 `TryGetPawnOwner()` Cast 到 `AApecoxPlayerCharacter`，然后取得其 `GetCharacterMovement()`。编辑器预览没有合法 Pawn 时必须安全返回。

### 3.3 暴露给 AnimBP 的状态

以下全部使用 `UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|...")`，放在 `protected` 区域并给出合理默认值：

```cpp
float GroundSpeed = 0.0f;
float VerticalSpeed = 0.0f;
float MovementDirection = 0.0f;
bool bIsMoving = false;
bool bIsFalling = false;
bool bIsCrouching = false;
float AimPitch = 0.0f;
EApecoxCharacterAnimationFamily AnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;
```

建议分类：

```text
Apecox|Animation|Locomotion
Apecox|Animation|Aim
Apecox|Animation|Equipment
```

### 3.4 移动状态计算

`UpdateLocomotionState()`：

```text
Velocity          = CharacterMovement->Velocity
GroundSpeed       = Velocity.Size2D()
VerticalSpeed     = Velocity.Z
bIsMoving         = GroundSpeed > 3.0f
bIsFalling        = CharacterMovement->IsFalling()
bIsCrouching      = CharacterMovement->IsCrouching()
```

只有 `bIsMoving` 为 true 时计算 `MovementDirection`：

```cpp
MovementDirection = FRotator::NormalizeAxis(
    Velocity.ToOrientationRotator().Yaw - OwningCharacter->GetActorRotation().Yaw);
```

静止时将 `MovementDirection` 设回 `0.0f`，避免保留上一帧方向。

不要使用 `UKismetAnimationLibrary::CalculateDirection()`，避免为这一个计算增加 `AnimGraphRuntime` 模块依赖；本轮不修改 `Apecox.Build.cs`。

### 3.5 装备动画族计算

`UpdateEquipmentState()` 每帧先将 `AnimationFamily` 设为 `Unarmed`，然后按以下链路安全读取：

```text
OwningCharacter
  -> GetEquipmentComponent()
  -> GetEquippedWeaponDefinition()
  -> PresentationDefinition
  -> EquippedAnimationFamily
```

任一环节为空都保持 `Unarmed`。不要缓存或复制第二份装备真相，不修改 EquipmentComponent 接口。

### 3.6 瞄准俯仰计算

`UpdateAimState()` 使用：

```cpp
const FRotator AimDelta =
    (OwningCharacter->GetBaseAimRotation() - OwningCharacter->GetActorRotation()).GetNormalized();
AimPitch = AimDelta.Pitch;
```

本轮只暴露 `AimPitch`，不提前增加 AimYaw、TurnInPlace 或 AimOffset 资产逻辑。

### 3.7 重置

`ResetAnimationState()` 把所有暴露状态恢复到默认值：速度与角度为 `0`，布尔值为 `false`，动画族为 `Unarmed`。不要清空当前仍合法的 UObject 缓存；缓存失效由 `CacheCharacterReferences()` 负责。

## 四、新增蹲伏 Native Input Tag

修改：

```text
Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h
Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp
```

新增：

```text
C++ 变量：InputTag_Crouch
Tag 值：InputTag.Crouch
```

注释说明：这是稳定的蹲伏输入意图，不是 GameplayAbility 身份，也不是角色蹲伏状态。

不要新增 `State.Crouching`、`Movement.Crouching` 等状态 Tag。

## 五、Character 启用和绑定蹲伏

修改：

```text
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
```

### 5.1 构造配置

在现有 CMC 配置块中加入：

```cpp
CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
CMC->MaxWalkSpeedCrouched = 300.0f;
```

更新原注释中“本轮不新增 Crouch”的过期内容，解释使用内建 CMC 蹲伏是为了复用其客户端预测、服务器校正、胶囊高度和复制行为。

不要修改现有 `AirControl`、`BrakingDecelerationFalling`、角色旋转或摄像机参数。

### 5.2 输入函数

在与 Jump 相邻的位置声明并实现：

```cpp
void HandleCrouchStarted(const FInputActionValue& ActionValue);
void HandleCrouchCompleted(const FInputActionValue& ActionValue);
```

实现：

```text
HandleCrouchStarted   -> Crouch()
HandleCrouchCompleted -> UnCrouch()
```

### 5.3 输入绑定

在 `SetupPlayerInputComponent()` 的 Native 输入区域加入：

```text
InputTag_Crouch + ETriggerEvent::Started   -> HandleCrouchStarted
InputTag_Crouch + ETriggerEvent::Completed -> HandleCrouchCompleted
InputTag_Crouch + ETriggerEvent::Canceled  -> HandleCrouchCompleted
```

不要创建或硬引用 `IA_Crouch`；它由用户后续在 `DA_Apecox_InputConfig` 中手工配置。不要在 C++ 中绑定 Left Ctrl。

## 六、注释与代码质量要求

- 新增公共类、枚举、字段和关键函数使用中文注释解释职责与设计原因。
- 不写逐行翻译式注释。
- 新增 `.h` 只进入 `Source/Apecox/Public/...`，`.cpp` 只进入镜像的 `Source/Apecox/Private/...`。
- 所有 UObject 成员使用 `UPROPERTY + TObjectPtr`。
- 遵守 UE generated header 必须是本头文件最后一个 include 的规则。
- 不引入未使用 Include，不修改无关格式。

## 七、严格禁止

- 不创建、迁移、重命名或修改任何 `.uasset/.umap`。
- 不修改现有 `ABP_Apecox_Rifle_FP_Arms`、`ABP_Apecox_Manny` 或 Montage。
- 不实现 FP/TP 状态机、BlendSpace、AimOffset、Linked Anim Layer。
- 不实现 Sprint、Slide、ADS、Reload、切枪或脚步声。
- 不新增自定义 CMC、RPC、复制属性、GA、GE、GameplayCue 或 AbilityTask。
- 不修改射击、命中、弹药、武器实例、死亡和重生逻辑。
- 不修改 Build.cs、Config、项目文件或插件。
- 不执行 Git、MCP 或解决方案生成。

## 八、构建

执行完整构建：

```text
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

如果 UE/Live Coding 占用模块导致构建失败，不要结束用户进程；记录准确错误并停止。除此之外的编译错误必须在本轮范围内修复后重新构建。

## 九、中文报告

新增：

```text
Agent/Reports/2026-08-24_M1A_Shared_Animation_State_Crouch_Report.md
```

报告必须包含：

1. 实际新增和修改的文件。
2. 新增类、父类、枚举、成员、函数、Native Tag 的完整名称与作用。
3. AnimInstance 每个状态的来源和计算方式。
4. 蹲伏从 Native Input 到 CMC 的调用链，以及为什么没有 RPC。
5. 与 Lyra 思路的借鉴点，以及明确没有搬入的重型部分。
6. 构建命令、结果和关键输出。
7. 是否存在范围外改动或需要 Codex/用户确认的问题。

构建与报告完成后立即停止，等待 Codex 复审。不要继续 UE 编辑器配置或 Git 操作。
