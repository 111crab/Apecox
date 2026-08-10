# 当前代码设计

更新日期：2026-08-10

## 当前执行设计：Phase 1C-V

在运行验证死亡/重生之前，为 `AApecoxPlayerCharacter` 建立可观察、可联网验证的 Manny 第一/第三人称表现基线。

```text
同一个 AApecoxPlayerCharacter
├─ Capsule + CharacterMovementComponent：唯一移动模拟、碰撞、预测和校正
├─ GetMesh()：第三人称 Manny，仅给其他玩家看
├─ FirstPersonMesh：第一人称 Manny，仅给拥有者看
└─ FirstPersonCamera：本地第一人称视角
```

第一/第三人称只拆分视觉表现，不拆分 Gameplay Actor、移动状态、ASC 或生命状态。

### 新增 Native GameplayTag

| C++ 名称 | Tag 字符串 | 用途 |
| --- | --- | --- |
| `InputTag_Move` | `InputTag.Move` | 二维移动输入意图 |
| `InputTag_Look_Mouse` | `InputTag.Look.Mouse` | 鼠标二维观察输入意图 |
| `InputTag_Jump` | `InputTag.Jump` | 跳跃输入意图 |

Walk、Run、Falling 不建 GameplayTag，它们由 CMC 和动画系统推导，不是跨系统需要显式授予/移除的稳定语义。

### `AApecoxPlayerCharacter` 新增组件和 Getter

| 名称 | 类型 | 作用 |
| --- | --- | --- |
| `FirstPersonMesh` | `TObjectPtr<USkeletalMeshComponent>` | 仅拥有者可见的第一人称 Manny 表现 |
| `FirstPersonCamera` | `TObjectPtr<UCameraComponent>` | 附着于第一人称 Mesh 的 `head` Socket，响应 ControlRotation |
| `GetFirstPersonMesh()` | `USkeletalMeshComponent*` | 为后续第一人称武器附着和表现访问提供稳定入口 |
| `GetFirstPersonCamera()` | `UCameraComponent*` | 为后续瞄准射线、ADS 和镜头系统提供稳定入口 |

采用 UE 5.8 官方可见性规则：`FirstPersonMesh` 使用 `OnlyOwnerSee`，现有 `GetMesh()` 使用 `OwnerNoSee`。资产由蓝图配置，不在 C++ 中硬编码路径。

由于 `ABP_FP_Copy` 需要从本地隐藏的 `GetMesh()` 读取最新组件空间骨骼，`GetMesh()->VisibilityBasedAnimTickOption` 必须固定为 `AlwaysTickPoseAndRefreshBones`；不能沿用 `ACharacter` 默认的 `AlwaysTickPose`。

### `AApecoxPlayerCharacter` 新增输入函数

| 函数 | 绑定 | 作用 |
| --- | --- | --- |
| `HandleMoveInput(const FInputActionValue&)` | Move / Triggered | 使用 Actor Forward/Right 输入 CMC |
| `HandleLookInput(const FInputActionValue&)` | Look.Mouse / Triggered | 修改 Controller Yaw/Pitch |
| `HandleJumpStarted(const FInputActionValue&)` | Jump / Started | 调用 `Jump()` |
| `HandleJumpCompleted(const FInputActionValue&)` | Jump / Completed + Canceled | 调用 `StopJumping()` |

角色 Yaw 跟随 Controller：`bUseControllerRotationYaw=true`，`bOrientRotationToMovement=false`。移动继续走 CMC 原生预测，不新增 RPC 或 Transform 复制代码。

### 动画和本轮边界

- 第三人称使用 `SKM_Manny_Simple + ABP_Unarmed`，提供 Idle、默认移动速度下的 Run、Jump/Fall。
- 第一人称使用同一 Manny Mesh + `ABP_FP_Copy`；该 AnimBP 从第三人称姿态复制并用 `CtrlRig_FPWarp` 修正第一人称构图。
- 第一人称手臂摆动来自动画姿态，不在 Character Tick 中人为摆动 Mesh 或 Camera。
- 本轮的 Run 不是 Shift Sprint，不新增 Sprint 输入、状态、属性或 Ability。
- 不实现武器、ADS、后坐力、滑铲、蹲伏、Montage 或项目自有 AnimInstance。
- 不修改已审查通过的 Health、Death、Respawn、ASC 与 Ability 输入生命周期。

## 已完成审查的 Phase 1C 死亡设计（保留参考）

Phase 1C：在现有 PlayerState ASC 基线上建立最小死亡、复活、换 Pawn 和 Dedicated Server 生命周期闭环。

本设计已经用户批准。实现必须保持以下职责边界：

