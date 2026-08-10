# Phase 1C 实施报告：死亡、复活与换 Pawn 生命周期

**日期**：2026-08-10  
**实施者**：ClaudeCode（子代理）  
**架构审查**：Codex（待审查）  
**分支**：未 Git 操作

---

## 一、修改和新增文件列表

### 新增文件（4 个）

| 文件 | 位置 | 说明 |
|------|------|------|
| `ApecoxHealthComponent.h` | `Source/Apecox/Public/Character/` | HealthComponent 头文件 |
| `ApecoxHealthComponent.cpp` | `Source/Apecox/Private/Character/` | HealthComponent 实现 |
| `ApecoxDeathAbility.h` | `Source/Apecox/Public/AbilitySystem/Abilities/` | DeathAbility 头文件 |
| `ApecoxDeathAbility.cpp` | `Source/Apecox/Private/AbilitySystem/Abilities/` | DeathAbility 实现 |

### 修改文件（8 个）

| 文件 | 修改概要 |
|------|----------|
| `ApecoxGameplayTags.h` | 新增 4 个 Native GameplayTag 声明 |
| `ApecoxGameplayTags.cpp` | 新增 4 个 Native GameplayTag 定义 |
| `ApecoxVitalAttributeSet.h` | 新增 `FApecoxAttributeEvent` 委托、`OnHealthChanged/OnMaxHealthChanged/OnOutOfHealth`、`bOutOfHealth`、旧值暂存成员 |
| `ApecoxVitalAttributeSet.cpp` | 重写 `PostGameplayEffectExecute`、`OnRep_Health`、新增 `PostAttributeChange` 恢复逻辑 |
| `ApecoxGameplayAbility.cpp` | 构造函数中 `State.Death` 加入 `ActivationBlockedTags` |
| `ApecoxAbilitySystemComponent.h` | `CancelAbilitiesByFunc` 从 `private` 移至 `public` |
| `ApecoxPlayerCharacter.h` | 新增 `HealthComponent`、`PawnInitializationEffect`、死亡回调、`CachedController`、`ApplyPawnInitializationEffect` |
| `ApecoxPlayerCharacter.cpp` | 构造 HealthComponent、InitializeAbilitySystem 中绑定并应用初始化 GE、Uninitialize 中解绑、死亡处理函数实现 |
| `ApecoxGameMode.h` | 新增 `RespawnDelaySeconds`、`PendingRespawnControllers`、`RequestPlayerRespawn`、`RestartPlayerAfterDelay` |
| `ApecoxGameMode.cpp` | 实现重生调度逻辑 |

---

## 二、新增类及父类

| 新增类 | 父类 | 说明 |
|--------|------|------|
| `UApecoxHealthComponent` | `UActorComponent` | 管理 Pawn 死亡状态转换，监听 `VitalAttributeSet` 事件，翻译为复制的 `DeathState` |
| `UApecoxDeathAbility` | `UApecoxGameplayAbility` | 响应 `GameplayEvent.Death`，取消其他 GA、清输入、进入死亡状态；`Abstract + Blueprintable` |
| `EApecoxDeathState` | `uint8` (UENUM) | `NotDead → DeathStarted → DeathFinished`，不逆向 |

---

## 三、新增成员详解

### 3.1 UApecoxHealthComponent

#### 成员变量

