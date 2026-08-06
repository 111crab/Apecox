# Character 冷启动基础类 — 实施报告

生成日期：2026-07-14

## 1. 修改文件列表

| 文件 | 操作 | 说明 |
|------|------|------|
| `Source/Apex/Character/ApexCharacterBase.h` | 新增 | 所有战斗角色基础类头文件 |
| `Source/Apex/Character/ApexCharacterBase.cpp` | 新增 | 构造函数 — Capsule/Mesh/CharacterMovement 默认配置 |
| `Source/Apex/Character/ApexPlayerCharacter.h` | 新增 | 玩家可控制角色头文件 |
| `Source/Apex/Character/ApexPlayerCharacter.cpp` | 新增 | Camera/SpringArm 创建 + Enhanced Input 绑定 + 移动/视角逻辑 |
| `Source/Apex/Apex.Build.cs` | **未修改** | 现有 EnhancedInput、InputCore、Engine 依赖已满足编译需求 |
| 旧模板类 (`AApexCharacter` / `AApexPlayerController`) | **未修改** | 保持原样不动 |
| `Agent/Reports/2026-07-14_Character_ColdStart_Report.md` | 新增 | 本报告 |

**结论：只新增了允许的 4 个 C++ 文件和 1 个报告文件，无额外修改。**

## 2. 新增类说明

### 2.1 AApexCharacterBase

- **父类：** `ACharacter`
- **职责：** 作为未来所有战斗角色（玩家、敌人、召唤物等）的基础身体类。
- **包含：** Capsule 尺寸 (42×96)、角色旋转模式、CharacterMovement 默认值（速度、跳跃、空中控制、刹车等）。
- **不包含：** Camera、SpringArm、输入、GAS、技能系统、Montage。
- **Tick：** 关闭（`bCanEverTick = false`）。

### 2.2 AApexPlayerCharacter

- **父类：** `AApexCharacterBase`
- **职责：** 玩家可控制角色。
- **包含：**
  - `CameraBoom` (USpringArmComponent)：相机摇臂，TargetArmLength=400，跟随 Controller 旋转。
  - `FollowCamera` (UCameraComponent)：第三人称跟随相机。
  - 4 个 InputAction 引用：`JumpAction`、`MoveAction`、`LookAction`、`MouseLookAction`。
  - `SetupPlayerInputComponent`：将 InputAction 绑定到处理函数（绑定前检查 Action 是否为空）。
  - `Move` / `Look`：从 `FInputActionValue` 提取 `FVector2D`。
  - `DoMove`：基于 Controller Yaw 计算世界空间方向，调用 `AddMovementInput`。
  - `DoLook`：调用 `AddControllerYawInput` / `AddControllerPitchInput`。
  - `DoJumpStart` / `DoJumpEnd`：调用 `Jump()` / `StopJumping()`。
- **不包含：** MappingContext 添加（由 `AApexPlayerController` 负责）、技能输入、GAS、Montage。
- **Tick：** 关闭（`bCanEverTick = false`）。

## 3. 核心成员变量命名表

| 变量名 | 类型 | 所属类 | UPROPERTY 属性 |
|--------|------|--------|---------------|
| `CameraBoom` | `TObjectPtr<USpringArmComponent>` | AApexPlayerCharacter | `VisibleAnywhere, BlueprintReadOnly, Category="Components"` |
| `FollowCamera` | `TObjectPtr<UCameraComponent>` | AApexPlayerCharacter | `VisibleAnywhere, BlueprintReadOnly, Category="Components"` |
| `JumpAction` | `TObjectPtr<UInputAction>` | AApexPlayerCharacter | `EditDefaultsOnly, BlueprintReadOnly, Category="Input"` |
| `MoveAction` | `TObjectPtr<UInputAction>` | AApexPlayerCharacter | `EditDefaultsOnly, BlueprintReadOnly, Category="Input"` |
| `LookAction` | `TObjectPtr<UInputAction>` | AApexPlayerCharacter | `EditDefaultsOnly, BlueprintReadOnly, Category="Input"` |
| `MouseLookAction` | `TObjectPtr<UInputAction>` | AApexPlayerCharacter | `EditDefaultsOnly, BlueprintReadOnly, Category="Input"` |

## 4. 为什么 Base 不包含 Camera / Input