```text
GameplayEffect 修改 Health
-> UApecoxVitalAttributeSet 检测 OutOfHealth
-> UApecoxHealthComponent 将数值事件翻译为 Pawn 死亡状态
-> UApecoxDeathAbility 处理 GAS 取消、阻塞和死亡流程
-> AApecoxPlayerCharacter 处理当前身体
-> AApecoxGameMode 在服务器延时生成新 Pawn
```

`UApecoxHealthComponent` 不保存 Health；Health/MaxHealth 仍只存在于 PlayerState 拥有的 `UApecoxVitalAttributeSet`。

## 新增 Native GameplayTag

文件：

- `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h`
- `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp`

| C++ 名称 | Tag 字符串 | 用途 |
| --- | --- | --- |
| `GameplayEvent_Death` | `GameplayEvent.Death` | OutOfHealth 后触发死亡 GA 的稳定 GAS 事件 |
| `State_Death` | `State.Death` | 死亡状态父标签，用于统一阻止普通 GA |
| `State_Death_Dying` | `State.Death.Dying` | 生命已归零、死亡流程尚未完成 |
| `State_Death_Dead` | `State.Death.Dead` | 死亡流程完成，旧 Pawn 可以销毁/等待重生 |

`Dying` 与 `Dead` 必须互斥。新 Pawn 初始化后两者都不存在。

## UApecoxVitalAttributeSet 修改

父类保持 `UAttributeSet`，属性保持 `Health/MaxHealth`；不新增正式伤害属性。

新增原生多播委托类型 `FApecoxAttributeEvent`，参数应至少覆盖：

- `AActor* EffectInstigator`
- `AActor* EffectCauser`
- `const FGameplayEffectSpec* EffectSpec`
- `float EffectMagnitude`
- `float OldValue`
- `float NewValue`

新增成员：

- `mutable FApecoxAttributeEvent OnHealthChanged`
- `mutable FApecoxAttributeEvent OnMaxHealthChanged`
- `mutable FApecoxAttributeEvent OnOutOfHealth`
- `bool bOutOfHealth`
- `float HealthBeforeAttributeChange`
- `float MaxHealthBeforeAttributeChange`

新增/扩展函数：

- `PreGameplayEffectExecute(...)`：保存 GE 执行前 Health/MaxHealth。
- `PostGameplayEffectExecute(...)`：沿用现有钳制，随后广播变化；只在首次跨入 `Health <= 0` 时广播 OutOfHealth。
- `OnRep_Health(...)`：客户端广播 Health 变化，并维护 `bOutOfHealth`，但客户端不得由此生成权威死亡事件。
- `OnRep_MaxHealth(...)`：客户端广播 MaxHealth 变化。
- `PostAttributeChange(...)`：Health 恢复到正数时重置 `bOutOfHealth`。

禁止在 AttributeSet 中调用 Character、GameMode、Destroy、RestartPlayer 或播放表现。

## EApecoxDeathState

位置：`ApecoxHealthComponent.h`，位于类声明前的模块级 `UENUM(BlueprintType)`。

```text
NotDead
DeathStarted
DeathFinished
```

含义：

- `NotDead -> DeathStarted`：生命归零，开始死亡流程。
- `DeathStarted -> DeathFinished`：死亡流程结束，可以处理旧 Pawn。
- 不允许逆向转换；新 Pawn 使用新组件实例恢复 `NotDead`。

## UApecoxHealthComponent

文件：

