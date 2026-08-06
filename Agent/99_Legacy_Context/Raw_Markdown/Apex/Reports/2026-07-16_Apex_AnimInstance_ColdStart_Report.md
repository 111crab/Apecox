# Apex AnimInstance 冷启动 — 实施报告

生成日期：2026-07-16

## 1. 实际修改文件

| 文件 | 操作 | 是否超出允许范围 |
|------|------|:--:|
| `Source/Apex/Public/Animation/ApexAnimInstance.h` | 新增 | 否 |
| `Source/Apex/Private/Animation/ApexAnimInstance.cpp` | 新增 | 否 |
| `Source/Apex/Private/Character/ApexPlayerCharacter.cpp` | 修改（加 1 行） | 否 |
| `Source/Apex/Apex.Build.cs` | 未修改 | 否 |
| `Agent/Reports/2026-07-16_Apex_AnimInstance_ColdStart_Report.md` | 新增（本报告）| 否 |

**结论：严格在允许范围内，未修改其他任何文件。**

## 2. UApexAnimInstance 概况

| 项目 | 内容 |
|------|------|
| **类名** | `UApexAnimInstance` |
| **父类** | `UAnimInstance` |
| **声明** | `UCLASS(Transient, Blueprintable)`, `APEX_API` |
| **头文件** | `Source/Apex/Public/Animation/ApexAnimInstance.h` |
| **实现文件** | `Source/Apex/Private/Animation/ApexAnimInstance.cpp` |
| **职责** | 将 CharacterMovement 玩法移动状态翻译为 AnimBP 可读取的稳定缓存值（float / bool），避免 AnimBP EventGraph 每帧 Cast Character 并重复计算。 |

## 3. 成员变量一览

### 3.1 AnimBP 只读状态（protected）

| 变量名 | 类型 | UPROPERTY | 作用 |
|--------|------|-----------|------|
| `GroundSpeed` | `float` | `Transient, BlueprintReadOnly, Category="Animation\|Locomotion"` | Velocity 2D 长度，驱动 1D Locomotion BlendSpace |
| `VerticalSpeed` | `float` | `Transient, BlueprintReadOnly, Category="Animation\|Locomotion"` | Velocity.Z，区分上升跳跃与下落 |
| `bHasAcceleration` | `bool` | `Transient, BlueprintReadOnly, Category="Animation\|Locomotion"` | 是否存在加速度输入，区分主动移动与惯性滑动 |
| `bIsFalling` | `bool` | `Transient, BlueprintReadOnly, Category="Animation\|Locomotion"` | 是否处于空中（IsFalling） |

初始值均为安全默认值（0 / false）。

### 3.2 内部缓存引用（private）

| 变量名 | 类型 | UPROPERTY | 作用 |
|--------|------|-----------|------|
| `OwningCharacter` | `TObjectPtr<ACharacter>` | `Transient` | 缓存 TryGetPawnOwner() 的角色引用 |
| `CharacterMovementComponent` | `TObjectPtr<UCharacterMovementComponent>` | `Transient` | 缓存角色的移动组件，避免 AnimBP 每帧动态查询 |

使用 `ACharacter` 而非 `AApexPlayerCharacter`，使 AI、敌人等也能复用本 AnimInstance。

## 4. 函数一览

| 函数 | 签名 | 访问级别 | UFUNCTION | 作用 |
|------|------|:--:|:--:|------|
| `NativeInitializeAnimation` | `virtual void NativeInitializeAnimation() override` | protected | 否 | 调用 `Super` → 缓存 Owner 引用 |
| `NativeUpdateAnimation` | `virtual void NativeUpdateAnimation(float DeltaSeconds) override` | protected | 否 | 调用 `Super` → 必要时重新缓存引用 → 从 CharacterMovement 读取并更新 4 个状态变量 |
| `CacheOwnerReferences` | `void CacheOwnerReferences()` | private | 否 | 通过 TryGetPawnOwner() 获取 ACharacter 并缓存 MovementComponent；失败时清空两者 |
| `ResetLocomotionState` | `void ResetLocomotionState()` | private | 否 | 将 4 个 Locomotion 状态重置为 0 / false |

