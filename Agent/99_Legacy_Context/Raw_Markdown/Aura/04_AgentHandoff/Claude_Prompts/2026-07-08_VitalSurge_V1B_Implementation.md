# ClaudeCode 实施 Prompt：生命涌动 V1-B 配置化技能

创建日期：2026-07-08  
状态：用户已确认设计方向，可交给 ClaudeCode 执行  
负责人：ClaudeCode 执行，Codex 审查，用户最终验证

## 任务目标

在 Aura 项目中实现第二个配置化技能：`生命涌动 / Vital Surge`。

本任务目标不是重构整个技能系统，也不是实现完整 SkillGraph。目标是在不破坏火球术 V1-A 的前提下，扩展现有 `UAuraSkillDefinition + UAuraConfiguredActiveAbility + AbilitySpec.SourceObject` 路线，使它支持：

```text
按住输入
-> 开始引导
-> 松开结算
-> 满 2 秒：瞬间治疗
-> 不满 2 秒：半量持续治疗
-> 引导期间尝试移动：取消技能，不治疗
```

## 用户已确认的规则

1. 技能名方向：生命涌动 / Vital Surge。
2. 输入使用 `InputTag.2`。
3. 按住开始引导，松开结算。
4. 完整引导时间先用 `2.0` 秒。
5. 一旦尝试移动就取消技能。
6. 完整引导是瞬间治疗。
7. 未完整引导是固定半量治疗，并且以持续恢复形式生效。
8. 数值先用占位。
9. 本技能不使用 Montage。
10. 新增通用 `EffectEntry`，不要新增临时 `DirectEffectClass`。

## 必读文件

实施前请先阅读：

- `.agents/ue-project-context.md`
- `Agent/00_Coordination/WorkingAgreement.md`
- `Agent/02_SkillSystem/Skill_Runbooks/Fireball_Runtime_Link.md`
- `Agent/02_SkillSystem/Skill_Runbooks/VitalSurge_Runtime_Link_Predesign.md`
- `Agent/02_SkillSystem/Skill_Runtime_Risk_Checklist.md`
- `Agent/02_SkillSystem/Montage_GameplayEvent_Timing_Decision.md`
- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp`
- `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/Player/AuraPlayerController.h`
- `Source/Aura/Private/Player/AuraPlayerController.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`

建议使用的 UE 知识/Skill：

- `ue-gameplay-abilities`
- `ue-input-system`
- `ue-animation-system`
- `ue-networking-replication`
- `ue-data-assets-tables`
- `ue-cpp-foundations`
- `ue-module-build-system`

## 允许修改范围

允许修改或新增：

- `Source/Aura/Public/AbilitySystem/Data/`
- `Source/Aura/Private/AbilitySystem/Data/`
- `Source/Aura/Public/AbilitySystem/Abilities/`
- `Source/Aura/Private/AbilitySystem/Abilities/`
- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`
- `Source/Aura/Public/Player/AuraPlayerController.h`
- `Source/Aura/Private/Player/AuraPlayerController.cpp`
- `Source/Aura/Public/AuraGameplayTags.h`
- `Source/Aura/Private/AuraGameplayTags.cpp`
- `Config/DefaultGameplayTags.ini`
- 必要时 `Source/Aura/Aura.Build.cs`
- `Agent/02_SkillSystem/Skill_Runbooks/` 下的中文说明文档
- `Agent/04_AgentHandoff/Claude_Reports/` 下的中文实施报告

如果 MCP/UE 编辑器自动化可用，允许创建或修改：

- `/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge`
- `/Game/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_InstantHeal`
- `/Game/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_HealOverTime`
- `BP_AuraCharacter.StartupSkillDefinitions`

如果 MCP/UE 编辑器自动化不可用，不要用脚本硬写 `.uasset`。请在报告中写出清晰的手动创建步骤。

## 禁止事项

1. 不删除或重写旧 `GA_FireBolt`。
2. 不删除或重写 `UAuraProjectileSpell`。
3. 不破坏当前已验证的 `DA_Fireball + UAuraConfiguredActiveAbility` 火球术路径。
4. 不整体重构输入系统。
5. 不把 Editor-only 依赖加入 `Aura` Runtime 模块。
6. 不实现完整 SkillGraph / IR / VM。
7. 不把火球术迁移到 `EffectEntries`，本轮只为生命涌动扩展模型。
8. 不为生命涌动新增一个专用 C++ GameplayAbility 类。
9. 不直接调用 `DisableMovement()` 作为 V1 首选方案，避免异常结束后角色卡住。
10. 不为了本技能改写火球术 Montage 事件策略；生命涌动本轮不使用 Montage。

## C++ 实施要求

### 1. 扩展 `UAuraSkillDefinition`

