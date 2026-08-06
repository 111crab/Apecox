# Apex 第三人称瞄准与朝向基线实施报告

*日期：2026-07-30*

## 1. 修改文件

### 重命名（git mv）

| 旧路径 | 新路径 |
|--------|--------|
| `Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.h` | `Source/Apex/Public/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.h` |
| `Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp` | `Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitViewCenterTargetData.cpp` |

### 修改

| 文件 | 变更 |
|------|------|
| `Source/Apex/Public/Player/ApexPlayerController.h` | +GameplayHUDWidgetClass, GameplayHUDWidget, CreateGameplayHUD, RemoveGameplayHUD, EndPlay |
| `Source/Apex/Private/Player/ApexPlayerController.cpp` | HUD 生命周期实现 + BeginPlay 调用 CreateGameplayHUD |
| `Source/Apex/Private/Character/ApexPlayerCharacter.cpp` | 构造函数添加 bUseControllerRotationYaw=true + 扫射朝向设置 |
| `Source/Apex/Public/Animation/ApexAnimInstance.h` | +MovementDirection 属性 |
| `Source/Apex/Private/Animation/ApexAnimInstance.cpp` | NativeUpdateAnimation 计算 MovementDirection；ResetLocomotionState 归零 |
| `Source/Apex/Public/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.h` | Task→ViewCenter 全部重命名 |
| `Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp` | 同上 |

## 2. 新增/重命名成员

### AApexPlayerController

| 成员 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `GameplayHUDWidgetClass` | `TSubclassOf<UUserWidget>` | EditDefaultsOnly | HUD Widget 蓝图类 |
| `GameplayHUDWidget` | `TObjectPtr<UUserWidget>` | Transient | 运行时 HUD 实例 |
| `CreateGameplayHUD()` | void | — | IsLocalPC 时 CreateWidget + AddToPlayerScreen(0) |
| `RemoveGameplayHUD()` | void | — | RemoveFromParent + 置空 |
| `EndPlay()` | virtual void | override | RemoveGameplayHUD → Super |

### AApexPlayerCharacter

构造函数新增：
```cpp
bUseControllerRotationYaw = true;
Movement->bOrientRotationToMovement = false;
Movement->bUseControllerDesiredRotation = false;
```

### UApexAnimInstance

| 成员 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `MovementDirection` | float | Transient, BlueprintReadOnly | 角色局部空间扫射方向（度），[-180, 180] |

### TargetData 重命名对照

| 旧名 | 新名 |
|------|------|
| `UApexAbilityTask_WaitAimTargetData` | `UApexAbilityTask_WaitViewCenterTargetData` |
| `FApexAimTargetDataReceivedDelegate` | `FApexViewCenterTargetDataReceivedDelegate` |
| `FApexAimTargetDataCancelledDelegate` | `FApexViewCenterTargetDataCancelledDelegate` |
| `WaitAimTargetData` | `WaitViewCenterTargetData` |
| `AimDataReceived` | `TargetDataReceived` |
| `AimDataCancelled` | `TargetDataCancelled` |
| `ConfirmAimTarget` | `ConfirmTargetData` |
| `SendAimTargetData` | `SendTargetData` |
| `CancelAimTarget` | `CancelTargetData` |
| `AimTargetDataTask` | `ViewCenterTargetDataTask` |
| `OnAimTargetDataReceived` | `OnViewCenterTargetDataReceived` |
| `OnAimTargetDataCancelled` | `OnViewCenterTargetDataCancelled` |
| `bAimDataReceived` | `bViewCenterTargetDataReceived` |
| `CachedAimTargetData` | `CachedViewCenterTargetData` |

## 3. HUD 生命周期

```
BeginPlay → CreateGameplayHUD
  → IsLocalPlayerController? → GameplayHUDWidgetClass? → CreateWidget → AddToPlayerScreen(0)

EndPlay → RemoveGameplayHUD → RemoveFromParent → Super::EndPlay
```

## 4. 玩家朝向

玩家采用面向视角的扫射模型（水平朝向跟随 Controller 视角），通用 CharacterBase 仍保留面向移动方向。Pitch/Roll 不跟随 Controller。

## 5. MovementDirection 计算

```
GroundSpeed > 0 → LocalVelocity = InverseTransformVectorNoScale(Velocity)
                → MovementDirection = Atan2(LocalVelocity.Y, LocalVelocity.X) 转度
GroundSpeed ≈ 0 → MovementDirection = 0
```

范围 [-180, 180]，ResetLocomotionState 时归零。

## 6. 编译结果

```
Result: Succeeded
Total execution time: 37.47 seconds
Exit code: 0, 0 errors, 0 warnings
```

## 7. 用户后续 UE 资产操作

- 在 BP_ThirdPersonPlayerController 中配置 GameplayHUDWidgetClass 为 WBP_Apex_GameplayHUD
- 创建四向扫射 BlendSpace（Forward/Back/Left/Right）
- 在 AnimBP 中接入 MovementDirection
- Turn In Place / Aim Offset 暂不实现

## 8. 未完成/风险

- 四向 BlendSpace、Turn In Place、Aim Offset 均未实现，待后续阶段
- 旧名未在 Agent 文档中搜索替换——保留历史 Prompt/Report 不变