所有 Override 函数均先调用对应 `Super`。

## 5. SetupPlayerInputComponent 窄修复

- **文件：** `Source/Apex/Private/Character/ApexPlayerCharacter.cpp`
- **位置：** `AApexPlayerCharacter::SetupPlayerInputComponent()` 函数体第一行
- **改动：** 新增一行 `Super::SetupPlayerInputComponent(PlayerInputComponent);`
- **原因：** UE Framework Override 标准父类调用，确保 Character / ApexCharacterBase 链上的输入初始化不被跳过。
- **影响：** 不改变现有 Action、函数名或绑定关系。

## 6. Build.cs / 配置 / 蓝图 / 地图 / 资产

- **Apex.Build.cs：** 未修改。现有 `Engine` 依赖已覆盖 `UAnimInstance`、`ACharacter`、`UCharacterMovementComponent`。
- **配置：** 未修改。
- **蓝图、地图、资产：** 未创建、未修改。

## 7. GameplayTag / Attribute / DataAsset / GA / GE / Cue

**均未新增。** 本批严格只做 C++ AnimInstance 状态翻译层，不涉及 GAS 任何子系统。

## 8. 编译结果

- **命令：**
  ```
  E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
  ```
- **结果：** ✅ Succeeded
- **编译动作：** 4 个 action（ApexAnimInstance.cpp 编译 + LIB 链接 + DLL 链接 + Metadata 写入），总耗时约 6.6 秒。
- **遇到错误：** 第一次编译时 `TObjectPtr::IsValid()` 不存在（UE 5.8 的 `TObjectPtr` 未提供该成员函数）。已修复为 `::IsValid(OwningCharacter.Get())` 全局函数形式。第二次编译全部通过。该问题属于 UE 5.8 版本的 API 差异，非设计偏差。

## 9. 尚未执行的 UE 人工步骤

以下步骤需用户在 UE 编辑器中手动完成：

### 9.1 C++ 上线前

1. **重新生成项目文件**（让 Rider 识别新增目录）：
   ```
   ./Scripts/RegenerateProjectFiles.ps1
   ```

### 9.2 BlendSpace 创建

1. 在 `/Game/Blueprints/Characters/Phase/Animation/` 中右键 → Animation → Blend Space 1D。
2. 选择 Skeleton：`phase_Skeleton`。
3. 命名：`BS_Apex_Phase_Locomotion`。
4. 设置：Axis Name = `GroundSpeed`，Min = `0`，Max = `500`。
5. 在 0 处放置 `Idle` 动画，在 500 处放置 `Jog_Fwd` 动画。

### 9.3 AnimBP 创建

1. 在 `/Game/Blueprints/Characters/Phase/Animation/` 中创建 Animation Blueprint。
2. Skeleton：`phase_Skeleton`。
3. Parent Class：`UApexAnimInstance`。
4. 命名：`ABP_Apex_Phase`。
5. EventGraph：保持为空（C++ 已提供 `GroundSpeed` 等状态变量）。
6. AnimGraph 结构：
   ```
   Locomotion State Machine → Save Cached Pose "LocomotionPose"
   → UpperBody Slot + Layered Blend Per Bone（spine_01）
   → FullBody Slot → Output Pose
   ```
7. Locomotion 状态机添加 4 个状态：
   - `Grounded` → `BS_Apex_Phase_Locomotion`
   - `JumpStart` → Phase `Jump_Start`（不循环）
   - `Falling` → Phase `Jump_Apex`
   - `Land` → Phase `Jump_Land`（不循环）
8. 转换规则详见 `Current_Code_Design.md` 第 9 节。

### 9.4 InputAction 与 IMC 配置

1. **确认项目自有 IA 存在：**
   - `/Game/Blueprints/Input/Actions/IA_Jump`
   - `/Game/Blueprints/Input/Actions/IA_Move`
   - `/Game/Blueprints/Input/Actions/IA_Look`
   - `/Game/Blueprints/Input/Actions/IA_MouseLook`