文件：

```text
Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h
Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp
```

必须注意：不要改变已有 enum 值的含义。已有资产可能已经保存了 enum 的整数值。

推荐写法：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillTargetingMode : uint8
{
    CursorHit UMETA(DisplayName = "Cursor Hit"),
    Self UMETA(DisplayName = "Self")
};

UENUM(BlueprintType)
enum class EAuraSkillDeliveryMode : uint8
{
    Projectile UMETA(DisplayName = "Projectile"),
    Direct UMETA(DisplayName = "Direct")
};
```

也可以显式赋值：

```cpp
CursorHit = 0,
Self = 1
```

新增输入模式：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillInputMode : uint8
{
    Press,
    HoldRelease
};
```

新增效果条目：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillEffectTarget : uint8
{
    Self,
    Source,
    HitActor
};

UENUM(BlueprintType)
enum class EAuraSkillEffectCondition : uint8
{
    Always,
    FullCharge,
    PartialCharge
};

USTRUCT(BlueprintType)
struct FAuraSkillEffectEntry
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    TSubclassOf<UGameplayEffect> EffectClass;

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    EAuraSkillEffectTarget Target = EAuraSkillEffectTarget::Self;

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    EAuraSkillEffectCondition Condition = EAuraSkillEffectCondition::Always;

    UPROPERTY(EditDefaultsOnly, Category = "Effect")
    TMap<FGameplayTag, FScalableFloat> SetByCallerMagnitudes;
};
```

在 `UAuraSkillDefinition` 中新增：

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
EAuraSkillInputMode InputMode = EAuraSkillInputMode::Press;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
float FullChargeDuration = 0.f;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
float PartialChargeScale = 0.5f;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
bool bCancelOnMoveInputDuringChannel = false;

UPROPERTY(EditDefaultsOnly, Category = "Effect")
TArray<FAuraSkillEffectEntry> EffectEntries;
```

注释要求：

- 解释 `Press` 与 `HoldRelease` 的区别。
- 解释为什么 `EffectEntry` 比 `DirectEffectClass` 更适合长期扩展。
- 解释完整引导和未完整引导为什么用两个 `EffectEntry`，而不是一个 GE 简单乘比例。

### 2. 扩展 `UAuraConfiguredActiveAbility`

文件：

```text
Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h
Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp
```

要求保留现有火球术 Press / CursorHit / Projectile 路径。可以把现有 `ActivateAbility` 中的逻辑抽成 `StartPressFlow()`，但行为不要变。

新增 HoldRelease 路径：

```text
ActivateAbility
-> 读取 CachedSkillDef
-> CommitAbility
-> if CachedSkillDef->InputMode == HoldRelease:
      StartHoldReleaseFlow()
   else:
      StartPressFlow()
```

新增或调整函数建议：

```cpp
void StartPressFlow();
void StartHoldReleaseFlow();

UFUNCTION()
void OnHoldInputReleased(float TimeHeld);

void ApplyEffectEntriesToSelf(EAuraSkillEffectCondition ChargeResult);
void AddChannelingState();
void RemoveChannelingState();
```

`StartHoldReleaseFlow` 要求：

1. `TargetingMode` 必须是 `Self`。
2. `DeliveryMode` 必须是 `Direct`。
3. 不创建 `UTargetDataUnderMouse`。
4. 不播放 Montage。
5. 不生成投射物。
6. 记录引导开始时间，或者使用 `WaitInputRelease` 回调提供的 `TimeHeld`。
7. 添加引导状态，例如 `State.Skill.Channeling`。
8. 创建并激活 `UAbilityTask_WaitInputRelease`。
9. 所有失败路径必须 `EndAbility`。

`OnHoldInputReleased` 要求：

```text
if TimeHeld >= FullChargeDuration:
    ChargeResult = FullCharge
else:
    ChargeResult = PartialCharge

服务端应用匹配 Condition 的 EffectEntry
结束 Ability
```

注意：

1. 治疗只允许服务端应用。
2. 客户端可以进入等待释放和播放表现，但不能修改 Health。
3. 需要防重复，例如 `bEffectEntriesApplied`，避免 Listen Server 或回调重复导致治疗应用两次。
4. `EndAbility` 必须清理引导状态、AbilityTask 引用、缓存状态。
5. 不要直接从回调里绕过 GAS 生命周期强行改属性。

`ApplyEffectEntriesToSelf` 要求：

1. 获取 Source ASC。
2. 遍历 `EffectEntries`。
3. 跳过 `EffectClass` 为空的条目，并写 Warning。
4. 只应用 `Condition == Always` 或 `Condition == ChargeResult` 的条目。
5. 用 `GetAbilityLevel()` 计算 `FScalableFloat`。
6. 使用 `AssignTagSetByCallerMagnitude` 写入 SetByCaller。
7. 使用 `ApplyGameplayEffectSpecToSelf` 应用到自身。
8. Spec 创建失败时写 Error 并结束 Ability。

