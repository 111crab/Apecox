# ClaudeCode Prompt - Apex AnimInstance 冷启动

生成日期：2026-07-16

## 你的身份与协作背景

你是 ClaudeCode，负责在明确范围内执行 C++ 修改、编译验证并输出中文实施报告。

- 用户负责最终决策、UE 编辑器人工操作和 PIE 验证。
- Codex 负责规划、命名预审、代码审查和经验记录。
- 当前项目是 `Apex`，不是旧项目 `Aura`。

项目路径：

```text
D:\UnrealProject\Apex
```

## 当前任务目标

只实现项目自有 `UApexAnimInstance` C++ 状态翻译层，并补上现有 PlayerCharacter Override 缺失的 `Super` 调用。

本次不创建 BlendSpace、AnimBP、状态机、IMC 或任何 UE 资产。C++ 审查通过后，用户会按 Codex 的人工清单在 UE 编辑器中完成这些操作。

## 必须先读取

```text
D:\UnrealProject\Apex\.agents\ue-project-context.md
D:\UnrealProject\Apex\Agent\00_Coordination\Working_Agreement.md
D:\UnrealProject\Apex\Agent\00_Coordination\Subagent_Review_Checklist.md
D:\UnrealProject\Apex\Agent\00_Coordination\Current_Code_Design.md
D:\UnrealProject\Apex\Source\Apex\Public\Character\ApexCharacterBase.h
D:\UnrealProject\Apex\Source\Apex\Private\Character\ApexCharacterBase.cpp
D:\UnrealProject\Apex\Source\Apex\Public\Character\ApexPlayerCharacter.h
D:\UnrealProject\Apex\Source\Apex\Private\Character\ApexPlayerCharacter.cpp
D:\UnrealProject\Apex\Source\Apex\Apex.Build.cs
```

以 `Current_Code_Design.md` 为本次已批准设计；若发现实现上无法成立的内容，停止扩展并在报告中说明，不得自行改架构。

## 允许修改的文件

只允许新增：

```text
D:\UnrealProject\Apex\Source\Apex\Public\Animation\ApexAnimInstance.h
D:\UnrealProject\Apex\Source\Apex\Private\Animation\ApexAnimInstance.cpp
```

只允许修改：

```text
D:\UnrealProject\Apex\Source\Apex\Private\Character\ApexPlayerCharacter.cpp
```

只允许新增报告：

```text
D:\UnrealProject\Apex\Agent\Reports\2026-07-16_Apex_AnimInstance_ColdStart_Report.md
```

除非编译必需，不得修改其他文件。若确有必要，先停止并在回复中说明，不要自行扩大范围。

## 明确禁止

- 不创建或修改 `.uasset`、蓝图、地图、Skeleton、Montage、BlendSpace、AnimBP、IMC。
- 不使用 UE MCP 或其他 MCP 操作编辑器。
- 不修改 `Apex.Build.cs`；本批只使用已有 `Engine` 依赖。
- 不引入 GameplayAbilities、GameplayTags、GameplayTasks、Niagara。
- 不新增 ASC、GA、GE、AttributeSet、GameplayCue、GameplayTag、DataAsset。
- 不添加 `MovementDirection`、AimOffset、Foot IK、Motion Matching、Linked Anim Layer。
- 不重构 Character、PlayerController 或现有输入系统。
- 不重新生成 `.sln` / `.slnx`，项目文件生成应在 Codex 审查和用户确认后执行。
- 不执行 Git add、commit、push。
- 不主动读取 Lyra、Aura 或其他学习项目；当前设计已经确定。

## 新增类：UApexAnimInstance

### 文件

```text
Source/Apex/Public/Animation/ApexAnimInstance.h
Source/Apex/Private/Animation/ApexAnimInstance.cpp
```

### 父类与声明

```cpp
UAnimInstance
```

使用项目导出宏，并允许 AnimBP 继承：

```cpp
UCLASS(Transient, Blueprintable)
class APEX_API UApexAnimInstance : public UAnimInstance
```

头文件放在 `Public/Animation`，实现放在 `Private/Animation`。`.cpp` 从 Public 根引用：

```cpp
#include "Animation/ApexAnimInstance.h"
```

## 已批准成员变量

### 内部引用

名称和类型必须为：

```cpp
TObjectPtr<ACharacter> OwningCharacter;
TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;
```

要求：

- `UPROPERTY(Transient)`。
- Private，仅供 C++ 内部缓存。
- 使用 `ACharacter`，不要收窄为 `AApexPlayerCharacter`。

### AnimBP 只读状态

名称和类型必须为：

```cpp
float GroundSpeed = 0.f;
float VerticalSpeed = 0.f;
bool bHasAcceleration = false;
bool bIsFalling = false;
```

