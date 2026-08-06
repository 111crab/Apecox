# ClaudeCode Prompt - Apex Vital AttributeSet 冷启动

项目：`D:\UnrealProject\Apex`

## 背景

Apex 是多人第三人称英雄技能框架原型。当前已经建立并验证：

```text
OwnerActor  = AApexPlayerState
AvatarActor = AApexPlayerCharacter
```

`AApexPlayerState` 持有 `UApexAbilitySystemComponent`，单人和 Listen Server + Client 均验证正确。本批只建立最小 AttributeSet 基础，不实现技能、GE 或伤害。

## 目标

严格按 `Agent/00_Coordination/Current_Code_Design.md`：

1. 新增无业务属性的 `UApexAttributeSet`。
2. 新增 `UApexVitalAttributeSet`。
3. 在 `AApexPlayerState` 创建并只读暴露 Vital AttributeSet。
4. 实现四项资源属性的默认值、复制、RepNotify、钳制和最大值不变量。
5. 编译并写中文实施报告。

## 开始前读取

```text
.agents/ue-project-context.md
Agent/00_Coordination/Working_Agreement.md
Agent/00_Coordination/Subagent_Review_Checklist.md
Agent/00_Coordination/Current_Code_Design.md
Agent/90_References/AttributeSet_Best_Practices/Lyra_AttributeSet_Best_Practices_Summary.md
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
Source/Apex/Public/Player/ApexPlayerState.h
Source/Apex/Private/Player/ApexPlayerState.cpp
```

先运行 `git status --short`。保留全部现有未提交改动，不得 reset、restore、清理或覆盖范围外文件。

## 允许修改

新增：

```text
Source/Apex/Public/AbilitySystem/Attributes/ApexAttributeSet.h
Source/Apex/Private/AbilitySystem/Attributes/ApexAttributeSet.cpp
Source/Apex/Public/AbilitySystem/Attributes/ApexVitalAttributeSet.h
Source/Apex/Private/AbilitySystem/Attributes/ApexVitalAttributeSet.cpp
Agent/Reports/2026-07-23_Apex_Vital_AttributeSet_ColdStart_Report.md
```

修改：

```text
Source/Apex/Public/Player/ApexPlayerState.h
Source/Apex/Private/Player/ApexPlayerState.cpp
```

需要修改其他文件时停止并报告，不要自行扩大范围。

## 必须实现

### UApexAttributeSet

- 继承 `UAttributeSet`，使用 `APEX_API`。
- 声明 `APEX_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)`。
- 提供：

```cpp
UApexAbilitySystemComponent* GetApexAbilitySystemComponent() const;
```

- 不保存具体 Attribute，不添加事件委托或业务逻辑。

### UApexVitalAttributeSet

- 继承 `UApexAttributeSet`，使用 `APEX_API` 和 `BlueprintType`。
- 私有属性及初始值：

```text
Health     = 100
MaxHealth  = 100
Mana       = 100
MaxMana    = 100
```

- 四项使用 `FGameplayAttributeData`、`BlueprintReadOnly`、`ReplicatedUsing`、`Category="Apex|Vital"` 和 `AllowPrivateAccess`。
- 四项均使用 `APEX_ATTRIBUTE_ACCESSORS`。
- 复制条件：

```text
Health / MaxHealth -> COND_None
Mana / MaxMana     -> COND_OwnerOnly
```

- 所有 RepNotify 使用 `GAMEPLAYATTRIBUTE_REPNOTIFY`。
- 重写：

```cpp
GetLifetimeReplicatedProps
PreAttributeBaseChange
PreAttributeChange
PostAttributeChange
```

- 统一私有 `ClampAttribute()`，规则：

```text
0 <= Health <= MaxHealth
1 <= MaxHealth
0 <= Mana <= MaxMana
0 <= MaxMana
```

- 最大值降低且当前值超过新上限时，通过 ASC 属性修改入口把当前值覆盖到新上限。
- 最大值提高时不自动补满，不保持比例。
- 不实现 `PreGameplayEffectExecute`、`PostGameplayEffectExecute`、Meta Attribute 或伤害逻辑。

### AApexPlayerState

- 前置声明 `UApexVitalAttributeSet`。
- 私有新增：

```cpp
TObjectPtr<UApexVitalAttributeSet> VitalAttributeSet;
```

- 使用 `VisibleAnywhere`、`BlueprintReadOnly`、`Category="Ability System"` 和 `AllowPrivateAccess`。
- 构造函数在 ASC 创建后：

```cpp
VitalAttributeSet = CreateDefaultSubobject<UApexVitalAttributeSet>(TEXT("VitalAttributeSet"));
```

- 新增：

```cpp
const UApexVitalAttributeSet* GetVitalAttributeSet() const;
```

- 不调用 `AddAttributeSetSubobject()`，避免重复注册默认子对象。

## 注释

使用简洁中文注释解释：

- 公共基类为何不保存具体属性。
- Health 全端复制、Mana OwnerOnly 的设计原因。
- BaseValue 和 CurrentValue 都需要钳制的原因。
- 最大值降低时为什么同步压低当前值。

不要逐行翻译代码，也不要把未来完整伤害系统写进注释。

## 禁止事项

- 不新增 `UApexCombatAttributeSet`。
- 不新增 `IncomingDamage`、`IncomingHealing`。
- 不创建或修改 GA、GE、ExecCalc、GameplayTag、GameplayCue、DataAsset、蓝图或资产。
- 不修改 Build.cs、uproject、配置或现有 ASC/Character/GameMode。
- 不使用 MCP。
- 不生成 Rider/Visual Studio 项目文件。
- 不执行 Git add、commit、push。

## 编译

```text
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApexEditor Win64 Development -Project="D:\UnrealProject\Apex\Apex.uproject" -WaitMutex
```

如遇到 Live Coding、编辑器占用或环境问题，记录原始错误并停止，不得通过扩大代码范围绕过。

## 报告

写入：

```text
Agent/Reports/2026-07-23_Apex_Vital_AttributeSet_ColdStart_Report.md
```

中文报告必须列出：

- 实际修改文件和是否超出范围。
- 两个新增类的父类、职责和路径。
- 四个 Attribute 的类型、初始值、复制条件和作用。
- `AApexPlayerState` 新成员与 Getter。
- 所有新增函数签名和作用。
- 钳制及最大值联动规则。
- 是否新增 Tag、GA、GE、ExecCalc、Meta Attribute、DataAsset、Cue 或 UE 资产。
- 编译命令和结果。
- 尚未执行的 PIE / 多人验证。

完成后停止，等待 Codex 审查。
