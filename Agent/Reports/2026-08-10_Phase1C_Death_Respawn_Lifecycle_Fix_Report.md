# Phase 1C 修复报告：死亡/重生生命周期审查修复

**日期**：2026-08-10  
**实施者**：ClaudeCode（子代理）  
**审查来源**：Codex — `Agent/Reviews/2026-08-10_Phase1C_Death_Respawn_Lifecycle_Review.md`（结论：未通过）  
**状态**：等待 Codex 二次审查

---

## 一、修改文件列表

| 文件 | 修改类型 | 对应审查问题 |
|------|----------|-------------|
| `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h` | 修改 | P1: GE 前后值链路 |
| `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | 重写 | P1: GE 前后值链路 |
| `Source/Apecox/Public/Character/ApecoxHealthComponent.h` | 重写 | P1: 网络更新 / P2: 幂等、委托转发、Tag 计数、删除无效成员、Target |
| `Source/Apecox/Private/Character/ApecoxHealthComponent.cpp` | 重写 | 同上 |
| `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h` | 修改 | P2: 恢复 CancelAbilitiesByFunc 为 private |
| `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp` | 重写 | P2: GAS 引擎接口、状态重置、验证、删除 CommitAbility |
| `Source/Apecox/Public/AbilitySystem/Abilities/ApecoxDeathAbility.h` | 修改 | P2: 新增 bDeathStarted |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | 修改 | P1: 解绑顺序、DestroyDueToDeath、删除 CachedController |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 修改 | P1: 解绑顺序、DestroyDueToDeath、next-tick 销毁 |
| `Source/Apecox/Public/Game/ApecoxGameMode.h` | 重写 | P1: 每玩家独立倒计时 |
| `Source/Apecox/Private/Game/ApecoxGameMode.cpp` | 重写 | P1: 每玩家独立倒计时 |

---

## 二、逐审查问题修复详情

### P1-1：VitalAttributeSet 的 GE 前后值链路

**问题**：`PreAttributeChange` 保存旧值，随后 `PostGameplayEffectExecute` 内 `SetHealth()` 再次触发 `PreAttributeChange`，覆盖已保存的旧值，导致 `OnOutOfHealth` 无法稳定广播。

**修复**：

1. 新增 `PreGameplayEffectExecute()` 覆写——在整个 GE Modifier 执行前统一保存 `HealthBeforeAttributeChange` 和 `MaxHealthBeforeAttributeChange`。

2. `PreAttributeChange()` 移除旧值快照职责，只保留 `ClampAttribute` 数值约束。

3. `PostGameplayEffectExecute()` 重写：
   - Health GE：先钳制到 `[0, MaxHealth]`，再比较快照与最终值，广播 `OnHealthChanged`；仅在 `OldValue > 0 && NewValue <= 0 && !bOutOfHealth` 时广播 `OnOutOfHealth`。
   - MaxHealth GE：广播 `OnMaxHealthChanged`；若 MaxHealth 降低连带压低 Health，同步广播 `OnHealthChanged` 和 `OnOutOfHealth`（如适用）。
   - **所有委托广播完成后**才设置 `bOutOfHealth = true`，防止回调中恢复生命导致门控被提前清除。

4. `OnRep_Health()`：客户端同步 `bOutOfHealth`（`Health > 0 → false`），保证复活后状态正确。

**新增/改名/删减成员**：

| 成员 | 变更 | 说明 |
|------|------|------|
| `PreGameplayEffectExecute()` | **新增** | 在 GE Modifier 执行前保存旧值 |
| `PreAttributeChange()` | **职责变更** | 仅钳制，不再保存旧值快照 |
| `OnRep_Health()` | **行为变更** | 新增 `bOutOfHealth = (Health <= 0)` 同步 |

---

### P1-2：旧 Pawn 未解除占有，RestartPlayer 不会生成新 Pawn

**问题**：`OnDeathFinished()` 只隐藏旧 Pawn、设置 5s LifeSpan 并请求重生；Controller 仍占有旧 Pawn，`RestartPlayer()` 不会自动销毁已有 Pawn，导致复用旧 Pawn。

**修复**：

1. `OnDeathFinished()`：Authority 通过 `SetTimerForNextTick` 调度 `DestroyDueToDeath()`，不在死亡委托栈内直接拆 Pawn。

2. 新增 `DestroyDueToDeath()`：
   - 所有端已在 `OnDeathFinished` 中隐藏。
   - Authority：缓存 Controller → `DetachFromControllerPendingDestroy()` 解除占有 → `SetLifeSpan(0.1f)` 短兜底 → `GameMode->RequestPlayerRespawn(PC)`。

3. 删除 `CachedController` 成员——Controller 引用在 `DestroyDueToDeath` 栈内作为局部变量使用，不需要跨函数保存。

4. 删除 5 秒 LifeSpan，改为 0.1 秒兜底。

**新增/改名/删减成员**：

| 成员 | 变更 | 说明 |
|------|------|------|
| `DestroyDueToDeath()` | **新增** | next-tick 销毁、解绑占有、短 LifeSpan、请求重生 |
| `CachedController` | **删除** | 不再跨函数保存 Controller，改为局部变量 |
| `OnDeathFinished()` | **行为变更** | 不再直接设置 LifeSpan 和请求重生，改为调度 next-tick |

---

### P1-3：ASC 解绑顺序会遗留死亡 Tag

**问题**：`UninitializeAbilitySystem()` 先把 ASC Avatar 置为 null，之后才调用 `HealthComponent->UninitializeFromAbilitySystem()`。后者只在 `ASC->GetAvatarActor() == Owner` 时清理死亡 Tag，所以 `State.Death.Dead` 留在跨 Pawn 持久化的 ASC 上，新 Pawn 仍被死亡父 Tag 阻塞。

**修复**：

重排 `UninitializeAbilitySystem()` 的执行顺序：

1. 移除 Character 对 HealthComponent 的死亡委托
2. `HealthComponent->UninitializeFromAbilitySystem()` —— 此时 ASC Avatar 仍是当前 Pawn，能正确清除死亡 Tag
3. `ClearAbilityInput()` + `CancelAllAbilities()` + `RemovePawnAbilitySets()`
4. `SetAvatarActor(nullptr)` 或 `ClearActorInfo()`
5. `CachedAbilitySystemComponent = nullptr`

旧 Pawn 晚解绑保护分支也增加死亡委托解绑。

---

### P1-4：全局重生计时器破坏每名玩家独立的 3 秒延迟

**问题**：所有 Controller 共用一个 `RespawnTimerHandle`。后死亡的玩家加入已运行的倒计时，晚 2.9 秒死亡的玩家只等 0.1 秒就重生。

**修复**：

1. `RestartPlayerAfterDelay` 签名改为 `void(TWeakObjectPtr<AController> Controller)` —— 接受单个弱引用。

2. `RequestPlayerRespawn` 为每个 Controller 创建独立的 `FTimerDelegate` + `SetTimer`。

3. `PendingRespawnControllers` 类型改为 `TSet<TWeakObjectPtr<AController>>`，仅防重入。

4. 删除全局 `RespawnTimerHandle` 成员。

5. Timer 回调逻辑：
   - 先从 Pending 集合移除
   - 验证 Authority、弱引用有效性
   - `ensureMsgf` 检查 `GetPawn() == nullptr`，非空则拒绝并暴露生命周期错误
   - 调用引擎 `RestartPlayer()`

**新增/改名/删减成员**：

| 成员 | 变更 | 说明 |
|------|------|------|
| `RestartPlayerAfterDelay(TWeakObjectPtr<AController>)` | **签名变更** | 从无参改为接受单个弱引用 |
| `PendingRespawnControllers` | **类型变更** | 从 `TSet<TObjectPtr<>>` 改为 `TSet<TWeakObjectPtr<>>` |
| `RespawnTimerHandle` | **删除** | 每 Controller 独立 Timer 无需全局句柄 |

---

### P1-5：死亡状态快速完成后缺少强制网络更新

**问题**：`StartDeath()` 与 `FinishDeath()` 修改复制属性后未调用 `ForceNetUpdate()`，可能在同一帧内完成两次转换，客户端完全错过 DeathState。

**修复**：`StartDeath()` 和 `FinishDeath()` 在状态修改和委托广播后均调用 `GetOwner()->ForceNetUpdate()`。同时两个函数新增 Authority 守卫。

---

### P2-1：HealthComponent 初始化并非真正幂等

**问题**：同一 ASC 重复传入时不解绑却再次 `AddUObject`，造成重复回调。找不到 VitalSet 时留下 `ASC 非空但未初始化完成` 的半状态。

**修复**：

1. 新增 `BoundASC` 成员追踪已完整绑定的 ASC。
2. `InitializeWithAbilitySystem`：
   - 同一 ASC 且 VitalSet 已完整绑定时直接返回
   - 已有其他绑定时先调用 `UninitializeFromAbilitySystem()`
   - 找不到 VitalSet 时**不保存 ASC**，直接返回

---

### P2-2：HealthComponent 丢失已批准的属性事件出口

**问题**：`HandleHealthChanged/MaxHealthChanged` 是空函数，组件没有 `OnHealthChanged/OnMaxHealthChanged` 委托。UI 或 Pawn 表现没有稳定的组件级监听入口。

**修复**：

1. 新增 `FApecoxHealthAttributeEvent` 六参数委托类型（与 VitalAttributeSet 的同名委托匹配）。
2. 组件上暴露 `OnHealthChanged` 和 `OnMaxHealthChanged` 委托。
3. 两个 Handler 原样转发 AttributeSet 的六参数。

---

### P2-3：客户端死亡回调观察到的 Tag 顺序与服务器不一致

**问题**：`OnRep_DeathState()` 先广播委托，最后才重建 Tag。客户端回调查询 `State.Death` 得到错误状态。

**修复**：`OnRep_DeathState()` 先调用 `ApplyDeathTagsForState()` 重建本地 Tag，再按状态跨度广播 Started/Finished。保留 `NotDead → DeathFinished` 合并复制的重放。

---

### P2-4：死亡 GameplayEvent 的 Target 指向 PlayerState

**问题**：`EventData.Target` 设为 `ASC->GetOwnerActor()`（即 PlayerState），而非死亡角色。

**修复**：改为 `EventData.Target = AbilitySystemComponent->GetAvatarActor()`。

---

### P2-5：DeathAbility 扩大了 ASC 内部 API，且复用状态未在激活时重置

**问题**：
- `CancelAbilitiesByFunc` 从 private 暴露为 public
- `bDeathFinished` 在多次激活间不重置
- 使用 `CommitAbility`（死亡不应受 Cost/Cooldown 限制）
- 缺少 ActorInfo/ASC/Avatar/HealthComponent 验证

**修复**：

1. ASC：`CancelAbilitiesByFunc` 恢复为 private。
2. DeathAbility：使用 GAS 引擎接口 `CancelAbilities(nullptr, nullptr, this)` 取消除自身外的活动 GA。
3. `ActivateAbility` 开始时重置 `bDeathStarted = false` 和 `bDeathFinished = false`。
4. 激活前严格验证 ActorInfo、ASC（Cast to UApecoxAbilitySystemComponent）、Avatar、HealthComponent。缺少任一关键对象时 `ensureMsgf` 并安全结束。
5. 删除 `CommitAbility()` 调用。
6. `FinishDeathAndEndAbility()` 作为 BlueprintCallable 入口防护 `CurrentActorInfo`、Avatar 和 HealthComponent 失效。
7. 新增 `bDeathStarted` 标志。`EndAbility()` 只在 `bDeathStarted && !bDeathFinished` 时才补调用 `FinishDeath`。

**新增/改名/删减成员**：

| 成员 | 变更 | 说明 |
|------|------|------|
| `bDeathStarted` | **新增** | 标记死亡已成功开始，EndAbility 据此决定是否补调用 FinishDeath |
| `CancelOtherAbilitiesAndClearInput()` | **实现变更** | 从 `CancelAbilitiesByFunc` lambda 改为 `CancelAbilities(nullptr, nullptr, this)` |
| `CommitAbility()` 调用 | **删除** | 死亡是系统权威反应，不依赖消耗/冷却 |

---

### P2-6：无效状态成员与报告错误需要清理

**问题**：
- `bDeathTagsApplied` 只写不读
- 使用 `AddLooseGameplayTag`/`RemoveLooseGameplayTag`，计数可能被重复路径污染
- 报告中 "RestartPlayer 会销毁旧 Pawn" 和 "全局批量 Timer 等价于每人延迟三秒" 不成立

**修复**：

1. 删除 `bDeathTagsApplied`。
2. `ClearDeathTags` 和 `ApplyDeathTagsForState` 改用 `SetLooseGameplayTagCount(Tag, 0/1)` 精确管理计数。
3. 本报告及实施报告已同步纠正运行链路和成员说明。

---

## 三、最终死亡/旧 Pawn 解绑/独立计时/新 Pawn 生成顺序

```
Authority 端完整链路：