| 名称 | 类型 | 可见性 | 作用 |
|------|------|--------|------|
| `DeathState` | `EApecoxDeathState` | `private` / `ReplicatedUsing=OnRep_DeathState` | 复制死亡状态给所有相关端 |
| `bDeathTagsApplied` | `bool` | `private` | 幂等防护：防止 `FinishDeath` 重复添加 Dead Tag |
| `AbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `private` | 缓存的 ASC 引用（不由组件拥有） |
| `VitalAttributeSet` | `TObjectPtr<const UApecoxVitalAttributeSet>` | `private` | 缓存的 VitalSet 引用（不由组件拥有） |

#### 成员函数

| 名称 | 参数 | 可见性 | 作用 |
|------|------|--------|------|
| `InitializeWithAbilitySystem` | `UApecoxAbilitySystemComponent*` | `public` / `BlueprintCallable` | 绑定 ASC 和 VitalAttributeSet 委托；幂等 |
| `UninitializeFromAbilitySystem` | — | `public` / `BlueprintCallable` | 解绑委托；仅在 ASC Avatar 仍是 Owner 时清理死亡 Tag |
| `GetHealth` | — | `public` / `BlueprintCallable` | 返回当前 Health（透传 VitalSet） |
| `GetMaxHealth` | — | `public` / `BlueprintCallable` | 返回当前 MaxHealth |
| `GetHealthNormalized` | — | `public` / `BlueprintCallable` | 返回 0.0~1.0 的标准化生命值 |
| `IsDeadOrDying` | — | `public` / `BlueprintCallable` | 是否处于死亡流程中 |
| `StartDeath` | — | `public` | DeathStarted 转换 + 添加 Dying Tag；幂等 |
| `FinishDeath` | — | `public` | DeathFinished 转换 + Dying→Dead Tag 切换；幂等 |
| `HandleHealthChanged` | `AActor*, AActor*, const FGameplayEffectSpec*, float, float, float` | `protected` | 本地表现层回调（UI 更新等） |
| `HandleMaxHealthChanged` | 同上 | `protected` | 本地表现层回调 |
| `HandleOutOfHealth` | 同上 | `protected` | **仅 Authority** 发送 `GameplayEvent.Death` |
| `OnRep_DeathState` | `EApecoxDeathState OldDeathState` | `private` / `UFUNCTION` | 处理 OnRep，支持合并复制跳帧 |
| `ClearDeathTags` | — | `private` | 移除 Dying/Dead Tag |
| `ApplyDeathTagsForState` | — | `private` | 根据 DeathState 重建 Tag（Dying/Dead 互斥） |

#### 委托

| 名称 | 类型 | 广播时机 |
|------|------|----------|
| `OnDeathStarted` | `FApecoxDeathStateEvent` (OneParam: `AActor*`) | DeathState 转换到 DeathStarted |
| `OnDeathFinished` | `FApecoxDeathStateEvent` (OneParam: `AActor*`) | DeathState 转换到 DeathFinished |

### 3.2 UApecoxDeathAbility

#### 成员变量

| 名称 | 类型 | 可见性 | 作用 |
|------|------|--------|------|
| `bAutoFinishDeath` | `bool` | `private` / `EditDefaultsOnly` | 首版 `true`：Activate 后立即完成死亡；未来设为 `false` 由蒙太奇回调调用 `FinishDeathAndEndAbility` |
| `bDeathFinished` | `bool` | `private` | 防止 `FinishDeathAndEndAbility` / `EndAbility` 重复调用 |

#### 成员函数

| 名称 | 可见性 | 作用 |
|------|--------|------|
| `ActivateAbility` | `public` / override | 1) Commit → 2) 取消其他 GA + 清输入 → 3) SetCanBeCanceled(false) → 4) StartDeath → 5) K2_OnDeathStarted → 6) 若 bAutoFinishDeath 则 FinishDeathAndEndAbility |
| `EndAbility` | `public` / override | 保证任何退出路径（含 Cancel）最终调用 FinishDeath（幂等） |
| `FinishDeathAndEndAbility` | `public` / `BlueprintCallable` | 调用 HealthComponent::FinishDeath，然后 EndAbility；幂等 |
| `K2_OnDeathStarted` | `protected` / `BlueprintImplementableEvent` | 蓝图事件出口——未来播放死亡蒙太奇 |
| `CancelOtherAbilitiesAndClearInput` | `protected` | 清空 ASC 输入缓存；通过 `CancelAbilitiesByFunc` 取消除自身外所有活动 GA |

### 3.3 新增 Native GameplayTag（ApecoxGameplayTags）

| Tag 名称 | 类别 | 作用 |
|----------|------|------|
| `GameplayEvent.Death` | 事件 | HealthComponent(Authority) 发送给 DeathAbility 的事件触发 |
| `State.Death` | 状态（父标签） | 加入所有 GA 的 `ActivationBlockedTags` 统一阻止激活 |
| `State.Death.Dying` | 状态（子标签） | DeathStarted 期间添加，与 Dead 互斥 |
| `State.Death.Dead` | 状态（子标签） | DeathFinished 期间添加，与 Dying 互斥 |

### 3.4 VitalAttributeSet 新增成员

| 名称 | 类型 | 可见性 | 作用 |
|------|------|--------|------|
| `OnHealthChanged` | `FApecoxAttributeEvent` (SixParams) | `public` / `mutable` | GE 后广播 Health 变化 |
| `OnMaxHealthChanged` | `FApecoxAttributeEvent` | `public` / `mutable` | GE 后广播 MaxHealth 变化 |
| `OnOutOfHealth` | `FApecoxAttributeEvent` | `public` / `mutable` | 首次正数→≤0 时广播一次 |
| `bOutOfHealth` | `bool` | `private` | 门控：阻止同一段低血量反复广播 OutOfHealth |
| `HealthBeforeAttributeChange` | `float` | `private` | PreAttributeChange 中暂存旧值供 PostGameplayEffectExecute 使用 |
| `MaxHealthBeforeAttributeChange` | `float` | `private` | 同上 |

#### 新增委托类型

```cpp
DECLARE_MULTICAST_DELEGATE_SixParams(FApecoxAttributeEvent,
    AActor* /*EffectInstigator*/,
    AActor* /*EffectCauser*/,
    const FGameplayEffectSpec* /*EffectSpec*/,
    float /*EffectMagnitude*/,
    float /*OldValue*/,
    float /*NewValue*/
);
```

### 3.5 ApecoxPlayerCharacter 新增成员

| 名称 | 类型 | 可见性 | 作用 |
|------|------|--------|------|
| `HealthComponent` | `TObjectPtr<UApecoxHealthComponent>` | `protected` / `VisibleAnywhere, BlueprintReadOnly` | 构造时创建，管理死亡状态 |
| `PawnInitializationEffect` | `TSubclassOf<UGameplayEffect>` | `protected` / `EditDefaultsOnly` | 蓝图配置的出生 Instant GE |
| `CachedController` | `TObjectPtr<AController>` | `private` / `Transient` | 死亡完成后缓存，用于重生请求 |
| `OnDeathStartedHandle` | `FDelegateHandle` | `private` | 解绑用句柄 |
| `OnDeathFinishedHandle` | `FDelegateHandle` | `private` | 解绑用句柄 |
| `OnDeathStarted` | `void(AActor*)` | `protected` | 停止移动、关闭碰撞 |
| `OnDeathFinished` | `void(AActor*)` | `protected` | 隐藏、LifeSpan、缓存 Controller、请求重生 |
| `ApplyPawnInitializationEffect` | `void()` | `protected` | 验证 Instant → Apply GE to Self |

### 3.6 ApecoxGameMode 新增成员

| 名称 | 类型 | 可见性 | 作用 |
|------|------|--------|------|
| `RespawnDelaySeconds` | `float` | `protected` / `EditDefaultsOnly` | 重生延迟，默认 3 秒 |
| `PendingRespawnControllers` | `TSet<TObjectPtr<APlayerController>>` | `private` / `Transient` | 防重入集合 |
| `RespawnTimerHandle` | `FTimerHandle` | `private` | 重生定时器句柄 |
| `RequestPlayerRespawn` | `void(APlayerController*)` | `public` | 加入等待队列并启动 Timer |
| `RestartPlayerAfterDelay` | `void()` | `protected` | Timer 回调：遍历待重生集合并调用 `RestartPlayer` |

---

## 四、完整死亡/重生运行链路

### Authority（服务器）

```
1. Instant GE 使 Health 归零
   └→ PreAttributeChange 保存旧值
   └→ PostGameplayEffectExecute:
       ├→ Clamp Health to [0, MaxHealth]
       ├→ OnHealthChanged.Broadcast(Instigator, Causer, Spec, Magnitude, OldValue, NewValue)
       └→ Health <= 0 && OldHealth > 0 && !bOutOfHealth:
           ├→ bOutOfHealth = true
           └→ OnOutOfHealth.Broadcast(...)

