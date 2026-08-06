# 当前代码设计

更新日期：2026-08-06

## 当前任务

Phase 1B：建立 AbilitySet、项目 GA 基类、Native GameplayTag、Enhanced Input 到 AbilitySpec 的输入路由，以及最小按下/保持/松开 Ability 闭环。

本设计已经由用户批准。实施不得扩大到正式技能、武器、Cost、Cooldown、伤害、死亡、重生、Tag Relationship Mapping 或 UE 资产批量操作。

## 本阶段运行链路

```text
Q
-> IA_Ability_Tactical
-> IMC_Apecox_Gameplay
-> UApecoxInputConfig: IA -> InputTag.Ability.Tactical
-> AApecoxPlayerCharacter 输入回调
-> UApecoxAbilitySystemComponent 缓存 Pressed/Held/Released SpecHandle
-> AApecoxPlayerController::PostProcessInput()
-> UApecoxAbilitySystemComponent::ProcessAbilityInput()
-> 根据 UApecoxGameplayAbility::ActivationPolicy 激活 AbilitySpec
-> GA 内 UAbilityTask_WaitInputRelease 等待 Generic Replicated Event
-> 松键事件到达本地和服务器任务
-> GA 主动 EndAbility()
```

职责分离：

- Character/Enhanced Input 只把本地输入意图翻译成 InputTag。
- ASC 负责 InputTag 到 AbilitySpec、逐帧输入缓存和激活尝试。
- GA 基类声明激活策略与并发组，不读取物理按键。
- AbilityTask 负责 GA 激活后的异步等待和网络事件关联。
- PlayerController 只选择每帧统一处理输入的时机，不实现具体技能逻辑。

## 新增 Native GameplayTag

文件：

- `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h`
- `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp`

使用命名空间和 UE Native Gameplay Tags 宏，不创建 Tag 单例。

| C++ 名称 | Tag 字符串 | 作用 |
| --- | --- | --- |
| `ApecoxGameplayTags::InputTag_Ability_Tactical` | `InputTag.Ability.Tactical` | 英雄战术/小技能输入槽位或意图；不是具体技能身份 |
| `ApecoxGameplayTags::State_Input_AbilityBlocked` | `State.Input.AbilityBlocked` | 阻止玩家通过输入激活 Ability；不影响服务器事件或 OnAvatarSet 被动能力 |

本阶段不创建 Ultimate、Fire、ADS、Reload、技能身份或测试专用 Tag。

## UApecoxGameplayAbility

文件：

- `Source/Apecox/Public/AbilitySystem/Abilities/ApecoxGameplayAbility.h`
- `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp`

父类：`UGameplayAbility`。

类声明：`Abstract`、`Blueprintable`。它是所有 Apecox GameplayAbility 的项目基类，不是万能技能模板。

### 枚举位置

下列两个 `UENUM(BlueprintType)` 放在 `ApecoxGameplayAbility.h` 的模块级作用域，位于类声明之前：

```text
EApecoxAbilityActivationPolicy
EApecoxAbilityActivationGroup
```

不把枚举嵌套进类，也不放进 ASC：

- GA 蓝图需要直接编辑它们。
- ASC 需要读取它们完成输入和并发调度。
- 它们描述 Ability 语义，归属 GA 领域；ASC 只是执行者。

### EApecoxAbilityActivationPolicy

| 枚举值 | 语义 |
| --- | --- |
| `OnInputTriggered` | 本帧收到 Pressed 时尝试激活一次 |
| `WhileInputActive` | InputTag 仍处于 Held 时，每帧对未激活 Spec 尝试激活；适合自动射击等“结束后继续按住可再次启动”的行为 |
| `OnAvatarSet` | 新 Pawn Avatar 绑定完成后按网络执行策略尝试自动激活；比笼统的 OnSpawn 更符合 PlayerState ASC |

默认值：`OnInputTriggered`。

### EApecoxAbilityActivationGroup

| 枚举值 | 语义 |
| --- | --- |
| `Independent` | 可与其他组并行，不参与排他替换 |
| `ExclusiveReplaceable` | 排他运行，但可被新的排他 Ability 取消并替换 |
| `ExclusiveBlocking` | 排他运行，并阻止新的排他 Ability 激活 |
| `MAX` | 隐藏哨兵值，只用于固定计数数组 |

