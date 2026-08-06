# ClaudeCode 实施 Prompt：Phase 1B AbilitySet 与输入生命周期

更新日期：2026-08-06

## 一、你的角色

你是 Apecox 项目的受约束实施子代理，负责按照已批准设计编写 C++、修改明确允许的 Config、执行编译并提交中文报告。

- 用户负责方向和公开命名确认。
- Codex 负责架构、Prompt、代码审查、UE 人工验证清单和 Git 收口。
- 你不得重新设计已批准 API、扩大范围、使用 MCP 操作 UE 编辑器、创建 UE 资产或执行 Git add/commit/push。

这是 Phase 1B。Phase 1A 已通过完整构建、单人 PIE 与两人 Listen Server，并形成提交：

```text
86ed380 gas: establish player ASC lifecycle baseline
```

该提交尚未 push。保留当前全部内容，不得 reset、restore、checkout、clean 或覆盖用户/Codex 改动。

## 二、开始前必须阅读

按顺序阅读：

1. `D:/UnrealProject/Apecox/.agents/ue-project-context.md`
2. `D:/UnrealProject/Apecox/Agent/00_Coordination/Working_Agreement.md`
3. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Phase.md`
4. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md`
5. 当前下列源码：
   - `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
   - `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`
   - `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
   - `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
   - `Source/Apecox/Public/Player/ApecoxPlayerController.h`
   - `Source/Apecox/Private/Player/ApecoxPlayerController.cpp`
   - `Source/Apecox/Public/Player/ApecoxPlayerState.h`
   - `Config/DefaultInput.ini`

先运行 `git status --short`，但不得做任何 Git 修改操作。

## 三、唯一实施范围

### 新增文件

严格遵守 Public/Private 对称目录：

1. `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h`
2. `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp`
3. `Source/Apecox/Public/AbilitySystem/Abilities/ApecoxGameplayAbility.h`
4. `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp`
5. `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySet.h`
6. `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySet.cpp`
7. `Source/Apecox/Public/Input/ApecoxInputConfig.h`
8. `Source/Apecox/Private/Input/ApecoxInputConfig.cpp`
9. `Source/Apecox/Public/Input/ApecoxInputComponent.h`
10. `Source/Apecox/Private/Input/ApecoxInputComponent.cpp`

### 修改文件

1. `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
2. `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`
3. `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
4. `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
5. `Source/Apecox/Public/Player/ApecoxPlayerController.h`
6. `Source/Apecox/Private/Player/ApecoxPlayerController.cpp`
7. `Config/DefaultInput.ini`
8. 中文实施报告。

除编译生成的忽略文件外，不得修改其他源码、Config、Agent 协作文档、`.uproject` 或 UE 资产。

## 四、Native GameplayTag

在 `namespace ApecoxGameplayTags` 中使用 `UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG_COMMENT`，不创建单例：

| C++ 名称 | 字符串 |
| --- | --- |
| `InputTag_Ability_Tactical` | `InputTag.Ability.Tactical` |
| `State_Input_AbilityBlocked` | `State.Input.AbilityBlocked` |

Header 对声明应用 `APECOX_API`。不要新增其他 Tag，不修改 `DefaultGameplayTags.ini`。

## 五、UApecoxGameplayAbility

### 5.1 枚举

在 `ApecoxGameplayAbility.h` 模块级作用域、类声明之前定义：

```cpp
UENUM(BlueprintType)
enum class EApecoxAbilityActivationPolicy : uint8
{
    OnInputTriggered,
    WhileInputActive,
    OnAvatarSet
};

UENUM(BlueprintType)
enum class EApecoxAbilityActivationGroup : uint8
{
    Independent,
    ExclusiveReplaceable,
    ExclusiveBlocking,
    MAX UMETA(Hidden)
};
```

枚举值可以添加简洁 `UMETA(DisplayName=...)`，但不得改名或增加项目策略。

### 5.2 类

`UApecoxGameplayAbility : UGameplayAbility`：

- `UCLASS(Abstract, Blueprintable)`。
- 默认：
  - ReplicateNo
  - InstancedPerActor
  - LocalPredicted
  - ClientOrServer
  - ActivationPolicy = OnInputTriggered
  - ActivationGroup = Independent