### 3. 移动输入取消

文件：

```text
Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h
Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp
Source/Aura/Private/Player/AuraPlayerController.cpp
```

推荐新增 ASC helper：

```cpp
bool CancelActiveChannelingAbilities();
```

行为建议：

1. 遍历 `GetActivatableAbilities()`。
2. 找到正在 Active 的 `FGameplayAbilitySpec`。
3. 检查 `Spec.SourceObject` 是否为 `UAuraSkillDefinition`。
4. 如果该 SkillDefinition 满足：
   - `InputMode == HoldRelease`
   - `bCancelOnMoveInputDuringChannel == true`
5. 使用 GAS 取消 API 按 SpecHandle 取消该 Ability。
6. 返回是否真的取消了任何技能。

为什么推荐这个 helper：

- 通用 `UAuraConfiguredActiveAbility` 可能服务多个技能。
- 单纯按 GA 类取消可能误伤未来别的配置技能。
- 单纯按 `State.Skill.Channeling` 取消不容易定位具体 Spec。
- 用 `SourceObject` 读回 `UAuraSkillDefinition`，最符合当前配置化路线。

在 `AAuraPlayerController::Move` 中：

1. 先读取 `InputAxisVector`。
2. 如果输入向量几乎为 0，不处理。
3. 如果有有效移动输入，调用 `GetASC()->CancelActiveChannelingAbilities()`。
4. 如果返回 true，说明移动尝试已经取消了引导技能，本帧不要继续 `AddMovementInput`。
5. 如果没有取消任何技能，保持原本移动逻辑。

这段代码必须有中文注释，解释为什么“移动取消”放在输入层，而不是只在释放时检查速度。

### 4. GameplayTags

需要新增或确认：

```text
Ability.Vital.VitalSurge
SetByCaller.Healing
SetByCaller.HealingPerTick
State.Skill.Channeling
```

如果 C++ 代码直接引用某些 Tag，优先加入 `FAuraGameplayTags` native tags，例如：

```cpp
FGameplayTag SetByCaller_Healing;
FGameplayTag SetByCaller_HealingPerTick;
FGameplayTag State_Skill_Channeling;
```

如果只是 DataAsset 身份 Tag，也可以先写在 `DefaultGameplayTags.ini`。请在报告中说明哪些是 native tag，哪些是 ini tag，以及为什么。

不要顺手修复 `Attributes.Resistance.lightning` 的大小写问题。本轮不做无关 tag 规范化。

### 5. UE 资产配置

如果 MCP/UE 编辑器自动化可用，请创建：

```text
/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge
/Game/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_InstantHeal
/Game/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_HealOverTime
```

`DA_VitalSurge` 建议：

| 字段 | 值 |
| --- | --- |
| `SkillTag` | `Ability.Vital.VitalSurge` |
| `DisplayName` | `生命涌动` |
| `InputTag` | `InputTag.2` |
| `AbilityClass` | `UAuraConfiguredActiveAbility` |
| `DefaultLevel` | `1` |
| `InputMode` | `HoldRelease` |
| `FullChargeDuration` | `2.0` |
| `PartialChargeScale` | `0.5` |
| `bCancelOnMoveInputDuringChannel` | `true` |
| `TargetingMode` | `Self` |
| `DeliveryMode` | `Direct` |
| `CastMontage` | 空 |
| `MontageSpawnEventTag` | 空 |
| `EffectEntries[0].EffectClass` | `GE_VitalSurge_InstantHeal` |
| `EffectEntries[0].Target` | `Self` |
| `EffectEntries[0].Condition` | `FullCharge` |
| `EffectEntries[0].SetByCallerMagnitudes` | `SetByCaller.Healing = 100` |
| `EffectEntries[1].EffectClass` | `GE_VitalSurge_HealOverTime` |
| `EffectEntries[1].Target` | `Self` |
| `EffectEntries[1].Condition` | `PartialCharge` |
| `EffectEntries[1].SetByCallerMagnitudes` | `SetByCaller.HealingPerTick = 25` |

注意：`PartialCharge` 条目的 SetByCaller 写未缩放基准值。运行时会乘 `PartialChargeScale = 0.5`，实际每跳为 `12.5`，4 秒总共恢复 `50`。

`GE_VitalSurge_InstantHeal` 建议：

| 配置项 | 值 |
| --- | --- |
| Duration Policy | `Instant` |
| Modifier Attribute | `Health` |
| Modifier Op | `Add` |
| Magnitude | `SetByCaller` |
| SetByCaller Tag | `SetByCaller.Healing` |