1. GE 使 Health → 0
   └→ VitalAttributeSet::PreGameplayEffectExecute: 保存快照
   └→ VitalAttributeSet::PostGameplayEffectExecute: 钳制 + 广播 OnOutOfHealth

2. HealthComponent::HandleOutOfHealth (Authority)
   └→ ASC->HandleGameplayEvent(GameplayEvent.Death)
       Target = AvatarActor（当前 Character）

3. DeathAbility::ActivateAbility
   ├→ 重置 bDeathStarted/bDeathFinished
   ├→ 验证 ActorInfo/ASC/Avatar/HealthComponent
   ├→ CancelAbilities(nullptr, nullptr, this) + ClearAbilityInput
   ├→ SetCanBeCanceled(false)
   ├→ HealthComponent->StartDeath()
   │   ├→ DeathState = DeathStarted + ForceNetUpdate
   │   ├→ SetLooseGameplayTagCount(State.Death.Dying, 1)
   │   └→ OnDeathStarted.Broadcast
   ├→ K2_OnDeathStarted
   └→ bAutoFinishDeath → FinishDeathAndEndAbility
       └→ HealthComponent->FinishDeath()
           ├→ DeathState = DeathFinished + ForceNetUpdate
           ├→ SetLooseGameplayTagCount(State.Death.Dying, 0)
           ├→ SetLooseGameplayTagCount(State.Death.Dead, 1)
           └→ OnDeathFinished.Broadcast