2. HealthComponent::HandleOutOfHealth (Authority only)
   └→ ASC->HandleGameplayEvent(GameplayEvent.Death, &EventData)

3. DeathAbility::ActivateAbility
   ├→ CommitAbility 检查
   ├→ CancelOtherAbilitiesAndClearInput()
   │   ├→ ASC->ClearAbilityInput()
   │   └→ ASC->CancelAbilitiesByFunc(predicate, replicate=true)
   │       └→ 取消所有活动 GA（自身除外）
   ├→ SetCanBeCanceled(false)
   ├→ HealthComponent->StartDeath()
   │   ├→ DeathState = DeathStarted (触发复制)
   │   ├→ ApplyDeathTagsForState() → Add State.Death.Dying
   │   └→ OnDeathStarted.Broadcast(Owner)
   ├→ K2_OnDeathStarted() (蓝图事件)
   └→ bAutoFinishDeath ? FinishDeathAndEndAbility()
       └→ HealthComponent->FinishDeath()
           ├→ DeathState = DeathFinished (触发复制)
           ├→ State.Death.Dying → State.Death.Dead
           └→ OnDeathFinished.Broadcast(Owner)

4. Character::OnDeathStarted (由 HealthComponent 委托触发)
   ├→ StopMovementImmediately + DisableMovement
   └→ Capsule->SetCollisionEnabled(NoCollision)

