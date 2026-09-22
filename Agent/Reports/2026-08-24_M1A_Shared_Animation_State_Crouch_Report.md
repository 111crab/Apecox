# M1-A 共享动画状态基础与蹲伏输入实施报告

实施日期：2026-08-24  
构建结果：**编译全部通过，最终 DLL 链接因编辑器占用而失败**（详见第 6 节）  
状态：等待 Codex 复审。

## 1. 实际新增和修改的文件

| 文件 | 改动 |
| --- | --- |
| `Source/Apecox/Public/Animation/ApecoxAnimationTypes.h` | 新增（动画族枚举） |
| `Source/Apecox/Public/Animation/ApecoxCharacterAnimInstance.h` | 新增（共享 AnimInstance） |
| `Source/Apecox/Private/Animation/ApecoxCharacterAnimInstance.cpp` | 新增（实现） |
| `Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h` | 修改（新增 `EquippedAnimationFamily`） |
| `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h` | 修改（新增 `InputTag_Crouch` 声明） |
| `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp` | 修改（新增 `InputTag_Crouch` 定义） |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | 修改（新增蹲伏输入函数声明） |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 修改（CMC 蹲伏配置 + 输入函数 + 绑定） |
| `Agent/Reports/2026-08-24_M1A_Shared_Animation_State_Crouch_Report.md` | 新增本报告 |

未修改 `Apecox.Build.cs`、Config、`.uasset/.umap`、AnimBP、Montage、射击/命中/弹药/武器实例/死亡重生逻辑。

## 2. 新增类 / 枚举 / 成员 / 函数 / Native Tag

### 枚举

| 名称 | 值 | 作用 |
| --- | --- | --- |
| `EApecoxCharacterAnimationFamily`（`UENUM(BlueprintType)`） | `Unarmed`（空手）、`Rifle`（步枪） | 装备后角色与 FP Arms 应使用的动画族；表现层有限选择，不改成 GameplayTag |

### 新增类

| 名称 | 父类 | 文件 | 职责 |
| --- | --- | --- | --- |
| `UApecoxCharacterAnimInstance` | `UAnimInstance` | `Animation/ApecoxCharacterAnimInstance.h/.cpp` | 只读取并缓存角色状态，不选动画资产、不写状态机、不播 Montage；FP Arms 与 TP Manny AnimBP 共同继承 |

### 新增成员

- `UApecoxCharacterAnimInstance` 缓存：`TObjectPtr<AApecoxPlayerCharacter> OwningCharacter`、`TObjectPtr<UCharacterMovementComponent> CharacterMovement`（均 `Transient`）。
- 暴露状态（均 `Transient, BlueprintReadOnly`，分类 `Apecox|Animation|Locomotion/Aim/Equipment`）：`GroundSpeed`、`VerticalSpeed`、`MovementDirection`、`bIsMoving`、`bIsFalling`、`bIsCrouching`、`AimPitch`、`AnimationFamily`。
- `UApecoxWeaponPresentationDefinition`：`EApecoxCharacterAnimationFamily EquippedAnimationFamily = Unarmed`（`EditDefaultsOnly, BlueprintReadOnly, Category = "Animation"`）。

### 新增/修改函数

- `virtual void NativeInitializeAnimation() override;` —— 先 `Super`，再 `CacheCharacterReferences()`。
- `virtual void NativeUpdateAnimation(float DeltaSeconds) override;` —— 先 `Super`，再缓存校验、空态重置、依次 `UpdateLocomotionState/UpdateEquipmentState/UpdateAimState`。
- `void CacheCharacterReferences()`、`void ResetAnimationState()`、`void UpdateLocomotionState()`、`void UpdateEquipmentState()`、`void UpdateAimState()`（均 protected）。
- `AApecoxPlayerCharacter::HandleCrouchStarted/HandleCrouchCompleted`（`const FInputActionValue&`）。

### 新增 Native Tag

| C++ 变量 | Tag 值 | 作用 |
| --- | --- | --- |
| `InputTag_Crouch` | `InputTag.Crouch` | 稳定的蹲伏输入意图——不是 GameplayAbility 身份，也不是角色蹲伏状态 |

未新增 `State.Crouching`、`Movement.Crouching` 等状态 Tag。

## 3. AnimInstance 各状态来源与计算