4. Character::OnDeathStarted
   └→ StopMovement + Capsule NoCollision

5. Character::OnDeathFinished
   └→ SetActorHiddenInGame(true)
   └→ Authority: SetTimerForNextTick → DestroyDueToDeath

6. Character::DestroyDueToDeath (next-tick)
   ├→ AController* Cached = GetController()
   ├→ DetachFromControllerPendingDestroy()  ← Controller 已无 Pawn
   ├→ SetLifeSpan(0.1f)
   └→ GameMode->RequestPlayerRespawn(PC)

7. GameMode::RequestPlayerRespawn(PC)
   ├→ PendingRespawnControllers.Add(WeakController) [防重入]
   └→ SetTimer(独立 Timer, RespawnDelaySeconds, 不循环)

8. 3 秒后 GameMode::RestartPlayerAfterDelay(WeakController)
   ├→ PendingRespawnControllers.Remove(Controller)
   ├→ 验证: Authority, IsValid(Controller), GetPawn() == nullptr
   └→ RestartPlayer(PC)
       ├→ FindPlayerStart + SpawnDefaultPawn → 新 Character (新 HealthComponent, DeathState=NotDead)
       ├→ Controller->Possess(NewPawn)
       │   └→ NewPawn->PossessedBy → InitializeAbilitySystem
       │       ├→ InitAbilityActorInfo(PS, this)  [同一 PS/ASC]
       │       ├→ GrantPawnAbilitySets
       │       ├→ HealthComponent->InitializeWithAbilitySystem(ASC)
       │       ├→ 绑定死亡委托
       │       └→ ApplyPawnInitializationEffect (恢复满血)
       └→ 旧 Pawn 被 LifeSpan 0.1s 销毁（如尚未销毁）
           └→ EndPlay → UninitializeAbilitySystem
               ├→ 移除死亡委托
               ├→ HealthComponent->UninitializeFromAbilitySystem
               │   └→ ASC Avatar 已是新 Pawn → 不清除死亡 Tag
               └→ 清缓存