5. Character::OnDeathFinished (由 HealthComponent 委托触发)
   ├→ SetActorHiddenInGame(true) (各端隐藏)
   ├→ SetLifeSpan(5.0f) (兜底销毁)
   ├→ CachedController = GetController()
   └→ GameMode->RequestPlayerRespawn(PC)

6. GameMode::RequestPlayerRespawn
   ├→ PendingRespawnControllers.Add(Controller)
   └→ 若 Timer 未激活，启动 RespawnDelaySeconds 定时器

7. 3 秒后 GameMode::RestartPlayerAfterDelay
   └→ 遍历 PendingRespawnControllers:
       └→ RestartPlayer(PC)
           ├→ Destroy 旧 Pawn (若仍存在)
           │   └→ 旧 Pawn EndPlay → UninitializeAbilitySystem
           │       ├→ HealthComponent 解绑委托
           │       └→ ASC Avatar 已是新 Pawn → 不清除 Avatar
           ├→ SpawnDefaultPawnAtTransform → 新 Character
           │   └→ Constructor 创建新 HealthComponent（DeathState = NotDead）
           └→ Controller->Possess(NewPawn)
               └→ NewPawn->PossessedBy → InitializeAbilitySystem
                   ├→ ASC->InitAbilityActorInfo(PS, this) [绑定同一 PS/ASC]
                   ├→ GrantPawnAbilitySets()
                   ├→ HealthComponent->InitializeWithAbilitySystem(ASC) [绑定新 VitalSet]
                   ├→ 绑定死亡委托
                   └→ ApplyPawnInitializationEffect()
                       ├→ 检查 Instant
                       ├→ ApplyGameplayEffectSpecToSelf
                       └→ VitalSet 属性重置为出生值 → 复制给客户端
```

### Owning Client（控制端）

```
1. 客户端可能导致 Health 变化（本地预测 GE），但最终值由服务器权威
2. OnRep_Health:
   ├→ 广播 OnHealthChanged（表现层）
   └→ 维护 bOutOfHealth 本地标志
3. OnRep_DeathState:
   ├→ 重放本地表现：
   │   ├→ NotDead → DeathStarted: OnDeathStarted.Broadcast
   │   ├→ NotDead → DeathFinished (合并): Started + Finished
   │   └→ DeathStarted → DeathFinished: OnDeathFinished.Broadcast
   └→ ApplyDeathTagsForState (本地重建死亡 Tag)
4. Character 死亡回调 (OnRep 触发):
   ├→ OnDeathStarted: 停止移动、关闭碰撞
   └→ OnDeathFinished: 隐藏旧 Pawn
5. RestartPlayer 后新 Pawn 通过属性复制获得 Health/MaxHealth 初始值
```

### Simulated Proxy（旁观端）

```
与 Owning Client 完全同步：同样通过 OnRep_DeathState 感知死亡，
通过属性复制获得新 Pawn 出生值。不做重生请求，不发送 GameplayEvent。
```

---

## 五、与 Lyra 思想相同的边界 vs 本项目主动删减

### 与 Lyra 共享的设计思想

| 思想 | Apecox 实现 |
|------|------------|
| **HealthComponent 不存属性，只做翻译** | 只引用 ASC/VitalSet，`GetHealth()` 透传 AttributeSet |
| **死亡通过 GameplayEvent 触发** | `GameplayEvent.Death` → DeathAbility 的 `AbilityTriggers` |
| **死亡 Tag 阻塞普通 GA** | `State.Death` 放入所有 GA 的 `ActivationBlockedTags` |
| **DeathState 复制统一驱动所有端** | `EApecoxDeathState` ReplicatedUsing=OnRep_DeathState |
| **PawnInitializationEffect 恢复出生属性** | Character 蓝图层配置 Instant GE，Authority 上应用 |
| **GameMode 延迟重生** | `RespawnDelaySeconds` 默认 3s，Timer 批量处理 |

### 主动删减的重型部分

| Lyra 含有的 | Apecox 删减原因 |
|-------------|----------------|
| `HealthComponent` 内含 Shield、Damage、Healing 多属性管道 | 首版只需要 Health→Death 链路；后续按需扩展 |
| 伤害类型系统（Physical/Magic/Poison 等） | 非首版 MVP 范围 |
| 击杀消息、助攻追踪、淘汰统计 | Phase 2+ 比赛规则 |
| 死亡蒙太奇通知系统 | 预留 `K2_OnDeathStarted` 蓝图出口，C++ 不做动画编排 |
| 服务器倒地带复活（DBNO / Revive） | 英雄不计划做倒地救起 |
| `Ability.Behavior.SurvivesDeath` | 首版所有 GA 在死亡时均不可激活 |
| 死亡期间专用 UI/Cue/Audio | 由蓝图子类配置，纯 C++ 不做表现 |
| 多阶段死亡（Ragdoll、Gib、Dissolve） | 仅做 `SetActorHiddenInGame` + LifeSpan 兜底 |
| 延迟重生期间的观战切镜 | 由蓝图配置 Camera 管理 |

---

## 六、构建结果

**构建命令**：
```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