1. **职责分离：** `AApexCharacterBase` 是「身体类」，代表角色的物理存在（碰撞、移动、骨骼网格）。Camera 和 Input 是「玩家视角与控制层」，只对玩家有意义。
2. **适用范围：** 如果 Base 包含 Camera，那么 AI 敌人、召唤物、NPC 都会携带无用的相机组件，浪费内存且增加混淆。
3. **可扩展性：** 未来可能有不同视角模式（第一人称、俯视角、固定镜头），这些应该在玩家层或组件层处理，不应污染基础类。
4. **输入同理：** 输入绑定只对玩家有意义。AI 由 Behavior Tree / StateTree 驱动，不需要 Enhanced Input。

## 5. 为什么 PlayerCharacter 不添加 MappingContext

1. **已有集中管理：** `AApexPlayerController::SetupInputComponent()` 已经负责通过 `UEnhancedInputLocalPlayerSubsystem` 添加 `DefaultMappingContexts` 和 `MobileExcludedMappingContexts`。
2. **避免重复：** 如果 `AApexPlayerCharacter` 再调用 `AddMappingContext`，同一个 Context 可能被添加两次，造成输入重复触发或优先级混乱。
3. **单一职责：** MappingContext 的生命周期管理属于 PlayerController 的职责（Controller 是「大脑」，Character 是「身体」），不应下放到 Character。

## 6. 编译结果

**✅ 编译成功。**

- 编译命令：`E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex`
- 编译时间：约 55 秒
- 输出 Dll：`D:/UnrealProject/Apex/Intermediate/Build/Win64/x64/UnrealEditor/Development/Apex/UnrealEditor-Apex-0001.dll`
- 编译过程有系统内存压力导致 UBA 重试编译进程，但最终所有 6 个 action 全部通过（2 个 .cpp 编译 + 1 个 Module 编译 + 1 个 LIB 链接 + 1 个 DLL 链接 + 1 个 Metadata 写入）。
- **Build.cs 无需修改**：现有 `EnhancedInput`、`InputCore`、`Engine` 依赖已覆盖所需类型。

## 7. 后续人工 UE 编辑器操作清单

以下操作需要用户在 UE 编辑器中手动完成：

1. **创建 `BP_Hero_Phase`：**
   - 在 Content Browser 中右键 → Blueprint Class。
   - 父类选择 `ApexPlayerCharacter`。
   - 命名：`BP_Hero_Phase`。

2. **设置 Mesh：**
   - 打开 `BP_Hero_Phase`。
   - 选中 `Mesh` 组件（继承自 Character）。
   - 在 Details 面板中设置 Skeletal Mesh 为 Paragon Phase 的 SK Mesh 资产。

3. **设置 Anim Class：**
   - 在 `Mesh` 组件的 Details 面板中设置 Anim Class。
   - 可先留空；如有 Paragon Phase 自带 AnimBP，则设置为该 AnimBP。

4. **配置 InputAction 引用：**
   - 在 `BP_Hero_Phase` 的 Class Defaults 中，找到 Input 分类。
   - 设置 `JumpAction`、`MoveAction`、`LookAction`、`MouseLookAction` 为项目中已有的 InputAction 资产。

5. **在 GameMode 或关卡中指定默认 Pawn：**
   - 打开当前使用的 GameMode 蓝图（`BP_ThirdPersonGameMode`）或关卡。
   - 将 Default Pawn Class 设置为 `BP_Hero_Phase`。

6. **PIE 验证：**
   - 启动 PIE。
   - 验证 WASD 移动、鼠标/摇杆视角、空格跳跃是否正常工作。

## 8. 未做事项

| 事项 | 状态 |
|------|------|
| AnimBP / AnimInstance | 未实现（本次范围外） |
| Montage 播放 | 未实现 |
| GAS (AbilitySystemComponent) | 未接入 |
| GameplayAbility | 未创建 |
| GameplayEffect | 未创建 |
| AttributeSet | 未创建 |
| GameplayTag | 未添加 |
| GameplayCue | 未添加 |
| 技能输入绑定 | 未实现 |
| 蓝图资产修改 | 未操作 |
| 地图修改 | 未操作 |
| GameMode 修改 | 未操作 |
| 旧模板类删除/重命名 | 未操作 |
| Build.cs 修改 | 不需要（现有依赖已满足） |

---

**实施人：** ClaudeCode
**审查状态：** 待 Codex 审查