```

---

## 四、新增、删除或改名的类、成员变量、函数

### 新增

| 文件 | 名称 | 类型 | 说明 |
|------|------|------|------|
| `ApecoxHealthComponent.h` | `FApecoxHealthAttributeEvent` | 委托 (SixParams) | 组件级属性变化事件，直接匹配 VitalAttributeSet 同名委托 |
| 同上 | `OnHealthChanged` | 成员委托 | 转发 VitalSet::OnHealthChanged |
| 同上 | `OnMaxHealthChanged` | 成员委托 | 转发 VitalSet::OnMaxHealthChanged |
| 同上 | `BoundASC` | TObjectPtr\<UApecoxAbilitySystemComponent\> | 追踪已完整绑定的 ASC，用于幂等判断 |
| `ApecoxDeathAbility.h` | `bDeathStarted` | bool | 标记死亡已成功开始 |
| `ApecoxVitalAttributeSet.h` | `PreGameplayEffectExecute` | 函数覆写 | GE 执行前保存 Health/MaxHealth 旧值 |
| `ApecoxPlayerCharacter.h` | `DestroyDueToDeath` | 函数 | next-tick 销毁、解绑占有、请求重生 |

### 改名

| 文件 | 旧名称 | 新名称 | 说明 |
|------|--------|--------|------|
| `ApecoxGameMode.h` | `RestartPlayerAfterDelay()` | `RestartPlayerAfterDelay(TWeakObjectPtr<AController>)` | 从批量改为单 Controller |
| `ApecoxGameMode.h` | `PendingRespawnControllers` | 类型从 `TSet<TObjectPtr<>>` 改为 `TSet<TWeakObjectPtr<>>` | 使用弱引用 |

### 删除

| 文件 | 名称 | 原因 |
|------|------|------|
| `ApecoxHealthComponent.h` | `bDeathTagsApplied` | 只写不读，幂等由 DeathState 保证 |
| `ApecoxPlayerCharacter.h` | `CachedController` | DestroyDueToDeath 栈内局部变量即够用 |
| `ApecoxGameMode.h` | `RespawnTimerHandle` | 每 Controller 独立 Timer，无需全局句柄 |
| `ApecoxDeathAbility.cpp` | `CommitAbility()` 调用 | 死亡不应受 Cost/Cooldown 影响 |

---

## 五、构建结果

**命令**：
```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