`GE_VitalSurge_HealOverTime` 建议：

| 配置项 | 值 |
| --- | --- |
| Duration Policy | `HasDuration` |
| Duration | `4.0` |
| Period | `1.0` |
| Modifier Attribute | `Health` |
| Modifier Op | `Add` |
| Magnitude | `SetByCaller` |
| SetByCaller Tag | `SetByCaller.HealingPerTick` |

最后把 `DA_VitalSurge` 加入 `BP_AuraCharacter.StartupSkillDefinitions`。不要移除 `DA_Fireball`，不要破坏火球术。

如果无法自动创建资产，请在报告中写出手动创建步骤，并说明 C++ 是否已编译通过。

## Montage 事件策略说明

生命涌动本轮不使用 Montage。

但请阅读并遵守：

```text
Agent/02_SkillSystem/Montage_GameplayEvent_Timing_Decision.md
```

本轮不要为了生命涌动大改火球术 Montage 逻辑。后续依赖 Montage 的技能采用以下原则：

1. 普通音效、脚步、纯 VFX 用普通 AnimNotify。
2. Ability 关键触发点用 GameplayEvent Notify。
3. 简单技能可默认等待 `Event.Montage.Cast.Release`。
4. 复杂技能显式配置等待的事件 Tag。

## 代码注释与文档要求

新增 C++ 注释要以学习为目标，重点解释：

1. 为什么 `EffectEntry` 是通用效果条目，而不是治疗专用字段。
2. `WaitInputRelease` 如何依赖 ASC 的 `AbilitySpecInputReleased`。
3. 为什么治疗必须由服务端应用。
4. 为什么移动取消放在输入层。
5. 为什么取消 Ability 必须走 `EndAbility` 清理。
6. 为什么枚举不能随意重排，以免破坏已有资产。

如果解释太长，不要把代码写得很吵；请写到 Markdown 报告或运行链路文档里。

请注意：当前部分中文注释在 PowerShell 输出中可能显示乱码。不要为了“修乱码”而批量重编码已有源文件，避免无关 diff。

## 编译要求

请使用 UE 5.8 构建命令：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat AuraEditor Win64 Development -Project="D:\UnrealProject\Aura\Aura.uproject" -WaitMutex -MaxParallelActions=4
```

如果环境无法运行编译，请在报告中明确说明原因，不要假装已通过。

## 验证要求

### 单人 PIE

请验证或给出手动验证步骤：

1. `DA_Fireball` 仍然可用，火球术不被破坏。
2. 按住 `2` 不到 2 秒后释放，出现总量 50 的持续治疗。
3. 按住 `2` 超过 2 秒后释放，立刻治疗 100。
4. 治疗不超过 `MaxHealth`。
5. UI 血球/血条正确刷新。
6. 引导期间尝试移动会取消技能，不产生治疗。
7. 技能取消或结束后，角色移动恢复正常。
8. 技能没有卡住，可以再次释放。

### Listen Server

如果可运行多人 PIE，请验证：

1. Host 短按/长按都能按规则治疗。
2. Client 短按/长按都能按规则治疗。
3. 治疗只由服务端结算。
4. 一次释放不会应用两次 GE。
5. 其他端能看到该同步的 Health 状态变化。
6. 引导取消或中断后，移动与再次释放都恢复正常。

如果不能验证多人，请写明原因，并给出用户手动验证清单。

## 必须输出的报告

请写入：

```text
Agent/04_AgentHandoff/Claude_Reports/2026-07-08_VitalSurge_V1B_Implementation_Report.md
```

报告必须包含：

1. 简要总结。
2. 修改/新增文件列表。
3. 新增字段、枚举、函数的职责说明。
4. 为什么本轮采用 `EffectEntry`。
5. 为什么移动取消放在输入层。
6. 是否创建了 `DA_VitalSurge` 和两个 GE 资产；如果没有，写手动创建步骤。
7. 执行了哪些编译、测试、MCP/UE 编辑器操作。
8. 单人验证结果。
9. 多人验证结果，或未验证原因。
10. 已知问题。
11. 下一步建议。

## 验收标准

本任务完成时，至少应满足：

1. C++ 编译通过，或报告中清楚说明未编译原因。
2. 火球术原有配置化路径不回退、不破坏。
3. `UAuraSkillDefinition` 支持 `Self`、`Direct`、`HoldRelease`、`EffectEntry`。
4. `UAuraConfiguredActiveAbility` 能执行生命涌动的 HoldRelease 自疗路径。
5. 完整引导瞬间治疗。
6. 未完整引导持续治疗。
7. 移动尝试取消技能且不治疗。
8. 治疗由服务端应用。
9. 报告和必要注释为中文，便于用户学习。