默认值：`Independent`。

并发组是封闭规则，使用枚举，不为每个特殊技能增加 GameplayTag。Tag Relationship Mapping 以后表达跨状态/跨类别关系，两者不重合。

### 默认 GAS 策略

构造函数默认：

- `ReplicationPolicy = ReplicateNo`
- `InstancingPolicy = InstancedPerActor`
- `NetExecutionPolicy = LocalPredicted`
- `NetSecurityPolicy = ClientOrServer`
- ActivationPolicy = OnInputTriggered
- ActivationGroup = Independent

具体被动、服务器专用或特殊执行 Ability 可以在派生 C++/Blueprint 默认值中覆盖网络策略。

### 公开/受保护 API

公开：

- `GetActivationPolicy()`
- `GetActivationGroup()`
- `GetApecoxAbilitySystemComponentFromActorInfo()`
- `GetApecoxPlayerControllerFromActorInfo()`
- `GetApecoxPlayerCharacterFromActorInfo()`
- `CanChangeActivationGroup()`
- `ChangeActivationGroup()`

受保护：

- `TryActivateAbilityOnAvatarSet(...)`
- `OnPawnAvatarSet()` 与 Blueprint 事件 `K2_OnPawnAvatarSet`
- 覆写 `OnGiveAbility`，用于处理“先授予后绑定 Avatar”和“已有 Avatar 后授予”两种时序。
- 覆写 `CanActivateAbility`，在 Super 通过后查询 ASC 并发组是否阻止。
- 覆写 `SetCanBeCanceled`，禁止 `ExclusiveReplaceable` 被设置成不可取消。

## UApecoxAbilitySet

文件：

- `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySet.h`
- `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySet.cpp`

父类：`UPrimaryDataAsset`，声明 `BlueprintType, Const`。

AbilitySet 是不可变、可撤销的授予包，不是函数、单技能定义或运行时状态容器。谁调用授予并保存 Handles，谁决定这批能力的生命周期。

### 数据结构

- `FApecoxAbilitySetAbility`
  - `TSubclassOf<UApecoxGameplayAbility> Ability`
  - `int32 AbilityLevel = 1`
  - `FGameplayTag InputTag`
- `FApecoxAbilitySetGameplayEffect`
  - `TSubclassOf<UGameplayEffect> GameplayEffect`
  - `float EffectLevel = 1.0f`
- `FApecoxAbilitySetAttributeSet`
  - `TSubclassOf<UAttributeSet> AttributeSet`
- `FApecoxAbilitySetGrantedHandles`
  - Ability SpecHandle 数组
  - Active GE Handle 数组
  - 动态 AttributeSet 指针数组
  - `RemoveFromAbilitySystem(UApecoxAbilitySystemComponent*)`

`UApecoxAbilitySet`：

- `GrantToAbilitySystem(ASC, OutGrantedHandles, SourceObject)`
- `GrantedAbilities`
- `GrantedGameplayEffects`
- `GrantedAttributeSets`

### 安全约束

- Grant/Remove 只在 Authority 执行；客户端调用安全返回。
- AbilitySpec 的 `SourceObject` 使用调用方传入值。
- InputTag 写入 `AbilitySpec.GetDynamicSpecSourceTags()`。
- GE 的 EffectContext 记录 SourceObject。
- AbilitySet 中的 GE 必须能够产生可撤销 ActiveGameplayEffectHandle；Instant GE 不得作为可撤销授予项静默通过。初始化瞬时数值以后使用独立初始化入口。
- 动态 AttributeSet 以 ASC Owner（PlayerState）为 Outer，并使用 UE 5.8 `AddAttributeSetSubobject` / `RemoveSpawnedAttribute`。
- Remove 顺序必须清 Ability、移除 Active GE、移除动态 AttributeSet，最后清空 Handles，且允许重复调用。

## UApecoxInputConfig

文件：

- `Source/Apecox/Public/Input/ApecoxInputConfig.h`
- `Source/Apecox/Private/Input/ApecoxInputConfig.cpp`

父类：`UDataAsset`。

`FApecoxInputAction`：