- `ActivationPolicy`、`ActivationGroup` 为 `EditDefaultsOnly, BlueprintReadOnly`。
- 提供 Current_Code_Design 中批准的强类型 Getter。
- Helper Getter 必须做空指针检查，不使用不安全强转。
- 提供 `CanChangeActivationGroup`、`ChangeActivationGroup`。
- `CanActivateAbility` 先调用 Super；Super 失败立即返回 false；再询问 Apecox ASC 当前并发组是否阻止。
- `SetCanBeCanceled` 禁止 ExclusiveReplaceable 变成不可取消，使用 ensure/日志暴露错误并保持原状态。
- `OnGiveAbility` 调用 Super 后尝试 OnAvatarSet 自动激活。
- `TryActivateAbilityOnAvatarSet` 必须同时检查：
  - ActorInfo/ASC/Avatar 有效；
  - Spec 未激活；
  - 策略为 OnAvatarSet；
  - Avatar 未 TearOff、没有正的 LifeSpan 销毁倒计时；
  - LocalPredicted/LocalOnly 仅本地控制端尝试；
  - ServerOnly/ServerInitiated 仅 Authority 尝试。
- 提供 protected Native `OnPawnAvatarSet()`，调用 BlueprintImplementableEvent `K2_OnPawnAvatarSet`。

不要加入 Cost、Cooldown、CameraMode、FailureMessage、AdditionalCost 或 Tag Relationship Mapping。

## 六、UApecoxAbilitySet

实现 Current_Code_Design 中四个结构与一个资产类，命名必须准确：

- `FApecoxAbilitySetAbility`
- `FApecoxAbilitySetGameplayEffect`
- `FApecoxAbilitySetAttributeSet`
- `FApecoxAbilitySetGrantedHandles`
- `UApecoxAbilitySet : UPrimaryDataAsset`

要求：

1. `UApecoxAbilitySet` 为 `BlueprintType, Const`。
2. Ability class 限制为 `TSubclassOf<UApecoxGameplayAbility>`。
3. InputTag 的 metadata 使用 `Categories="InputTag"`。
4. `GrantToAbilitySystem` 及 `RemoveFromAbilitySystem` 只在 `IsOwnerActorAuthoritative()` 为真时执行。
5. 输入/类/等级无效时 ensure 并跳过，不崩溃。
6. Ability：
   - 从 Ability Class CDO 创建 Spec；
   - 设置 Level；
   - 设置 SourceObject；
   - 有效 InputTag 写入 `GetDynamicSpecSourceTags().AddTag()`；
   - GiveAbility；
   - 记录有效 SpecHandle。
7. GameplayEffect：
   - 创建 EffectContext；
   - SourceObject 有效时 `AddSourceObject`；
   - 从 CDO ApplyGameplayEffectToSelf；
   - 只接受可产生有效 ActiveGEHandle 的可撤销效果；
   - Instant GE 应 ensure/报错并跳过，不能静默产生不可撤销修改；
   - 记录有效 Handle。
8. AttributeSet：
   - 以 `ASC->GetOwner()`（PlayerState）为 Outer 创建；
   - 使用 UE 5.8 `AddAttributeSetSubobject`；
   - 记录指针。
9. Remove：
   - ClearAbility；
   - RemoveActiveGameplayEffect；
   - RemoveSpawnedAttribute；
   - Reset 三组记录。
10. 重复 Remove 必须安全。

不要在 AbilitySet 中加入 SkillDefinition、UI、蒙太奇、武器、输入资产、数值字段或万能配置。

## 七、输入配置与组件

### 7.1 UApecoxInputConfig

- `FApecoxInputAction`：`const UInputAction*` + `FGameplayTag`。
- `UApecoxInputConfig : UDataAsset`，BlueprintType、Const。
- `NativeInputActions`、`AbilityInputActions` 两个 EditDefaultsOnly 数组。
- `FindNativeInputActionForTag` 和 `FindAbilityInputActionForTag` 使用 `MatchesTagExact` 或等价 Exact 比较。
- 查询失败返回 nullptr；不要创建日志分类或使用 LogTemp 刷屏。

### 7.2 UApecoxInputComponent

- 父类 `UEnhancedInputComponent`。
- Header 中实现模板：
  - `BindNativeAction`
  - `BindAbilityActions`