2. **创建 `IMC_Apex_BaseMove`：**
   - 路径：`/Game/Blueprints/Input/IMC_Apex_BaseMove`
   - 类型：Input Mapping Context
   - 包含键盘/鼠标/手柄的基础移动、视角和跳跃映射。
3. **BP_Hero_Phase 切换 IA：**
   - 打开 `BP_Hero_Phase` → Class Defaults → Input 分类。
   - 将 `JumpAction`、`MoveAction`、`LookAction`、`MouseLookAction` 全部指向项目自有 IA。
4. **PlayerController 切换 IMC：**
   - 当前使用的 PlayerController 的 `DefaultMappingContexts` 改为只包含 `IMC_Apex_BaseMove`。

### 9.5 最终 PIE 验证

1. 打开编辑器，确认 `BP_Hero_Phase.Mesh.AnimClass = ABP_Apex_Phase`。
2. PIE 启动，验证：
   - Idle ↔ Jog 根据速度平滑过渡。
   - 主动跳跃播放 JumpStart → Falling → Land。
   - 走下平台直接进入 Falling（VerticalSpeed ≤ 0，不触发 JumpStart）。
   - WASD / 鼠标视角 / Space 使用项目自有 IA / IMC 正常工作。
   - 无崩溃、无空引用错误。

## 10. 需要 Codex 重点审查的风险或设计偏差

| 项目 | 说明 |
|------|------|
| **TObjectPtr::IsValid() 兼容性** | UE 5.8 的 `TObjectPtr` 未提供 `IsValid()` 成员函数，已改用 `::IsValid(ptr.Get())` 全局函数。这是本次唯一与 Prompt 静态设计的偏差，建议在项目中统一采用 `::IsValid(ptr.Get())` 或 `ptr != nullptr` 风格。 |
| **ApexPlayerCharacter.cpp 改动已编译但在第二次增量编译中被跳过** | 第一次（失败的）编译已将该文件的 `Super::` 修复编译进 `.obj`；第二次只重编了 `ApexAnimInstance.cpp`。最终 DLL 包含了全部改动，编译结果正确。 |
| **未验证 AnimBP 运行时行为** | 本批只完成 C++ 翻译层。AnimBP 状态机连接、BlendSpace 采样和转换参数的正确性需在 PIE 中验证。 |

---

**实施人：** ClaudeCode
**审查状态：** 待 Codex 审查

---

## 附录 A：Codex 审查 P2 修复记录（2026-07-16）

### 审查文件

`Agent/Reviews/2026-07-16_Apex_AnimInstance_ColdStart_Codex_Review.md`

### 问题描述

P2：`NativeUpdateAnimation()` 的重新缓存条件只检查 `OwningCharacter` 无效或 Pawn 切换，未检查 `CharacterMovementComponent` 是否已失效。如果 `OwningCharacter` 仍有效但 `CharacterMovementComponent` 已失效，代码会每帧重置动画状态但不尝试重新获取 MovementComponent。

### 修复内容

**文件：** `Source/Apex/Private/Animation/ApexAnimInstance.cpp`

**位置：** `NativeUpdateAnimation()` 函数，重新缓存条件（原第 20 行）。

**改动前：**
```cpp
// 如果缓存引用无效或 Pawn 已切换，重新缓存。
if (!::IsValid(OwningCharacter.Get()) || OwningCharacter != TryGetPawnOwner())
```

**改动后：**
```cpp
// 如果任一缓存引用无效或 Pawn 已切换，重新缓存。
if (!::IsValid(OwningCharacter.Get())
    || !::IsValid(CharacterMovementComponent.Get())
    || OwningCharacter != TryGetPawnOwner())
```

**说明：** 仅新增一个 `CharacterMovementComponent` 有效性检查条件，并同步将注释从"缓存引用无效"改为"任一缓存引用无效"。未修改类名、成员变量、函数签名或其他逻辑。

### 编译结果

- **命令：** `E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex`
- **结果：** ✅ Succeeded（增量编译 7.4 秒，1 个编译 action + 链接）

### 审查状态更新

- P2 问题已修复，待 Codex 复审。
- 复审通过后可进入 `Scripts/RegenerateProjectFiles.ps1` 和 UE 人工配置阶段。