- `TObjectPtr<const UInputAction> InputAction`
- `FGameplayTag InputTag`

配置数组：

- `NativeInputActions`：移动、观察、交互等直接调用本地函数的输入。
- `AbilityInputActions`：通过 ASC 路由到 AbilitySpec 的输入。

查询函数使用 Exact Tag 匹配：

- `FindNativeInputActionForTag`
- `FindAbilityInputActionForTag`

本阶段只实际配置 Tactical Ability Input；Native 数组预留真实结构但不创建未使用 Tag。

## UApecoxInputComponent

文件：

- `Source/Apecox/Public/Input/ApecoxInputComponent.h`
- `Source/Apecox/Private/Input/ApecoxInputComponent.cpp`

父类：`UEnhancedInputComponent`。

职责：

- 模板函数 `BindNativeAction`。
- 模板函数 `BindAbilityActions`。
- `RemoveBinds`。
- Ability 输入使用 `Triggered` 作为 Pressed，并同时把 `Completed`、`Canceled` 绑定为 Released。
- `IA_Ability_Tactical` 必须配置 one-shot Pressed Trigger，避免默认 Down Trigger 每帧产生 Pressed。
- 绑定 Handle 由 Character 保存，便于重建输入组件时解除。

`Config/DefaultInput.ini`：

```ini
DefaultInputComponentClass=/Script/Apecox.ApecoxInputComponent
```

保留 `EnhancedPlayerInput`。

## UApecoxAbilitySystemComponent

现有类扩展以下职责。

### 输入接口

公开：

- `AbilityInputTagPressed(const FGameplayTag& InputTag)`
- `AbilityInputTagReleased(const FGameplayTag& InputTag)`
- `ProcessAbilityInput(float DeltaTime, bool bGamePaused)`
- `ClearAbilityInput()`

内部缓存：

- `InputPressedSpecHandles`：本帧刚触发。
- `InputHeldSpecHandles`：仍处于按住状态，跨帧保留。
- `InputReleasedSpecHandles`：本帧刚释放。

处理顺序：

1. 若拥有 `State.Input.AbilityBlocked`，清空三组输入并返回。
2. 扫描 Held，为 `WhileInputActive` 且未激活的 Spec 收集激活请求。
3. 扫描 Pressed，设置 `Spec.InputPressed=true`：
   - 已激活：调用 `AbilitySpecInputPressed`，向正在等待的任务发送事件。
   - 未激活且策略为 `OnInputTriggered`：收集激活请求。
4. 统一对去重后的 Handle 调用 `TryActivateAbility`。
5. 扫描 Released，设置 `Spec.InputPressed=false`；若仍激活，调用 `AbilitySpecInputReleased`。
6. 清空 Pressed/Released，Held 保留到真正松键。

先收集后统一激活，避免同一帧 Held 与 Pressed 导致重复激活或先激活后错误收到第二次 Pressed。

### Generic Replicated Event

覆写：

- `AbilitySpecInputPressed`
- `AbilitySpecInputReleased`

先调用 Super。若 Spec 活跃，使用该 Ability 实例当前 ActivationPredictionKey，调用：

```text
InvokeReplicatedEvent(InputPressed/InputReleased, SpecHandle, PredictionKey)
```

不启用 `bReplicateInputDirectly`。WaitInputPress/WaitInputRelease 通过 GAS Generic Replicated Event Delegate 监听相同的 EventType + SpecHandle + PredictionKey。

### Avatar 自动激活

覆写 `InitAbilityActorInfo`：

- 检测是否获得新的 Pawn Avatar。
- 调用 Super 后通知已有 `UApecoxGameplayAbility` 实例 `OnPawnAvatarSet`。
- 调用 `TryActivateAbilitiesOnAvatarSet`。
- Avatar 改变时清理旧输入缓存，避免 Held Handle 泄漏到新 Pawn。

### 并发组

公开/受控函数：

- `IsActivationGroupBlocked`
- `AddAbilityToActivationGroup`
- `RemoveAbilityFromActivationGroup`
- `CancelActivationGroupAbilities`
- 内部 `CancelAbilitiesByFunc`

覆写：