**结果**：✅ **Succeeded**（0 错误，0 警告**）

> \*\* 1 个预存警告：`ApecoxAbilitySystemComponent.cpp` 中 `NonInstanced` API 弃用警告（C4996）。非本批引入，不改范围外代码。

**输出**：`E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`

**编译轮次**：第 3 次通过（第 1 次 `BlueprintSpawnableComponent` UHT 错误已修复，第 2 次 `FGameplayEffectSpec` 前向声明缺失已修复）

---

## 七、未执行的 UE 手工资产操作清单

以下操作需要用户在 UE 编辑器中手动完成，本批 C++ 实现未涉及：

1. **创建 `BP_Apecox_Death` 蓝图**
   - 基于 `UApecoxDeathAbility` 创建 `GA_Apecox_Death` 蓝图子类
   - 配置到 PawnAbilitySet 或英雄初始 GrantedAbility 列表
   - 后续可配置 `bAutoFinishDeath = false` 并在 `K2_OnDeathStarted` 中播放死亡蒙太奇

2. **配置 `PawnInitializationEffect`**
   - 在 Character 蓝图（`BP_ApecoxCharacter`）的 `PawnInitializationEffect` 属性中指定出生 GE
   - 创建 `GE_Apecox_PawnInit` Instant GameplayEffect，设置 Health/MaxHealth Modifier

3. **配置 GameMode 蓝图**
   - 在 `BP_ApecoxGameMode` 中可覆盖 `RespawnDelaySeconds`

4. **配置死亡 Tag 关系**
   - 确保 `State.Death.Dying` 和 `State.Death.Dead` 的父标签均为 `State.Death`
   - 此关系在 Native Tag 的 `.ini` 注册中定义（通常由 UHT 自动处理）

5. **验证死亡伤害 GE**
   - 确认项目中至少有一个 Instant GE 能对 Health 造成 >0 伤害
   - 用于测试完整死亡链路

---

## 八、风险、假设与 Codex 复审点

### 风险

1. **旧 Pawn UnPossess 时序依赖**：`RestartPlayer` 是先 `Destroy` 旧 Pawn 再 `Possess` 新 Pawn。如果 UE 版本的行为不同（先 UnPossess 再 Destroy），旧 Pawn 的 `UninitializeAbilitySystem` 可能先运行并在 `GetAvatarActor() == this` 分支中调用 `SetAvatarActor(nullptr)`——此时新 Pawn 尚未绑定。当前代码依赖于 UE 5.8 的标准 `RestartPlayer` 流程：Destroy 先于 Spawn。如果行为不符，需要将 Avatar 置空步骤移到 EndPlay 中。

2. **HealthComponent OnRep 顺序**：如果 `DeathState` 的复制先于 `Health` 属性的复制到达，客户端可能在 Health 显示 >0 的情况下进入死亡状态。当前设计容忍此情况——死亡 Tag 生效后 GA 已被阻止，UI 层应检查 `IsDeadOrDying()` 而非仅 Health 值。

3. **PendingRespawnControllers 不跨 World**：集合是 Transient 的，Server Travel 后丢失。对于小型多人地图，这不成问题；如果以后需要关卡切换时的重生队列持久化，需要用 GameState 或 GameInstance 承载。

### 假设

