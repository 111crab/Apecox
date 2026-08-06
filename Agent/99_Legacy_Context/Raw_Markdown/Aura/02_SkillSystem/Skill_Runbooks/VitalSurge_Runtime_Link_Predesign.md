# 生命涌动运行链路预设计

日期：2026-07-08  
状态：按用户最新反馈更新后的预设计，等待确认后交给 ClaudeCode 实现  
建议技能名：生命涌动 / Vital Surge  
建议输入：`InputTag.2`

## 当前已确认

1. 第二个技能采用“生命涌动 / Vital Surge”方向。
2. 使用 `InputTag.2`。
3. 技能支持短按和长按：按下开始引导，松开结算。
4. 长按完整引导时间暂定 `2.0` 秒。
5. 一旦玩家在引导期间尝试移动，就取消技能。
6. 完整引导后是瞬间治疗，不是持续恢复。
7. 未完整引导时先用简单规则：固定获得一半治疗量，并且这部分治疗以持续恢复形式生效。
8. 本技能先不使用 Montage。
9. 本轮采用通用 `EffectEntry`，不新增临时 `DirectEffectClass` 字段。
10. 数值先用占位，后续再调。

## 技能目标

火球术已经验证了“鼠标目标 + Montage 事件 + 投射物 + 命中伤害”的链路。生命涌动用于验证另一组常见技能机制：

1. `Self` 目标：目标就是施法者自己。
2. `Direct` 送达：不生成投射物，直接对自身 ASC 应用 GE。
3. `HoldRelease` 输入：按下开始，引导期间等待松开。
4. 引导型技能：根据按住时长选择完整效果或未完整效果。
5. 移动打断：引导期间只要玩家尝试移动，就取消技能。
6. 多种 GE 结果：完整引导走瞬间治疗 GE，未完整引导走持续治疗 GE。
7. 通用 `EffectEntry`：为未来技能编辑器的“效果条目列表”打基础。

这个技能故意不依赖 Montage，因为它的关键时机来自输入释放和引导时长，而不是动画帧。

## 玩家视角流程

1. 玩家按住 `2` 键，角色开始引导生命涌动。
2. 引导期间如果玩家按移动键或产生有效移动输入，技能立即取消。
3. 玩家松开 `2` 键时，技能根据引导时长结算。
4. 如果引导时间达到 `2.0` 秒，立即恢复完整治疗量。
5. 如果引导时间不足 `2.0` 秒，获得一半治疗量，但通过持续恢复 GE 分几跳生效。
6. 治疗后的 Health 仍由 `UAuraAttributeSet` clamp 到 `0..MaxHealth`。
7. UI 血球/血条通过现有属性复制和委托刷新。

## 与火球术的机制差异

| 机制点 | 火球术 | 生命涌动 |
| --- | --- | --- |
| 输入模式 | 按键触发后执行 | 按住开始，引导，松开结算 |
| 目标选择 | 鼠标命中点 `CursorHit` | 自身 `Self` |
| 效果送达 | 投射物 `Projectile` | 直接应用 `Direct` |
| 时机来源 | Montage Notify：`Event.Montage.FireBolt` | 输入释放 + 引导时长 |
| GE 类型 | 伤害 GE + `ExecCalc_Damage` | 瞬间治疗 GE + 持续治疗 GE |
| 网络重点 | 投射物复制、命中权威、伤害同步 | 输入释放同步、服务端结算、移动取消、治疗同步 |
| UI 验证 | 目标血量变化、伤害飘字 | 自身 Health UI 变化 |

## 现有项目支点

| 类/函数 | 文件 | 复用方式 |
| --- | --- | --- |
| `UAuraAbilitySystemComponent::AddCharacterSkillDefinitions` | `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp` | 继续通过 `UAuraSkillDefinition` 授予技能 |
| `UAuraAbilitySystemComponent::AbilityInputTagHeld` | 同上 | 当前会激活 Ability，并调用 `AbilitySpecInputPressed` |
| `UAuraAbilitySystemComponent::AbilityInputTagReleased` | 同上 | 当前会调用 `AbilitySpecInputReleased`，可驱动 `WaitInputRelease` |
| `AAuraPlayerController::AbilityInputTagHeld/Released` | `Source/Aura/Private/Player/AuraPlayerController.cpp` | 非左键输入会转发给 ASC |
| `AAuraPlayerController::Move` | `Source/Aura/Private/Player/AuraPlayerController.cpp` | 可在这里检测“尝试移动”，用于取消引导技能 |
| `UAuraAttributeSet::PostGameplayEffectExecute` | `Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp` | 瞬间 GE 修改 Health 后会 clamp |
| `UOverlayWidgetController` 属性监听 | `Source/Aura/Private/UI/WidgetContronller/OverlayWidgetController.cpp` | Health 变化会广播到 UI |