| 状态 | 来源 | 计算 |
| --- | --- | --- |
| `GroundSpeed` | `CharacterMovement->Velocity` | `Velocity.Size2D()` |
| `VerticalSpeed` | 同上 | `Velocity.Z` |
| `bIsMoving` | 同上 | `GroundSpeed > 3.0f` |
| `bIsFalling` | CMC | `CharacterMovement->IsFalling()` |
| `bIsCrouching` | CMC | `CharacterMovement->IsCrouching()` |
| `MovementDirection` | Velocity 与 Actor Yaw | 仅 `bIsMoving` 时 `FRotator::NormalizeAxis(Velocity.ToOrientationRotator().Yaw - ActorRotation.Yaw)`；静止归 0 |
| `AnimationFamily` | EquipmentComponent 公开装备摘要 | 每帧先置 `Unarmed`，再经 `GetEquipmentComponent → GetEquippedWeaponDefinition → PresentationDefinition → EquippedAnimationFamily` 安全读取；任一环节为空保持 `Unarmed` |
| `AimPitch` | 基 AimRotation 与 ActorRotation | `(GetBaseAimRotation() - GetActorRotation()).GetNormalized().Pitch` |

未使用 `UKismetAnimationLibrary::CalculateDirection()`，未新增 `AnimGraphRuntime` 依赖；UObject 读取全部在游戏线程 `NativeUpdateAnimation()`，未使用 `NativeThreadSafeUpdateAnimation()`。

## 4. 蹲伏调用链（为什么没有 RPC）

```text
InputTag_Crouch (IA 由用户在 DA_Apecox_InputConfig 手工配置，C++ 不硬编码 Left Ctrl)
  -> BindNativeAction(Started/Completed/Canceled)
  -> AApecoxPlayerCharacter::HandleCrouchStarted/Completed
  -> Crouch() / UnCrouch()
  -> UCharacterMovementComponent 内建蹲伏
```

没有 RPC 的原因：`UCharacterMovementComponent::Crouch/UnCrouch` 走 CMC 原生客户端预测与服务器校正，胶囊高度、移动速度（`MaxWalkSpeedCrouched = 300`）、半高调整和复制全部由引擎内置，Character 只需转交输入意图，无需自定义 RPC、GA、GE 或状态 Tag。

## 5. 与 Lyra 思路的借鉴点与未搬入部分

**借鉴点**：把动画状态收敛为「从 Character/CMC/Equipment 读取玩法真相」的共享 AnimInstance 基类，而非在动画侧维护第二份状态；Native Input Tag 表达稳定的输入意图而非 Ability 身份；蹲伏复用 CMC 内建预测。

**明确未搬入的重型部分**：Lyra 的 Linked Anim Layer（Selection/Locomotion/Combining/Context 分层）、完整 Locomotion 状态机与 BlendSpace/AimOffset 资产、脚部 IK/Footstep 上下文、线程安全更新（`NativeThreadSafeUpdateAnimation`）、以及 `ALyraCharacter` 的大规模移动/相机/输入扩展。本轮只做最小状态缓存与单输入闭环。

## 6. 构建命令与结果

```text
命令：E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
      -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

结果：**编译全部通过（14 个 action，0 编译错误），最终 DLL 链接失败。**

- 新增/修改的 `ApecoxCharacterAnimInstance.cpp`、`ApecoxGameplayTags.cpp`、`ApecoxPlayerCharacter.cpp`、`ApecoxEquipmentComponent.cpp` 均编译成功，`.lib` 成功生成。
- 既有警告 1 个（与本次无关）：`ApecoxAbilitySystemComponent.cpp:228` 的 `NonInstanced` 弃用警告。
- 最终 `Link [x64] UnrealEditor-Apecox.dll` 失败：

```text
LNK1104: 无法打开文件 "D:\UnrealProject\Apecox\Binaries\Win64\UnrealEditor-Apecox.dll"
原因：The process cannot access the file because it is being used by another process. - UnrealEditor.exe
```

即 Unreal Editor 正在运行并锁定目标 DLL。按项目约定「编辑器运行导致 DLL 锁定时，只报告，不强制结束用户进程」，本轮未强制结束编辑器进程。关闭编辑器后重跑同一命令即可完成链接。

非代码提示：`UbaSessionServer - Killed process ... Low on memory` 为构建机内存压力导致的 UBA 反复重试（最终均编译成功），与代码无关。

## 7. 范围外改动与需确认问题

**范围外改动**：无。所有改动均在 Prompt 允许文件内。

**需 Codex/用户确认**：
- 枚举 `EApecoxCharacterAnimationFamily` 与共享 AnimInstance 的类名 `UApecoxCharacterAnimInstance`、成员命名已在 Prompt 中给出，与批准一致，无额外命名需确认。
- 后续 UE 手工步骤（由 Codex 复审通过后再定）：在 `DA_WeaponPresentation_Rifle` 中将 `EquippedAnimationFamily` 设为 `Rifle`；在 `DA_Apecox_InputConfig` 配置 `IA_Crouch`（C++ 不硬编码按键）；将 `ABP_Apecox_Rifle_FP_Arms` 与 `ABP_Apecox_Manny` 的父类改为 `UApecoxCharacterAnimInstance` 以读取共享状态。

---

*等待 Codex 复审。构建的 DLL 链接因编辑器占用未完成，不视为通过。*