- Ability Pressed：`ETriggerEvent::Triggered`。
- Ability Released：同时绑定 `ETriggerEvent::Completed` 和 `ETriggerEvent::Canceled`。
- 每次绑定把返回 Handle 放入外部 `TArray<uint32>&`。
- `RemoveBinds(TArray<uint32>&)` 使用 `RemoveBindingByHandle` 后清空数组。
- 空 InputConfig、InputAction、InputTag 安全跳过。

### 7.3 DefaultInput.ini

只把：

```ini
DefaultInputComponentClass=/Script/EnhancedInput.EnhancedInputComponent
```

改为：

```ini
DefaultInputComponentClass=/Script/Apecox.ApecoxInputComponent
```

保留 `DefaultPlayerInputClass=/Script/EnhancedInput.EnhancedPlayerInput` 和其他设置。

## 八、扩展 UApecoxAbilitySystemComponent

### 8.1 输入缓存

新增公开函数：

- `AbilityInputTagPressed`
- `AbilityInputTagReleased`
- `ProcessAbilityInput`
- `ClearAbilityInput`

新增三个 Handle 数组：

- `InputPressedSpecHandles`
- `InputHeldSpecHandles`
- `InputReleasedSpecHandles`

按 Current_Code_Design 的六步顺序实现。额外要求：

- InputTag 使用 Dynamic Spec Source Tags 的 Exact 匹配。
- 使用 AddUnique/Remove，避免重复 Handle。
- `AbilitiesToActivate` 使用函数局部数组，不使用跨 ASC 的 static 可变数组。
- Pressed/Released 每帧 Reset，Held 只在 Release/Clear 时移除。
- 若拥有 `State.Input.AbilityBlocked`，立即 Clear 并返回。
- 无效或已被撤销的 Handle 安全跳过。
- 不直接构造 RPC，不使用 `bReplicateInputDirectly`。

### 8.2 Generic Replicated Event

覆写 `AbilitySpecInputPressed/Released`：

1. 先 Super。
2. Spec Active 时取得 Primary Instance 的当前 ActivationPredictionKey；若无实例使用 Spec ActivationInfo。
3. 调用 `InvokeReplicatedEvent(InputPressed/InputReleased, Spec.Handle, OriginalPredictionKey)`。
4. 保留必要的 UE 5.8 deprecation pragma，解释原因。

不要在这里直接调用 `ServerSetReplicatedEvent`。WaitInputPress/Release 的 AbilityTask 在本地 delegate 回调中负责 prediction window、客户端到服务器事件发送与服务器消费。

### 8.3 OnAvatarSet

覆写 `InitAbilityActorInfo`：

- 调用 Super 前判断是否为与旧值不同的新 Pawn Avatar。
- 新 Avatar 时先清输入缓存，防止旧 Held 泄漏。
- Super 后遍历 AbilitySpec：
  - 所有项目 Ability 应为 Instanced；遇到其他类型安全跳过并 ensure。
  - 通知实例 `OnPawnAvatarSet`。
- 再调用 `TryActivateAbilitiesOnAvatarSet`，遍历 CDO 并调用基类 helper。
- 同时覆盖“Ability 先授予”和“Avatar 先绑定”的时序。

### 8.4 并发组

实现：

- `IsActivationGroupBlocked`
- `AddAbilityToActivationGroup`
- `RemoveAbilityFromActivationGroup`
- `CancelActivationGroupAbilities`
- 内部 `CancelAbilitiesByFunc`
- `NotifyAbilityActivated`
- `NotifyAbilityEnded`

要求：

- 固定计数数组初始化为零。
- Independent 永不阻止。
- Blocking 计数大于零时，Replaceable 和 Blocking 都被阻止。
- 新的 Replaceable/Blocking 激活后取消旧的 Replaceable，但不取消 Blocking。
- 只取消确实 Active、属于 Apecox GA、匹配谓词且 `CanBeCanceled()` 的实例。
- 使用 `ABILITYLIST_SCOPE_LOCK()` 防止迭代期间 Ability 列表失效。
- 激活/结束计数必须对称；使用 check/ensure 防止溢出、下溢和多个排他 Ability 同时存在。
- 不实现 Tag Relationship Mapping。

## 九、扩展 PlayerController

`AApecoxPlayerController` 覆写：

```cpp
virtual void PostProcessInput(float DeltaTime, bool bGamePaused) override;
```

实现：

