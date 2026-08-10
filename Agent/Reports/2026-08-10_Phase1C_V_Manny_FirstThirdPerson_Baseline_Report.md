# Phase 1C-V Manny 第一/第三人称角色基线实施报告

**日期**：2026-08-10  
**状态**：等待 Codex 审查

---

## 一、修改文件列表

| 文件 | 修改内容 |
|------|----------|
| `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h` | 新增 3 个 Native GameplayTag 声明 |
| `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp` | 新增 3 个 Native GameplayTag 定义 |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | 新增 FirstPersonMesh/Camera 组件、Getter、输入函数声明、前向声明 |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 构造函数配置、SetupPlayerInputComponent 绑定、输入函数实现 |

---

## 二、新增 GameplayTag

| C++ 名称 | Tag 字符串 | 用途 |
|----------|-----------|------|
| `InputTag_Move` | `InputTag.Move` | 二维移动输入意图——不是 Ability 身份 |
| `InputTag_Look_Mouse` | `InputTag.Look.Mouse` | 鼠标二维观察输入意图——不是 Ability 身份 |
| `InputTag_Jump` | `InputTag.Jump` | 跳跃输入意图——不是 Ability 身份 |

Walk、Run、Falling 未建 GameplayTag——它们是 CMC/动画系统推导的派生状态，不是跨系统需要显式授予/移除的稳定语义。

---

## 三、新增组件与成员变量

| 名称 | 类型 | 属性 | 作用 |
|------|------|------|------|
| `FirstPersonMesh` | `TObjectPtr<USkeletalMeshComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 仅拥有者可见的第一人称 Manny 表现；`OnlyOwnerSee=true`；`FirstPerson` 基元类型；`NoCollision`；附着于 GetMesh() |
| `FirstPersonCamera` | `TObjectPtr<UCameraComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 附着于 FirstPersonMesh 的 `head` Socket；`bUsePawnControlRotation=true`；`bEnableFirstPersonFieldOfView=true`；`FirstPersonFieldOfView=70.0`；`bEnableFirstPersonScale=true`；`FirstPersonScale=0.6` |

---

## 四、新增 Getter

| 函数 | 返回类型 | 作用 |
|------|----------|------|
| `GetFirstPersonMesh()` | `USkeletalMeshComponent*` | 为后续第一人称武器附着和表现访问提供稳定入口 |
| `GetFirstPersonCamera()` | `UCameraComponent*` | 为后续瞄准射线、ADS 和镜头系统提供稳定入口 |

均为 `public`、内联实现、`const` 成员函数。

---

## 五、构造函数配置详情

### 5.1 胶囊体

- `InitCapsuleSize(34.0f, 96.0f)`：匹配 UE 5.8 第一人称模板的紧凑站立基线。

### 5.2 第三人称 Mesh (GetMesh)

- `SetOwnerNoSee(true)`：本地拥有者看不见第三人称全身，避免重叠。
- `FirstPersonPrimitiveType = WorldSpaceRepresentation`：按世界空间渲染，不受第一人称 FOV/Scale 影响。

### 5.3 第一人称 Mesh (FirstPersonMesh)

- `CreateDefaultSubobject<USkeletalMeshComponent>("FirstPersonMesh")`
- `SetupAttachment(ThirdPersonMesh)`：附着于第三人称 Mesh，共享骨架层级。
- `SetOnlyOwnerSee(true)`：仅拥有者可见。
- `FirstPersonPrimitiveType = FirstPerson`：第一人称渲染管线。
- `SetCollisionProfileName(NoCollision)`：不参与碰撞——碰撞由 Capsule 统一负责。

### 5.4 第一人称摄像机 (FirstPersonCamera)

- `CreateDefaultSubobject<UCameraComponent>("FirstPersonCamera")`
- `SetupAttachment(FirstPersonMesh, "head")`：附着于第一人称 Mesh 的 head Socket。
- `SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f))`：匹配 UE 5.8 模板摄像机偏移。
- `bUsePawnControlRotation = true`：摄像机跟随玩家鼠标/摇杆输入。
- `bEnableFirstPersonFieldOfView = true` + `FirstPersonFieldOfView = 70.0f`：第一人称 FOV 修正。
- `bEnableFirstPersonScale = true` + `FirstPersonScale = 0.6f`：第一人称视线遮挡缩放。

### 5.5 角色移动配置

- `bUseControllerRotationYaw = true`：角色 Yaw 跟随 Controller Yaw，因此 HandleMoveInput 的 Actor Forward/Right 与第一人称视角方向一致。
- `bOrientRotationToMovement = false`：禁止 CMC 自动根据速度方向旋转角色，避免与 Controller 视角旋转冲突。
- `BrakingDecelerationFalling = 1500.0f` + `AirControl = 0.5f`：UE 5.8 第一人称模板最小空中操控基线。