## 数据模型扩展

### Targeting

文件：`Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`

建议让 `EAuraSkillTargetingMode` 支持：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillTargetingMode : uint8
{
    Self,
    CursorHit
};
```

| 值 | 用途 |
| --- | --- |
| `Self` | 目标就是施法者自己，不创建鼠标 TargetData |
| `CursorHit` | 火球术现有鼠标目标路径 |

### Delivery

建议让 `EAuraSkillDeliveryMode` 支持：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillDeliveryMode : uint8
{
    Direct,
    Projectile
};
```

| 值 | 用途 |
| --- | --- |
| `Direct` | 直接对目标 ASC 应用 GE |
| `Projectile` | 生成投射物，命中后应用 GE |

### Input / Channel

生命涌动需要表达“按住引导，松开结算”：

```cpp
UENUM(BlueprintType)
enum class EAuraSkillInputMode : uint8
{
    Press,
    HoldRelease
};
```

建议在 `UAuraSkillDefinition` 中新增：

```cpp
UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
EAuraSkillInputMode InputMode = EAuraSkillInputMode::Press;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
float FullChargeDuration = 0.f;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
float PartialChargeScale = 0.5f;

UPROPERTY(EditDefaultsOnly, Category = "Input|Channel")
bool bCancelOnMoveInputDuringChannel = false;
```

生命涌动建议配置：

| 字段 | 值 |
| --- | --- |
| `InputMode` | `HoldRelease` |
| `FullChargeDuration` | `2.0` |
| `PartialChargeScale` | `0.5` |
| `bCancelOnMoveInputDuringChannel` | `true` |

### EffectEntry

本轮不再新增只服务治疗的临时字段，而是引入通用效果条目：

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
UPROPERTY(EditDefaultsOnly, Category = "Effect")
TArray<FAuraSkillEffectEntry> EffectEntries;
```

设计理由：

1. 完整引导和未完整引导不是同一个效果的简单缩放：一个是瞬间治疗，一个是持续恢复。
2. 因此生命涌动应该配置两个 `EffectEntry`，用 `Condition` 决定触发哪个。
3. 这种结构后续可以自然扩展到 Buff、护盾、免疫、持续伤害、召唤物生成后的附加效果。

## 治疗 GE 设计

生命涌动需要两个 GE。

### 完整引导：瞬间治疗

建议新建：

```text
Content/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_InstantHeal.uasset
```

建议配置：

| 配置项 | 建议 |
| --- | --- |
| Duration Policy | `Instant` |
| Modifier Attribute | `Health` |
| Modifier Op | `Add` |
| Magnitude | `SetByCaller` |
| SetByCaller Tag | `SetByCaller.Healing` |

占位数值：

```text
SetByCaller.Healing = 100
```

### 未完整引导：持续治疗

建议新建：

```text
Content/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_HealOverTime.uasset
```

建议配置：

| 配置项 | 建议 |
| --- | --- |
| Duration Policy | `HasDuration` |
| Duration | `4.0` 秒 |
| Period | `1.0` 秒 |
| Modifier Attribute | `Health` |
| Modifier Op | `Add` |
| Magnitude | `SetByCaller` |
| SetByCaller Tag | `SetByCaller.HealingPerTick` |

占位数值：

```text
完整治疗 = 100
未完整治疗目标 = 50
持续时间 = 4 秒
每 1 秒跳一次
DA 中 SetByCaller.HealingPerTick = 25
运行时实际每跳 = 25 * PartialChargeScale(0.5) = 12.5
```

需要新增 GameplayTag：

```text
SetByCaller.Healing
SetByCaller.HealingPerTick
```

## 预期配置资产

建议新建：

```text
/Game/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge
```

建议字段：

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

注意：`PartialCharge` 条目的 SetByCaller 写未缩放基准值。运行时会乘 `PartialChargeScale = 0.5`，所以实际每跳是 `12.5`，4 秒总共恢复 `50`。

建议新增 GameplayTags：

```text
Ability.Vital.VitalSurge
SetByCaller.Healing
SetByCaller.HealingPerTick
State.Skill.Channeling
```

## 运行链路预设计

### 1. 技能授予

`BP_AuraCharacter.StartupSkillDefinitions` 加入 `DA_VitalSurge`。

运行时：

```text
AAuraCharacterBase::AddCharacterAbilities
-> UAuraAbilitySystemComponent::AddCharacterSkillDefinitions
-> GiveAbility(FGameplayAbilitySpec)
```

关键要求：

1. `AbilitySpec.SourceObject = DA_VitalSurge`。
2. `AbilitySpec.DynamicAbilityTags` 包含 `InputTag.2`。
3. 不要和火球术使用同一个输入 Tag。

### 2. 输入触发

当前输入链路：

```text
AAuraPlayerController::AbilityInputTagHeld(InputTag.2)
-> UAuraAbilitySystemComponent::AbilityInputTagHeld(InputTag.2)
-> AbilitySpecInputPressed(AbilitySpec)
-> TryActivateAbility(SpecHandle)