- `NotifyAbilityActivated`：加入并发组；新的排他 Ability 取消旧的 Replaceable。
- `NotifyAbilityEnded`：移出并发组。
- 固定计数数组大小使用 `EApecoxAbilityActivationGroup::MAX`。
- Independent 永不被组机制阻止；Blocking 存在时新的 Replaceable/Blocking 都不能激活。
- 计数溢出、下溢和同时运行多个排他 Ability 使用 check/ensure 暴露。

## AApecoxPlayerController::PostProcessInput

新增覆写：

```cpp
virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;
```

每帧职责仅有：

1. 从当前 PlayerState 获取 `UApecoxAbilitySystemComponent`。
2. 调用 `ProcessAbilityInput(DeltaTime, bGamePaused)`。
3. 调用 `Super::PostProcessInput`。

它不负责：

- 判断 Q 对应哪个技能。
- 执行 GA 逻辑。
- 生成网络 RPC、伤害或表现。
- 保存 Pressed/Held/Released 数据。

选择 PostProcessInput 是因为 Enhanced Input 已在本帧完成所有 Action 回调，ASC 可以看到完整输入快照；若在每个 IA 回调里立即激活，会让同帧组合键、按下/松开和多个 Spec 的处理顺序分散。

## AApecoxPlayerCharacter 扩展

新增配置：

- `InputConfig : TObjectPtr<const UApecoxInputConfig>`
- `DefaultInputMappingContext : TObjectPtr<const UInputMappingContext>`
- `DefaultInputMappingPriority : int32`
- `PawnAbilitySets : TArray<TObjectPtr<const UApecoxAbilitySet>>`

新增运行时状态：

- `AbilityInputBindingHandles : TArray<uint32>`
- `GrantedPawnAbilitySetHandles : TArray<FApecoxAbilitySetGrantedHandles>`

新增函数：

- `SetupPlayerInputComponent`
- `HandleAbilityInputTagPressed`
- `HandleAbilityInputTagReleased`
- `GrantPawnAbilitySets`
- `RemovePawnAbilitySets`

规则：

- 仅本地控制 Pawn 安装 IMC 和绑定 IA。
- 服务器在 ActorInfo 初始化完成后授予 PawnAbilitySets。
- 同一 Pawn 重复 Init 不得重复授予。
- Uninitialize 时先 ClearAbilityInput、CancelAllAbilities、移除 Pawn AbilitySet，再解绑 Avatar。
- Pawn AbilitySet 的 SourceObject 使用当前 Character。
- PawnAbilitySets 只表示与当前 Pawn 生命周期绑定的授予包；未来英雄玩家级、装备级 AbilitySet 由对应所有者保存自己的 Handles。

## UE 测试资产（代码审查后人工创建）

全部位于 `/Game/Blueprints`：

- `/Game/Blueprints/Input/Actions/IA_Ability_Tactical`
- `/Game/Blueprints/Input/IMC_Apecox_Gameplay`
- `/Game/Blueprints/Input/DA_Apecox_InputConfig`
- `/Game/Blueprints/AbilitySystem/Abilities/GA_Phase1B_InputLifecycle`
- `/Game/Blueprints/AbilitySystem/AbilitySets/AS_Phase1B_InputLifecycle`
- `/Game/Blueprints/Character/BP_ApecoxPlayerCharacter`
- `/Game/Blueprints/Game/BP_ApecoxGameMode`（用于引用项目自有 Pawn；是否需要 BP Controller 由代码字段归属决定，本设计不要求）

测试 GA：

- 继承 `UApecoxGameplayAbility`。
- ActivationPolicy = OnInputTriggered。
- ActivationGroup = Independent。
- 激活后创建 `Wait Input Release`。
- Released 输出打印带 NetMode/LocalRole 的诊断信息并调用 EndAbility。
- 不添加 Cost、Cooldown、GE、Cue、Montage 或临时 Tag。

## 本批明确不做

- 正式英雄技能、开火、ADS、换弹或武器 Ability。
- Cost、Cooldown、Health 初始化、伤害和 UI。
- Tag Relationship Mapping 或 Ability 类别 Tag。
- Death/Respawn、Dedicated Server 最终验收。
- SkillDefinition、CombatEntity、技能编辑器。
- MCP、自动创建 UE 资产、生成 Rider/Visual Studio 项目文件。