- `Source/Apecox/Public/Character/ApecoxHealthComponent.h`
- `Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

父类：`UActorComponent`。不需要 Transform、Tick 或 `USceneComponent`。

类声明：`Blueprintable, BlueprintSpawnableComponent`，默认复制。

公开 API：

- `InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC)`
- `UninitializeFromAbilitySystem()`
- `GetHealth()` / `GetMaxHealth()` / `GetHealthNormalized()`
- `GetDeathState()` / `IsDeadOrDying()`
- `StartDeath()`
- `FinishDeath()`

核心成员：

- `TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent`
- `TObjectPtr<const UApecoxVitalAttributeSet> VitalAttributeSet`
- `UPROPERTY(ReplicatedUsing=OnRep_DeathState) EApecoxDeathState DeathState`
- `OnHealthChanged` / `OnMaxHealthChanged`
- `OnDeathStarted` / `OnDeathFinished`

内部函数：

- `HandleHealthChanged(...)`
- `HandleMaxHealthChanged(...)`
- `HandleOutOfHealth(...)`
- `OnRep_DeathState(EApecoxDeathState OldDeathState)`
- `ClearDeathTags()`

关键约束：

- `HandleOutOfHealth` 只有在 Owner 具有 Authority 时，才通过 ASC 发送 `GameplayEvent.Death`。
- `StartDeath()` 设置 `DeathStarted`，添加 `State.Death.Dying`，移除 `State.Death.Dead`。
- `FinishDeath()` 设置 `DeathFinished`，移除 `State.Death.Dying`，添加 `State.Death.Dead`。
- `OnRep_DeathState` 必须重放状态转换；收到 `NotDead -> DeathFinished` 时依次执行 Start/Finish。
- Tag 由复制的 DeathState 在各端本地重建，不再额外创建一份独立复制状态。
- 解绑旧组件时，只有 ASC Avatar 仍是该组件 Owner，才允许清理 ASC Death Tag，防止旧 Pawn 晚解绑破坏新 Avatar。

## UApecoxDeathAbility

文件：

- `Source/Apecox/Public/AbilitySystem/Abilities/ApecoxDeathAbility.h`
- `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp`

父类：`UApecoxGameplayAbility`。

类声明：`Abstract, Blueprintable`。UE 中创建 `GA_Apecox_Death` 子类。

构造函数固定默认值：

- `InstancingPolicy = InstancedPerActor`
- `NetExecutionPolicy = ServerInitiated`
- `ActivationGroup = ExclusiveBlocking`
- GameplayEvent Trigger = `GameplayEvent.Death`
- `bAutoFinishDeath = true`

公开/受保护 API：

- 覆写 `ActivateAbility(...)`
- 覆写 `EndAbility(...)`
- `StartDeath()`
- `FinishDeathAndEndAbility()`，`BlueprintCallable`
- `K2_OnDeathStarted`，`BlueprintImplementableEvent`

激活顺序：

1. 验证 ASC、Avatar 和 HealthComponent。
2. 清空 ASC 输入。
3. 取消除自身外的活动 Ability。
4. 将自身设为不可取消。
5. `HealthComponent->StartDeath()`。
6. 调用 `K2_OnDeathStarted`，为未来死亡蒙太奇提供出口。
7. 首版 `bAutoFinishDeath=true`，立即调用 `FinishDeathAndEndAbility()`。

`EndAbility()` 应保证已经开始的死亡最终调用 `FinishDeath()`，但所有路径必须幂等。

## UApecoxGameplayAbility 修改

在项目 GA 基类构造函数中，把 `State.Death` 加入 `ActivationBlockedTags`。

这样所有普通 Apecox GA 在 Dying/Dead 时走 GAS 原生激活阻塞。死亡 GA 在添加死亡 Tag 前已由 `GameplayEvent.Death` 激活，因此不会阻止自身首次激活。

未来确有跨死亡 Ability 时，再由专用派生类移除此阻塞或引入 `Ability.Behavior.SurvivesDeath`；本阶段不提前增加该 Tag。

## AApecoxPlayerCharacter 修改

新增成员：

- `VisibleAnywhere`：`TObjectPtr<UApecoxHealthComponent> HealthComponent`
- `EditDefaultsOnly`：`TSubclassOf<UGameplayEffect> PawnInitializationEffect`

新增函数：

- `ApplyPawnInitializationEffect()`
- `HandleDeathStarted(AActor* OwningActor)`
- `HandleDeathFinished(AActor* OwningActor)`
- `DisableMovementAndCollision()`
- `DestroyDueToDeath()`

构造函数创建 HealthComponent，并绑定 Death Started/Finished 委托。

ASC 初始化顺序：

1. `InitAbilityActorInfo(PlayerState, this)`
2. 缓存 ASC
3. Grant Pawn AbilitySets
4. HealthComponent 绑定 ASC/VitalAttributeSet
5. Authority 应用 `PawnInitializationEffect`

`PawnInitializationEffect` 必须是 Instant GE；Context SourceObject 使用当前 Character。它是独立初始化入口，不进入可撤销 AbilitySet。

解绑顺序必须对称：

1. HealthComponent 解绑委托
2. 清 ASC 输入并取消 Ability
3. 撤销 Pawn AbilitySets
4. 清除或替换 Avatar
5. 清 Character 缓存

死亡处理：

- Started：停止 CharacterMovement、关闭 Capsule 碰撞；不把比赛或复活逻辑写进 Character。
- Finished：下一 Tick 调用 `DestroyDueToDeath()`，避免在 Death Ability 同步调用栈中撤销自身。
- `DestroyDueToDeath()` 仅 Authority 缓存 Controller、解绑占有、请求 GameMode 重生并给旧 Pawn 设置短 LifeSpan；所有端隐藏旧 Pawn。

## AApecoxGameMode 修改

新增成员：

- `EditDefaultsOnly float RespawnDelaySeconds = 3.0f`
- `TSet<TWeakObjectPtr<AController>> PendingRespawnControllers`

新增函数：

- `RequestPlayerRespawn(AController* Controller)`
- `RestartPlayerAfterDelay(TWeakObjectPtr<AController> Controller)`

规则：

- 只允许 Authority 调用。
- 同一个 Controller 已在 Pending 集合时不得重复创建 Timer。
- Timer 到期先从 Pending 移除。
- Controller 有效且没有 Pawn 时调用引擎 `RestartPlayer()`。
- GameMode 不接触 ASC 内部、AttributeSet 或 AbilitySet。

## UE 测试资产约定

代码审查通过后再由用户手工创建，ClaudeCode 不操作 UE 编辑器：

- `GE_Apecox_InitializePawnStats`：Instant；Override MaxHealth=100、Health=100。
- `GE_Debug_SelfDamage`：Instant；Add Health=-100，仅用于当前阶段。
- `GA_Apecox_Death`：继承 `UApecoxDeathAbility`，保持 Auto Finish。
- `GA_Debug_SelfDamage`：继承 `UApecoxGameplayAbility`，Q 激活后 Apply GE to Owner 并 EndAbility。
- AbilitySet DataAsset 必须使用 `DA_` 前缀，同时授予死亡 GA 和调试 GA；只有调试 GA 绑定 Tactical InputTag。

## 审查重点

- OutOfHealth 是否只在一次正数到零的过渡中触发。
- 客户端 OnRep 是否错误触发权威死亡 GameplayEvent。
- DeathState 和 Death Tag 是否出现不一致或 Dying/Dead 同时存在。
- Death Ability 是否在取消所有 Ability 时取消了自身。
- Character 是否在 Death Ability 调用栈内立即 Remove AbilitySet。
- 旧 Pawn 晚解绑是否清除了新 Pawn 的 Avatar 或 Death Tag。
- Respawn 是否可能重复计时、重复生成 Pawn。
- 新 Pawn 是否会重复授予 AbilitySet 或遗留 Held Input。
- Dedicated Server 路径是否引用 UI、Camera、Audio、SkeletalMesh 或本地玩家对象。

---

# Phase 1C-CAM：项目 PlayerCameraManager 基线

批准日期：2026-08-10

## 目标

补齐 UE 5.8 官方 First Person 模板中的镜头 Pitch 约束，避免完整 Manny 第一人称表现允许摄像机低头进入头部或胸腔内部。该修改只建立项目镜头策略入口，不扩展 ADS、后坐力、CameraMode 或武器表现。

## 新增类

### AApecoxPlayerCameraManager

文件：

- `Source/Apecox/Public/Camera/ApecoxPlayerCameraManager.h`
- `Source/Apecox/Private/Camera/ApecoxPlayerCameraManager.cpp`

父类：`APlayerCameraManager`。

公开 API：

- `AApecoxPlayerCameraManager()`

构造函数只设置父类已有属性：

- `ViewPitchMin = -70.0f`
- `ViewPitchMax = 80.0f`

这两个值来自本机 UE 5.8 官方 `BP_FirstPersonCameraManager`。本轮不复制第三方蓝图，不新增重复成员变量或 UPROPERTY。

职责：

- 管理本地玩家镜头的公共规则。
- 当前只负责第一人称上下观察角度。
- 后续 ADS FOV、后坐力镜头反馈、死亡/观战镜头可以在独立阶段扩展，但本轮不预留空 API。
- PlayerCameraManager 不承载 Character 移动、GAS、武器或动画逻辑，也不需要复制。

## AApecoxPlayerController 修改

新增公开构造函数：

- `AApecoxPlayerController()`

构造函数设置：

```cpp
PlayerCameraManagerClass = AApecoxPlayerCameraManager::StaticClass();
```

现有 `PostProcessInput()` 保持不变。`AApecoxGameMode` 已经使用 `AApecoxPlayerController::StaticClass()`，因此不修改 GameMode，也不创建 PlayerController 蓝图。

## 明确不做

- 不修改 `AApecoxPlayerCharacter`、双 Mesh、Camera Component、FOV、Scale 或 Camera Offset。
- 不修改 CMC、输入、ASC、AttributeSet、Health、Death 或 Respawn。
- 不修改 `NearClipPlane`；它不能解决摄像机位于完整身体内部的问题。
- 不复制或继承第三方 `BP_FirstPersonCameraManager`。
- 不创建或修改 `.uasset`、`.umap`、GameplayTag、GA、GE 或 DataAsset。
- Motion Blur 已由用户在项目设置中关闭，本轮不再次修改 Config。

## 验证标准

- 本地视角限制在 `-70 ~ 80` 度，不能继续低头进入 Manny 身体内部。
- 水平观察、WASD、Jump 和 Tactical Ability 保持原行为。
- Listen Server 的 Host 与 Client 各自使用本地 CameraManager，不互相影响。
- 死亡并重生后 Pitch 约束继续生效，因为 CameraManager 随 PlayerController 保留。
- 不出现 CameraManager 类加载、PlayerController 或输入生命周期错误。