AAuraPlayerController::AbilityInputTagReleased(InputTag.2)
-> UAuraAbilitySystemComponent::AbilityInputTagReleased(InputTag.2)
-> AbilitySpecInputReleased(AbilitySpec)
```

生命涌动建议在 `UAuraConfiguredActiveAbility` 内使用 `UAbilityTask_WaitInputRelease`：

```text
按住 2
-> 激活 Ability
-> 记录 ChannelStartTime
-> 添加 State.Skill.Channeling
-> WaitInputRelease

松开 2
-> ASC 调用 AbilitySpecInputReleased
-> WaitInputRelease 回调
-> 计算 HeldDuration
-> HeldDuration >= 2 秒：FullCharge
-> HeldDuration < 2 秒：PartialCharge
-> 服务端应用匹配 Condition 的 EffectEntry
```

### 3. Ability 激活

建议 `ActivateAbility` 分支：

```text
ActivateAbility
-> 读取 CachedSkillDef
-> CommitAbility
-> if InputMode == HoldRelease:
      StartHoldReleaseFlow()
   else:
      StartPressFlow()
```

`StartHoldReleaseFlow` 做：

1. 校验 `TargetingMode == Self`。
2. 校验 `DeliveryMode == Direct`。
3. 记录 `ChannelStartTime`。
4. 标记当前正在引导，例如添加 `State.Skill.Channeling`。
5. 创建 `UAbilityTask_WaitInputRelease`。
6. 等待释放、取消或移动输入打断。

### 4. 移动取消

用户确认的是“一旦尝试移动就取消技能”，不是简单地禁止角色位移。

建议 V1-B 做法：

```text
AAuraPlayerController::Move
-> 检测 MoveAction 输入值是否超过很小阈值
-> 如果本地角色 ASC 拥有 State.Skill.Channeling
      调用 ASC 取消带有引导/当前技能 Tag 的 Ability
      return，不继续 AddMovementInput
-> 否则正常移动
```

这里的重点是“尝试移动”发生在输入层，所以比释放时检查速度更符合需求。速度检查只能知道角色有没有动起来，不能准确知道玩家有没有尝试移动。

实现注意：

1. 推荐用 GameplayTag 表示引导状态，例如 `State.Skill.Channeling`。
2. `State.Skill.Channeling` 如果用 loose tag，多人表现需要注意复制；本轮最关键的是本地输入能取消，服务端最终结算正确。
3. Ability 被取消后必须走 `EndAbility` 清理路径。
4. `EndAbility` 中必须移除引导状态，避免角色之后无法正常释放技能或移动。
5. 不建议第一版直接 `DisableMovement()`，因为异常结束时更容易把角色卡住。

长期更稳的方式是 GameplayTag 驱动输入权限：移动层只关心“当前是否被某个状态阻止移动/需要取消引导”，不需要知道具体技能名。

### 5. 释放结算

释放时：

```text
HeldDuration = Now - ChannelStartTime

if HeldDuration >= FullChargeDuration:
    ChargeResult = FullCharge
else:
    ChargeResult = PartialCharge
```

当前规则不是线性蓄力，只分两档：

| 条件 | 结果 |
| --- | --- |
| 按住时间 >= 2 秒 | 完整引导，瞬间治疗 100 |
| 按住时间 < 2 秒 | 未完整引导，持续治疗总量 50 |
| 引导期间尝试移动 | 取消技能，不治疗 |

### 6. 服务端应用 EffectEntry

建议新增内部函数：

```cpp
void ApplyEffectEntriesToSelf(EAuraSkillEffectCondition ChargeResult);
```

链路：

```text
ApplyEffectEntriesToSelf(ChargeResult)
-> SourceASC = GetAbilitySystemComponent(Avatar)
-> For Each EffectEntry:
      如果 Entry.Condition 不是 Always 且不等于 ChargeResult，则跳过
      EffectContext = SourceASC->MakeEffectContext()
      EffectContext.SetAbility(this)
      EffectContext.AddSourceObject(CachedSkillDef)
      SpecHandle = SourceASC->MakeOutgoingSpec(EffectEntry.EffectClass, GetAbilityLevel(), EffectContext)
      For Each SetByCaller:
          Value = ScalableFloat.GetValueAtLevel(AbilityLevel)
          AssignTagSetByCallerMagnitude(SpecHandle, Tag, Value)
      SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get())