1. `GetPlayerState<AApecoxPlayerState>()`。
2. 取得强类型 ASC。
3. ASC 有效时调用 `ProcessAbilityInput`。
4. 调用 Super。

不要在 Controller 绑定具体 IA、保存 AbilitySet、处理技能逻辑或新增 RPC。

## 十、扩展 PlayerCharacter

### 配置字段

EditDefaultsOnly：

- `InputConfig`
- `DefaultInputMappingContext`
- `DefaultInputMappingPriority = 0`
- `PawnAbilitySets`

Transient：

- `AbilityInputBindingHandles`
- `GrantedPawnAbilitySetHandles`

使用 Current_Code_Design 中批准的准确类型。

### SetupPlayerInputComponent

- 必须调用 Super。
- Cast 到 `UApecoxInputComponent`；失败 ensure 并返回。
- 解除旧 Binding Handles 后重新绑定。
- 仅对本地控制 Pawn：
  - 从 Controller -> LocalPlayer 获取 `UEnhancedInputLocalPlayerSubsystem`；
  - 添加有效 `DefaultInputMappingContext` 与 Priority；
  - 使用 InputConfig 绑定 Ability Actions。
- 回调只转发到当前缓存 ASC 的 `AbilityInputTagPressed/Released`。
- 不实现移动、观察、跳跃或相机。

### Pawn AbilitySet 生命周期

- `GrantPawnAbilitySets`：Authority-only；ActorInfo 初始化后调用；已有 Handles 时不得重复授予；每个 Set 使用 Character 作为 SourceObject。
- `RemovePawnAbilitySets`：只移除本 Character 保存的 Handles，重复调用安全。
- `InitializeAbilitySystem` 成功绑定并缓存 ASC 后，服务器授予。
- `UninitializeAbilitySystem`：
  - 如果当前 Avatar 仍是 this：ClearAbilityInput -> CancelAllAbilities -> RemovePawnAbilitySets -> 保留 Owner 并清 Avatar。
  - 如果 ASC 已绑定新 Avatar：不得 Clear 全局输入、CancelAll 或清新 Avatar；Authority 仍应只清理本 Character 自己保存的 AbilitySet Handles，避免旧 Spec 泄漏。
  - 最后清缓存。
- 保留 Phase 1A 的旧 Avatar 乱序防护和 PlayerState Owner 保护，不得回退或简化。

## 十一、注释与代码质量

- 所有新类写中文学习型类注释：说明做什么、为什么归属在此、与相邻系统不重合的边界。
- Generic Replicated Event、逐帧统一处理、Authority-only Grant/Remove、旧 Avatar 清退和并发组计数写简洁“为什么”注释。
- 不逐行翻译代码，不复制 Lyra 的项目专属 Camera、Message、GameFeature 或 Cost 体系。
- 不使用 `static_cast` 代替 UObject Cast。
- UObject/Asset 引用使用 `TObjectPtr`；需要 GC 追踪的结构成员使用 `UPROPERTY`。
- 避免不必要 Tick、手写网络 RPC 和硬编码资产路径。

## 十二、编译与检查

1. 运行格式/差异检查：
   - `git diff --check`
   - 确认只修改允许范围。
2. Unreal Editor 可能仍在运行，先执行无链接构建：
   ```text
   E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE -NoLink
   ```
3. 不要求关闭用户 UE；若完整链接被文件锁阻止，如实报告，由 Codex 后续收口。
4. 编译错误必须修复到 `Result: Succeeded`，不要把“只剩链接锁”之外的错误交给用户。

## 十三、中文报告

创建：

`D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Report.md`

至少包含：

1. 实际修改/新增文件。
2. 所有新增 C++ 类、父类、职责。
3. 两个枚举及各枚举值。
4. 所有公开函数、关键成员变量、类型、可见性和用途。
5. 两个 Native Tag。
6. AbilitySet Grant/Remove 生命周期。
7. PostProcessInput、三组输入缓存和 WaitInputRelease 的完整调用链。
8. Authority、Owning Client、Simulated Proxy 各自执行什么。
9. 编译命令与原始结果。
10. 与 Prompt 的任何偏差、风险和待人工 UE 配置项。
11. 明确未创建 UE 资产、未使用 MCP、未执行 Git 操作。

完成后停止。不要开始创建 Blueprint/IA/IMC/AbilitySet 资产，不继续 Cost/Cooldown、死亡、武器或正式技能。