要求：

- `UPROPERTY(Transient, BlueprintReadOnly, Category="Animation|Locomotion")`。
- 放在 `protected`，供 AnimBP 读取但不从蓝图写入。
- 不添加 Setter、BlueprintCallable 或额外暴露。

## 已批准函数

```cpp
virtual void NativeInitializeAnimation() override;
virtual void NativeUpdateAnimation(float DeltaSeconds) override;

void CacheOwnerReferences();
void ResetLocomotionState();
```

要求：

- 两个 Override 放在 `protected`。
- 两个 Helper 放在 `private`。
- Override 必须先调用对应的 `Super`。
- 不声明为 UFUNCTION。

### NativeInitializeAnimation

- 调用 `Super::NativeInitializeAnimation()`。
- 调用 `CacheOwnerReferences()`。
- Owner 为空是合法的编辑器预览状态，不得 `check()`。

### CacheOwnerReferences

- 使用 `TryGetPawnOwner()` 获取 Pawn，并 Cast 为 `ACharacter`。
- 如果成功，缓存 `GetCharacterMovement()`。
- 如果失败，将两个内部引用清空。

### NativeUpdateAnimation

- 调用 `Super::NativeUpdateAnimation(DeltaSeconds)`。
- 引用无效或当前 Pawn 已变化时，重新调用 `CacheOwnerReferences()`。
- 如果 Character 或 MovementComponent 仍无效，调用 `ResetLocomotionState()` 后返回。
- 从 `CharacterMovementComponent->Velocity` 计算：

```cpp
GroundSpeed = Velocity.Size2D();
VerticalSpeed = Velocity.Z;
```

- 使用当前二维加速度是否大于 `KINDA_SMALL_NUMBER` 更新 `bHasAcceleration`。
- 使用 `CharacterMovementComponent->IsFalling()` 更新 `bIsFalling`。

### ResetLocomotionState

将四个状态恢复为：

```cpp
GroundSpeed = 0.f;
VerticalSpeed = 0.f;
bHasAcceleration = false;
bIsFalling = false;
```

## 线程和职责边界

- 在 `NativeUpdateAnimation` 中读取 CharacterMovement UObject 状态并缓存为基础类型。
- AnimBP 后续只读取这些缓存，不在 EventGraph 每帧 Cast Character。
- 本类不控制 CharacterMovement，不调用 Jump，不激活 Ability，不播放 Montage。
- 不新增 Character Tick。
- 本批不使用 `NativeThreadSafeUpdateAnimation` 或自定义 AnimInstanceProxy；当前状态量很小，不提前复杂化。

## PlayerCharacter 窄修复

在：

```text
AApexPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
```

函数开头补充：

```cpp
Super::SetupPlayerInputComponent(PlayerInputComponent);
```

只做这一处修复，不改变现有 Action、函数名或绑定关系。

## 注释要求

- 使用简洁中文注释解释类是“玩法移动状态到 AnimBP 的翻译层”。
- 解释为什么缓存 `ACharacter` / MovementComponent，而不是让每个 AnimBP EventGraph 重复 Cast。
- 解释 Owner 为空可能来自编辑器预览或初始化时序。
- 不逐行翻译代码，不堆积噪音注释。

## 编译验证

不要重新生成项目文件。直接尝试：

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如果 UE 编辑器、Live Coding 或环境状态阻止编译，保留完整错误并在报告中说明，不要通过扩大代码范围规避。

## 中文实施报告

创建：

```text
D:\UnrealProject\Apex\Agent\Reports\2026-07-16_Apex_AnimInstance_ColdStart_Report.md
```

报告必须包含：

1. 实际修改文件，是否超出允许范围。
2. `UApexAnimInstance` 的父类、职责与文件路径。
3. 所有成员变量：名称、类型、访问级别、UPROPERTY 和作用。
4. 所有函数：签名、访问级别、作用、是否 UFUNCTION。
5. `SetupPlayerInputComponent` 的具体修复。
6. 是否修改 Build.cs、配置、蓝图、地图或任何资产。
7. 是否新增 GameplayTag、Attribute、DataAsset、GA、GE、Cue。
8. 编译命令、结果和关键错误（如有）。
9. 尚未执行的 UE 人工步骤：BlendSpace、AnimBP、状态机、`IMC_Apex_BaseMove`、IA 切换和 PIE。
10. 需要 Codex 重点审查的风险或设计偏差。

## 完成后回复

简要回复：

- 是否编译成功。
- 新增和修改了哪些文件。
- 是否完全遵守范围。
- 报告文件路径。
- 若失败，失败原因与下一步建议。