```

为什么必须由服务端结算：

1. 治疗会修改 `Health`，这是 Gameplay 结果。
2. 客户端不能自己决定回血，否则多人下会不可信。
3. 服务端应用 GE 后，Health 属性通过复制同步到客户端。

### 7. AttributeSet 和 UI

完整引导：

```text
GE_VitalSurge_InstantHeal
-> Instant GE 修改 Health
-> UAuraAttributeSet::PostGameplayEffectExecute
-> Clamp 到 0..MaxHealth
-> Health 复制 / 委托广播
-> UI 刷新
```

未完整引导：

```text
GE_VitalSurge_HealOverTime
-> 每个 Period 修改 Health
-> 每次跳治疗都经过 AttributeSet clamp / UI 刷新
```

本轮不新增 `IncomingHealing`。

理由：

1. 当前只是普通自疗，不涉及治疗暴击、治疗加成、治疗减免、禁疗。
2. 直接改 `Health` 足够验证持续治疗链路。
3. 后续做吸血、治疗统计、禁疗、治疗暴击时，再引入 `IncomingHealing` 和 `ExecCalc_Healing`。

## Montage / Notify 处理原则

本技能不使用 Montage。

但对于整个技能系统来说，你的理解基本正确：如果一个技能的 gameplay 时机必须和动画某一帧绑定，那么在 Montage 里放 AnimNotify 或 GameplayEvent Notify，是 UE/GAS 项目里非常常见、也很合理的做法。

关键区别是：

1. “哪一帧触发”通常应该在 Montage 时间轴里由人配置，因为这和动画资产本身强相关。
2. 技能配置不应该只写一个 `CastMontage`，还应该写“等待哪个事件 Tag”。
3. 运行时代码不应该猜测某个技能应该在 Montage 的第几帧生效。

推荐配置颗粒度：

| 配置项 | 作用 |
| --- | --- |
| `CastMontage` | 播放哪个 Montage |
| `MontageEventTag` | 等待哪个 Notify / GameplayEvent |
| `MontageEventPolicy` | Notify 丢失时是取消、报错、还是 fallback |
| `EffectApplicationTiming` | Immediate、InputRelease、MontageEvent、ProjectileHit、PeriodicTick |

因此更准确的说法是：

```text
Montage 负责“动画时间轴上哪一帧发事件”；
SkillDefinition 负责“这个技能要播放哪个 Montage、等待哪个事件、事件来了以后做什么”。
```

未来技能编辑器可以做得更舒服：选择 Montage 后，编辑器扫描并提示里面有哪些 Notify Tag，甚至检查“这个技能要求的 Tag 是否存在”。但第一版不需要自动改 Montage，先手动配置 Notify 是合理路线。

## 需要新增或修改的代码点

### `UAuraSkillDefinition`

文件：

```text
Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h
```

建议修改：

1. `EAuraSkillTargetingMode` 增加 `Self`。
2. `EAuraSkillDeliveryMode` 增加 `Direct`。
3. 新增 `EAuraSkillInputMode`。
4. 新增 `FullChargeDuration`、`PartialChargeScale`、`bCancelOnMoveInputDuringChannel`。
5. 新增 `EAuraSkillEffectTarget`、`EAuraSkillEffectCondition`、`FAuraSkillEffectEntry`。
6. 新增 `TArray<FAuraSkillEffectEntry> EffectEntries`。
7. 保留旧的 `DamageEffectClass` / `DamageTypes`，火球术暂时不迁移，避免破坏已验证路径。

### `UAuraConfiguredActiveAbility`

文件：

```text
Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h
Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp
```

建议修改：

1. `CanActivateAbility` 根据 `DeliveryMode` 分支校验：
   - `Projectile`：继续校验 `ProjectileClass` 和旧 `DamageEffectClass`。
   - `Direct`：校验 `EffectEntries` 至少有一个有效 `EffectClass`。
2. `ActivateAbility` 根据 `InputMode` 分支：
   - `Press`：走火球术当前路径。
   - `HoldRelease`：进入引导释放路径。
3. 新增 `StartHoldReleaseFlow`。
4. 新增 `OnHoldInputReleased`。
5. 新增 `ApplyEffectEntriesToSelf(EAuraSkillEffectCondition ChargeResult)`。
6. 新增防重复应用标记，例如 `bEffectEntriesApplied`。
7. `EndAbility` 中清理引导状态，保证取消、失败、正常结束都走同一处兜底。

### `AAuraPlayerController`

文件：

```text
Source/Aura/Private/Player/AuraPlayerController.cpp
```

建议修改：

1. 在 `Move` 中检测是否有有效移动输入。
2. 如果当前 ASC 拥有 `State.Skill.Channeling`，取消引导技能。
3. 取消后不继续调用 `AddMovementInput`。
4. 这段代码需要中文注释解释：为什么移动取消放在输入层，而不是只检查速度。

### `UAuraAbilitySystemComponent`

当前已有 `AbilityInputTagReleased`，但实现前要复查：

1. `AbilitySpecInputReleased` 是否能稳定驱动 `UAbilityTask_WaitInputRelease`。
2. `Held` 是否每帧重复调用 `AbilitySpecInputPressed`，如果会影响 `WaitInputRelease`，需要加保护。
3. 输入链路需要补中文注释，解释 `Pressed/Held/Released` 和 GAS InputPressed 状态的关系。

### GameplayTags

建议新增：

```ini
+GameplayTagList=(Tag="Ability.Vital.VitalSurge",DevComment="Hold-release self heal configured skill prototype")
+GameplayTagList=(Tag="SetByCaller.Healing",DevComment="SetByCaller magnitude for instant healing")
+GameplayTagList=(Tag="SetByCaller.HealingPerTick",DevComment="SetByCaller magnitude for periodic healing per tick")
+GameplayTagList=(Tag="State.Skill.Channeling",DevComment="Actor is channeling a skill")
```

### UE 资产

建议创建：

```text
Content/Blueprints/AbilitySystem/SkillDefinitions/DA_VitalSurge.uasset
Content/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_InstantHeal.uasset
Content/Blueprints/AbilitySystem/GameplayEffects/GE_VitalSurge_HealOverTime.uasset
```

## 验证标准

### 单人 PIE

1. 按住 `2` 不到 2 秒后释放，出现总量 50 的持续治疗。
2. 按住 `2` 超过 2 秒后释放，立刻治疗 100。
3. 治疗不超过 `MaxHealth`。
4. UI 血球/血条正确刷新。
5. 引导期间尝试移动会取消技能，不产生治疗。
6. 技能取消或结束后，角色移动恢复正常。
7. 技能没有卡住，能再次释放。

### Listen Server

1. Host 短按/长按都能按规则治疗。
2. Client 短按/长按都能按规则治疗。
3. 治疗只由服务端结算。
4. 一次释放不会应用两次 GE。
5. 其他端能看到该同步的 Health 状态变化。
6. 引导取消或中断后，移动与再次释放都恢复正常。

### 日志关注点

1. Direct 技能不应等待 TargetData。
2. Direct 技能不应生成投射物。
3. `EffectEntries` 为空时应拒绝激活或结束 Ability。
4. GE Spec 创建失败时应有明确日志，并结束 Ability。
5. 移动取消必须进入 `EndAbility` 清理路径。

## 给 ClaudeCode 的实现提示方向

等用户确认本预设计后，再生成正式 Prompt。Prompt 应要求 ClaudeCode：

1. 先阅读本文件、火球术运行链路文档、风险清单。
2. 不破坏火球术当前稳定路径。
3. 新增 `Self`、`Direct`、`HoldRelease`、`EffectEntry`。
4. 使用 `WaitInputRelease` 或等价 GAS 机制处理松开结算。
5. 引导满 2 秒时应用瞬间治疗 GE。
6. 引导不足 2 秒时应用持续治疗 GE。
7. 引导期间尝试移动时取消技能，不治疗。
8. 治疗只能由服务端结算。
9. 新代码写中文学习注释，重点解释输入释放、服务端结算、GE 应用、移动取消清理。
10. 创建或指导创建 `DA_VitalSurge`、`GE_VitalSurge_InstantHeal`、`GE_VitalSurge_HealOverTime`。
11. 编译并写中文报告。

## 当前无阻塞确认点

本轮用户已经确认：

1. 移动尝试取消技能。
2. 完整引导瞬间治疗。
3. 未完整引导固定半量，且持续恢复。
4. 数值先用占位。
5. 本技能不使用 Montage。

下一步可以进入正式 ClaudeCode 实现 Prompt。
