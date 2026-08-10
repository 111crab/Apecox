# ClaudeCode 执行提示：Phase 1C-V Manny 第一/第三人称角色基线

## 你的身份与协作边界

你是 Apecox 项目的代码实施子代理。Codex 负责架构规划和代码审查，用户负责审核命名、执行 UE 编辑器中的资产配置和最终运行验证。

本轮只修改 C++ 并执行构建，不操作 UE 编辑器、不调用 MCP、不创建或修改 `.uasset`、不执行 Git 暂存/提交/推送。遇到只能在 UE 编辑器完成的操作，写入中文实施报告中的“人工操作清单”，不要自行绕过。

项目：`D:/UnrealProject/Apecox`
引擎：`E:/UE_5.8`

新增业务 C++ 必须遵守：头文件放入 `Source/Apecox/Public/...`，实现放入镜像目录 `Source/Apecox/Private/...`。本轮原则上不新增 C++ 文件。

## 开始前必须阅读

1. `Agent/README.md`
2. `Agent/00_Coordination/Current_Phase.md`
3. `Agent/00_Coordination/Current_Code_Design.md`
4. `Agent/00_Coordination/Subagent_Review_Checklist.md`
5. 当前角色、输入和 Tag 源码：
   - `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
   - `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
   - `Source/Apecox/Public/Input/ApecoxInputComponent.h`
   - `Source/Apecox/Public/Input/ApecoxInputConfig.h`
   - `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h`
   - `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp`
6. UE 5.8 官方第一人称参考实现：
   - `E:/UE_5.8/Templates/TP_FirstPerson/Source/TP_FirstPerson/TP_FirstPersonCharacter.h`
   - `E:/UE_5.8/Templates/TP_FirstPerson/Source/TP_FirstPerson/TP_FirstPersonCharacter.cpp`

不要照搬官方模板的输入资产成员。Apecox 已经有 `UApecoxInputConfig`、`UApecoxInputComponent` 和 Native/Ability 输入分层，必须复用现有框架。

## 本轮目标

建立可用于后续死亡/重生验证的最小第一/第三人称角色：

- 本地拥有者看到第一人称 Manny 表现和摄像机。
- 其他玩家看到第三人称 Manny 全身。
- 两套表现共享同一个 `ACharacter`、Capsule、`UCharacterMovementComponent`、Controller 和网络移动状态。
- 支持移动、鼠标观察、按下/松开跳跃。
- 默认移动速度能驱动第三人称 Run 动画；第一人称姿态通过 UE 5.8 的 `ABP_FP_Copy` 从第三人称姿态复制并进行 First Person Warp，因此手臂会随 Idle/Run/Jump 姿态变化。
- 不改变已通过审查的 GAS、Health、Death、Respawn 生命周期。

## 批准的代码设计

### 1. Native GameplayTags

在现有 `ApecoxGameplayTags` 命名空间新增：

| C++ 名称 | Tag 字符串 | 用途 |
| --- | --- | --- |
| `InputTag_Move` | `InputTag.Move` | 二维移动输入 |
| `InputTag_Look_Mouse` | `InputTag.Look.Mouse` | 鼠标二维观察输入 |
| `InputTag_Jump` | `InputTag.Jump` | 跳跃输入 |

这些是稳定的玩家输入意图，不是 Ability 身份。不要为 Walk/Run/Falling 创建 GameplayTag；这些是 CMC/动画派生状态。

### 2. `AApecoxPlayerCharacter` 新增组件

新增前置声明并使用 `TObjectPtr`：

- `FirstPersonMesh : USkeletalMeshComponent`
  - `VisibleAnywhere, BlueprintReadOnly`
  - `OnlyOwnerSee = true`
  - `FirstPersonPrimitiveType = FirstPerson`
  - `NoCollision`
  - 按 UE 5.8 官方方案附着到 `GetMesh()`
- `FirstPersonCamera : UCameraComponent`
  - `VisibleAnywhere, BlueprintReadOnly`
  - 附着到 `FirstPersonMesh` 的 `head` Socket
  - 使用 Pawn Control Rotation
  - 使用 UE 5.8 官方 First Person FOV/Scale 支持和官方基线参数

现有 `GetMesh()`：

- `OwnerNoSee = true`
- `FirstPersonPrimitiveType = WorldSpaceRepresentation`

提供只读 Getter：

- `USkeletalMeshComponent* GetFirstPersonMesh() const`
- `UCameraComponent* GetFirstPersonCamera() const`

不要在 C++ 中硬引用 Manny、AnimBP 或路径；由用户在 `BP_ApecoxPlayerCharacter` 中配置。

### 3. 角色移动与朝向基线

构造函数中配置：