1. `ApecoxGameplayTags::State_Death` 在 UObject CDO 构造时已注册——Native GameplayTags 在引擎启动阶段初始化，早于模块加载时的 CDO 构造。
2. `CancelAbilitiesByFunc` 从 private 移至 public 不会破坏任何现有调用——该函数被 ASC 内部和 DeathAbility 共用，语义未变。
3. Character 的 `PawnInitializationEffect` 只配置 Instant GE——非 Instant 会被 `ensureMsgf` 拒绝，不会静默产生未跟踪 Active GE。

### Codex 复审点

1. **HealthComponent 与 Character 的职责边界**：`StartDeath`/`FinishDeath` 由 DeathAbility 调用，Character 通过委托响应。是否需要为未来 NPC（非 Character 角色）调整 HealthComponent 与 Avatar 的耦合方式？

2. **PawnInitializationEffect 应用时机**：当前在 `InitializeAbilitySystem` 的 `InitAbilityActorInfo` 之后立即应用。如果 AbilitySet 中包含某些需要基于 Health 值做条件激活的 GA，初始化 GE 的 Modifier 可能与 GA 激活存在竞争——但 Instant GE 同步执行，应该安全。

3. **`bAutoFinishDeath` 的未来扩展**：当前设为 `true`，死亡流程瞬间完成。将来如果改为 `false`（等待蒙太奇结束），需要蓝图子类在蒙太奇结束时调用 `FinishDeathAndEndAbility()`。建议在蓝图子类中显式配置此标志。

---

## 九、Git 声明

**本批实现未执行** `git add`、`git commit`、`git push`、`git stash` 或任何 Git 操作。未创建分支，未修改 `.gitignore`。所有文件变更仅存在于工作树中，由用户自行决定提交策略。

---

## 十、Public/Private 目录对称检查

```
Source/Apecox/
├── Public/
│   ├── Character/
│   │   ├── ApecoxHealthComponent.h      ✅ 新增
│   │   └── ApecoxPlayerCharacter.h      ✅ 修改
│   ├── AbilitySystem/
│   │   ├── Abilities/
│   │   │   ├── ApecoxDeathAbility.h     ✅ 新增
│   │   │   └── ApecoxGameplayAbility.h  （无需修改）
│   │   ├── Attributes/
│   │   │   └── ApecoxVitalAttributeSet.h ✅ 修改
│   │   └── ApecoxAbilitySystemComponent.h ✅ 修改（publicize CancelAbilitiesByFunc）
│   ├── Game/
│   │   └── ApecoxGameMode.h             ✅ 修改
│   └── GameplayTags/
│       └── ApecoxGameplayTags.h          ✅ 修改
├── Private/
│   ├── Character/
│   │   ├── ApecoxHealthComponent.cpp    ✅ 新增
│   │   └── ApecoxPlayerCharacter.cpp    ✅ 修改
│   ├── AbilitySystem/
│   │   ├── Abilities/
│   │   │   ├── ApecoxDeathAbility.cpp   ✅ 新增
│   │   │   └── ApecoxGameplayAbility.cpp ✅ 修改
│   │   └── Attributes/
│   │       └── ApecoxVitalAttributeSet.cpp ✅ 修改
│   ├── Game/
│   │   └── ApecoxGameMode.cpp           ✅ 修改
│   └── GameplayTags/
│       └── ApecoxGameplayTags.cpp        ✅ 修改
```

全部对称 ✅

---

## 十一、自检清单确认

| # | 检查项 | 状态 |
|---|--------|------|
| 1 | Public/Private 目录对称 | ✅ |
| 2 | 所有新增 UObject/Component/Ability 指针受 UPROPERTY 或生命周期保护 | ✅ |
| 3 | 委托在初始化时绑定，在 Uninitialize/OnUnregister 中解除 | ✅ |
| 4 | DeathState 注册 DOREPLIFETIME 并调用 Super | ✅ |
| 5 | Native Tag 使用声明/定义宏，不使用运行时字符串 | ✅ |
| 6 | OutOfHealth、StartDeath、FinishDeath、RequestPlayerRespawn 具有幂等保护 | ✅ |
| 7 | DeathAbility 不会取消自身 | ✅ (CancelAbilitiesByFunc 谓词排除 `this`) |
| 8 | 新 Pawn 绑定后，旧 Pawn 清理路径不把 ASC Avatar 置空 | ✅ (UninitializeFromAbilitySystem 有 Avatar 验证) |
| 9 | Dedicated Server 路径不访问 LocalPlayer、Camera、Audio、HUD | ✅ |
| 10 | 代码注释说明设计理由 | ✅ (中文注释覆盖所有关键决策) |

---

**实施完成，等待 Codex 审查。**