---

## 六、输入事件与函数对应表

| GameplayTag | 增强输入事件 | C++ 函数 | 行为 |
|-------------|-------------|----------|------|
| `InputTag.Move` | `Triggered` | `HandleMoveInput` | 读取 `FVector2D`，按 Controller Yaw 构建世界方向 → `AddMovementInput` |
| `InputTag.Look.Mouse` | `Triggered` | `HandleLookInput` | 读取 `FVector2D`，X→`AddControllerYawInput`，Y→`AddControllerPitchInput` |
| `InputTag.Jump` | `Started` | `HandleJumpStarted` | 调用 `Jump()` |
| `InputTag.Jump` | `Completed` | `HandleJumpCompleted` | 调用 `StopJumping()` |
| `InputTag.Jump` | `Canceled` | `HandleJumpCompleted` | 调用 `StopJumping()`——IA 被取消时释放跳跃，避免残留 |

所有绑定通过 `UApecoxInputComponent::BindNativeAction` 模板完成，移动和观察不经过 ASC/GAS 路由。

**重要设计决策**：

- **为什么不需要单独网络 RPC**：移动走 `AddMovementInput` → CMC 原生客户端预测和服务器校正；观察走 `AddControllerYawInput`/`AddControllerPitchInput` → Controller 旋转由引擎自行同步。没有新增任何复制变量或 RPC。
- **为什么不判断死亡状态**：死亡后 CMC 已被 `OnDeathStarted` 禁用，`AddMovementInput` 不会产生实际移动；`Jump()` 在禁用 CMC 后也是安全的空操作。
- **Yaw/Pitch 正负**：C++ 不擅自反转值，正负由用户在 IMC Modifier 中配置。

---

## 七、构建结果

**构建命令**：
```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

**结果**：✅ **Succeeded**（0 错误，0 新增警告）

唯一出现的警告是已有的 `C4996: 'EGameplayAbilityInstancingPolicy::NonInstanced'`，在 `ApecoxAbilitySystemComponent.cpp:213`，与本次修改无关。

---

## 八、UE 编辑器人工操作清单

本轮 C++ 不硬编码资产路径，以下配置需在 `BP_ApecoxPlayerCharacter` 蓝图子类或 InputConfig DataAsset 中完成：

1. **FirstPersonMesh 资产**：设置为 `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`，Anim Class 设为 `/Game/FirstPerson/Anims/ABP_FP_Copy`。
2. **第三人称 GetMesh() 资产**：保持现有 `SKM_Manny_Simple`，Anim Class 设为 `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed`。
3. **InputConfig DataAsset (DA_ApecoxInputConfig)**：在 `NativeInputActions` 数组中新增 3 条映射：
   - `InputTag.Move` → `IA_Move`（ValueType=Axis2D）
   - `InputTag.Look.Mouse` → `IA_Look`（ValueType=Axis2D）
   - `InputTag.Jump` → `IA_Jump`（ValueType=Digital）
4. **IMC (IMC_Default)**：为新 IA 绑定键位映射和 Modifier（Move: WASD/摇杆，Look: 鼠标/右摇杆，Jump: 空格/手柄A）。
5. **添加 UE 5.8 First Person 内容包**：确保 `ABP_FP_Copy` 和 `CtrlRig_FPWarp` 可用。
6. **GameMode**：确保 Level 使用 `BP_ApecoxGameMode` 或等效子类，Default Pawn Class 指向配置了上述资产的 `BP_ApecoxPlayerCharacter`。

---

## 九、未修改声明

以下系统和代码路径在本轮**未做任何修改**：

- ✅ `UApecoxHealthComponent` — 死亡监听和状态管理无变化
- ✅ `UApecoxVitalAttributeSet` — 属性和 OutOfHealth 广播无变化
- ✅ `UApecoxDeathAbility` — 死亡 GA 生命周期无变化
- ✅ `AApecoxGameMode` — 重生逻辑无变化
- ✅ `UApecoxAbilitySystemComponent` — ASC 输入缓存和 Ability 路由无变化
- ✅ `UApecoxInputComponent` / `UApecoxInputConfig` — 输入基础设施无变化
- ✅ `HandleAbilityInputTagPressed` / `HandleAbilityInputTagReleased` — Tactical Ability 输入行为不变
- ✅ `InitializeAbilitySystem` / `UninitializeAbilitySystem` — ASC 生命周期顺序不变
- ✅ 未新增移动 RPC、Transform 复制、GAS 移动 Ability
- ✅ 未新增 `UAnimInstance` 子类、未硬编码动画路径
- ✅ 未修改 `.uasset`、`.umap`、`.uproject`、Config 或 Git 状态

---

**实施完成，等待 Codex 审查。**