**git diff --check**：✅ 通过（仅有 LF→CRLF 工作树警告，无空白错误）

**构建**：✅ **Succeeded**（0 错误，0 新增警告\*）

> \* 1 个预存弃用警告 C4996：`ApecoxAbilitySystemComponent.cpp` 的 `NonInstanced` API。非本批引入。

**编译轮次**：第 2 次通过（第 1 次 FTimerHandle 临时对象传递错误已修复）

---

## 六、未做操作声明

- ❌ 未创建/修改/删除任何 `.uasset` 或 `.umap`
- ❌ 未操作 UE 编辑器
- ❌ 未执行 `git add`、`git commit`、`git push`、`git stash` 或任何分支操作
- ❌ 未扩展护盾、UI、动画、伤害类型、免疫、击杀消息或比赛规则
- ❌ 未迁移 Lyra 源码
- ❌ 未访问 MCP

---

## 七、Public/Private 目录对称

```
Source/Apecox/
├── Public/
│   ├── Character/ApecoxHealthComponent.h          ✅
│   ├── Character/ApecoxPlayerCharacter.h          ✅
│   ├── AbilitySystem/Abilities/ApecoxDeathAbility.h ✅
│   ├── AbilitySystem/Abilities/ApecoxGameplayAbility.h (无需修改)
│   ├── AbilitySystem/Attributes/ApecoxVitalAttributeSet.h ✅
│   ├── AbilitySystem/ApecoxAbilitySystemComponent.h ✅
│   ├── Game/ApecoxGameMode.h                      ✅
│   └── GameplayTags/ApecoxGameplayTags.h          (无需修改)
├── Private/
│   ├── Character/ApecoxHealthComponent.cpp        ✅
│   ├── Character/ApecoxPlayerCharacter.cpp        ✅
│   ├── AbilitySystem/Abilities/ApecoxDeathAbility.cpp ✅
│   ├── AbilitySystem/Abilities/ApecoxGameplayAbility.cpp (无需修改)
│   ├── AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp ✅
│   ├── Game/ApecoxGameMode.cpp                    ✅
│   └── GameplayTags/ApecoxGameplayTags.cpp        (无需修改)
```

全部对称 ✅

---

**修复完成，等待 Codex 二次审查。**