- Capsule 使用 UE 5.8 第一人称模板基线：Radius `34`、Half Height `96`。
- `bUseControllerRotationYaw = true`。
- `GetCharacterMovement()->bOrientRotationToMovement = false`。
- 保留 CMC 原生多人预测，不新增移动 RPC、不手工复制 Transform。
- 使用官方第一人称模板的最小空中操控基线：`BrakingDecelerationFalling = 1500`、`AirControl = 0.5`。
- 不新增 Sprint、Crouch、Slide、自定义 CMC 或 GAS 移动 Ability。

“Run”在本轮是默认 `MaxWalkSpeed` 下由 `ABP_Unarmed` 选择的移动动画，不是独立按键或玩法状态。除非当前源码已有明确覆盖，否则不要为了本轮额外修改 `MaxWalkSpeed`。

### 4. Native 输入函数

在 `AApecoxPlayerCharacter` 增加：

- `HandleMoveInput(const FInputActionValue& ActionValue)`
- `HandleLookInput(const FInputActionValue& ActionValue)`
- `HandleJumpStarted(const FInputActionValue& ActionValue)`
- `HandleJumpCompleted(const FInputActionValue& ActionValue)`

行为：

- Move：读取 `FVector2D`，按 Actor Forward/Right 调用 `AddMovementInput`。由于角色 Yaw 跟随 Controller，这与第一人称视角方向一致。
- Look：读取 `FVector2D`，调用 `AddControllerYawInput` / `AddControllerPitchInput`。不要在代码中擅自反转 Y；具体正负由用户在 IMC Modifier 中配置。
- Jump Started：调用 `Jump()`。
- Jump Completed：调用 `StopJumping()`。
- 死亡后 CMC 已由现有死亡流程 Disable，本轮不要复制一套死亡输入判断。

### 5. 输入绑定

在现有 `SetupPlayerInputComponent` 中，通过 `UApecoxInputComponent::BindNativeAction` 绑定：

- `InputTag.Move` + `Triggered` -> `HandleMoveInput`
- `InputTag.Look.Mouse` + `Triggered` -> `HandleLookInput`
- `InputTag.Jump` + `Started` -> `HandleJumpStarted`
- `InputTag.Jump` + `Completed` -> `HandleJumpCompleted`
- 为避免 Input Action 被取消时 Jump 持续，额外将 `Canceled` 绑定到 `HandleJumpCompleted`

保留当前 Ability 输入 `Started/Completed/Canceled` 绑定和 ASC 输入缓存逻辑，不要修改 Tactical Ability 行为。

## 动画和资产边界

本轮 C++ 不新增 `UAnimInstance` 子类，不实现动画状态机，不播放 Montage。

后续人工配置使用 UE 5.8 第一人称内容包中的：

- 第三人称 Mesh：`/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple`
- 第三人称 Anim Class：`/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed`
- 第一人称 Mesh：同一个 `SKM_Manny_Simple`
- 第一人称 Anim Class：`/Game/FirstPerson/Anims/ABP_FP_Copy`

`ABP_FP_Copy` 负责从第三人称 Mesh 复制姿态并通过 `CtrlRig_FPWarp` 修正第一人称显示，所以本轮不手写“手臂摆动”。

## 明确禁止

- 不新增第一/第三人称两套移动逻辑。
- 不复制或迁移 UE 模板 C++ 类。
- 不新增 `UApecoxAnimInstance`。
- 不实现武器、开火、换弹、ADS、后坐力、准星、Sprint、Crouch、Slide。
- 不改 HealthComponent、VitalAttributeSet、DeathAbility、GameMode 重生规则。
- 不修改 `.uasset`、`.umap`、`.uproject`、Config 或 Git 状态。
- 不调用 MCP。

## 注释要求

代码注释使用中文，重点解释：

- 为什么第一/第三人称是两套表现、同一套移动模拟。
- 为什么本地/远端可见性由 `OnlyOwnerSee` 和 `OwnerNoSee` 控制。
- 为什么第一人称输入不需要单独网络 RPC，而是沿用 CMC 预测。
- 为什么动画资源不硬编码进 C++。

避免逐行翻译式注释。

## 构建与自检

使用：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

构建前后检查：

1. 原有 Tactical Ability 输入代码未改变语义。
2. 原有 ASC 初始化/反初始化、死亡、重生代码未改变。
3. 第一人称组件只影响视觉和摄像机。
4. 没有移动 RPC 或 Tick 中手工同步 Transform。
5. Public/Private 目录规则仍满足。

## 中文实施报告

写入：

`Agent/Reports/2026-08-10_Phase1C_V_Manny_FirstThirdPerson_Baseline_Report.md`

必须列出：

1. 修改文件。
2. 新增的 GameplayTag 名称和用途。
3. 新增组件、成员变量、Getter、函数的完整名称和作用。
4. 构造函数中的可见性、摄像机和 CMC 配置。
5. 输入事件与函数对应表。
6. 构建命令、构建结果和关键输出。
7. UE 编辑器仍需人工完成的资产操作，仅列必须项。
8. 明确声明未修改的 GAS/死亡/重生边界。

完成报告后停止，等待 Codex 审查。
