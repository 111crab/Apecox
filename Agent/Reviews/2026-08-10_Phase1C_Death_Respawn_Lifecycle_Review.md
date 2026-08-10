# Phase 1C 代码审查：死亡、重生与换 Pawn 生命周期

审查日期：2026-08-10  
结论：**未通过，必须修复后重新审查。当前不要创建 UE 测试资产。**

## P1：VitalAttributeSet 的旧值捕获时机错误，死亡事件可能不触发

文件：

- `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h`
- `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`

当前在 `PreAttributeChange()` 中保存 `HealthBeforeAttributeChange`，随后又在
`PostGameplayEffectExecute()` 中调用 `SetHealth()`。后一次设置仍会经过属性变更回调，
可能把暂存的旧值覆盖为已经修改后的值，最终让 `OldValue > 0` 不成立，
`OnOutOfHealth` 无法稳定广播。

应采用 GAS/Lyra 的标准边界：

1. 覆写 `PreGameplayEffectExecute()`，在整个 GE Modifier 执行前保存 Health 与 MaxHealth。
2. `PreAttributeChange()` 只负责约束最终值，不再承担 GE 旧值快照。
3. `PostGameplayEffectExecute()` 完成钳制后，统一比较快照与最终值，再广播
   `OnHealthChanged`、`OnMaxHealthChanged` 和首次 `OnOutOfHealth`。
4. MaxHealth 降低并连带压低 Health 时，也必须进入相同的 HealthChanged/OutOfHealth 判断。

## P1：旧 Pawn 未解除占有，RestartPlayer 不会生成新 Pawn

文件：

- `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
- `Source/Apecox/Private/Game/ApecoxGameMode.cpp`

`OnDeathFinished()` 只隐藏旧 Pawn、设置 5 秒 LifeSpan 并请求重生；Controller 在 3 秒后仍然
占有旧 Pawn。UE 5.8 的 `AGameModeBase::RestartPlayer()` 不会自动销毁已有 Pawn：
当 `Controller->GetPawn()` 非空时，它会继续使用并重新 Possess 该 Pawn。因此当前实现会复用
已经隐藏、禁用移动、关闭碰撞且 Health 为 0 的旧 Pawn，而不是创建新 Pawn。

修复要求：

1. `OnDeathFinished()` 使用 next-tick 回调进入 `DestroyDueToDeath()`，避免在死亡委托栈内拆 Pawn。
2. Authority 在该流程中保存 Controller、调用 `DetachFromControllerPendingDestroy()`，
   对旧 Pawn 执行短 LifeSpan（约 0.1 秒）并隐藏。
3. 重生倒计时可以在死亡完成时开始，但 `RestartPlayer()` 到期前必须保证 Controller 已无 Pawn。
4. GameMode 到期时若 Controller 仍有 Pawn，应以 `ensure` 暴露生命周期错误并拒绝错误复用，
   不要假设 `RestartPlayer()` 会替项目销毁旧 Pawn。

## P1：ASC 解绑顺序会遗留死亡 Tag

文件：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`

`UninitializeAbilitySystem()` 先把 ASC Avatar 设为 null，之后才调用
`HealthComponent->UninitializeFromAbilitySystem()`。后者为了避免旧 Pawn 清理新 Pawn 状态，
只在 `ASC->GetAvatarActor() == Owner` 时清理死亡 Tag；因此当前顺序会使
`State.Death.Dead` 留在跨 Pawn 持久化的 PlayerState ASC 上，新 Pawn 仍被死亡父 Tag 阻塞。

应按以下顺序执行：先移除 Character 对 HealthComponent 的委托并反初始化 HealthComponent，
再清输入、取消 GA、移除 Pawn AbilitySet，最后清空 ASC Avatar/ActorInfo。

## P1：全局重生计时器破坏每名玩家独立的 3 秒延迟

文件：

- `Source/Apecox/Public/Game/ApecoxGameMode.h`
- `Source/Apecox/Private/Game/ApecoxGameMode.cpp`

当前所有 Controller 共用一个 `RespawnTimerHandle`。后死亡的玩家会加入第一名玩家已经运行的
倒计时，例如晚 2.9 秒死亡的玩家可能 0.1 秒后就重生。

应为每名 Controller 建立独立倒计时。推荐让定时回调接收
`TWeakObjectPtr<AController>`，并使用 Pending 集合仅对同一 Controller 防重入；回调到期时先从
Pending 移除，再校验弱引用、Authority 与 `GetPawn() == nullptr`，最后调用 `RestartPlayer()`。

## P1：死亡状态快速完成后缺少强制网络更新

文件：`Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

`StartDeath()` 与 `FinishDeath()` 修改复制属性后均未调用 Owner 的 `ForceNetUpdate()`。
首版中两次转换在同一帧完成，旧 Pawn 随后又要被快速销毁；没有强制更新时，客户端可能完全
错过 DeathState，无法稳定重放死亡回调。两个状态转换在广播后都应调用
`GetOwner()->ForceNetUpdate()`，并保留 `NotDead -> DeathFinished` 的合并复制处理。

## P2：HealthComponent 初始化并非真正幂等

文件：`Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

当同一个 ASC 被重复传入时，当前实现不会解绑，却会再次 `AddUObject()` 三个委托，造成重复回调。
相同且已经完整初始化时应直接返回；其他情况先反初始化，再绑定新 ASC/VitalSet。

## P2：HealthComponent 丢失了已批准的属性事件出口

文件：

- `Source/Apecox/Public/Character/ApecoxHealthComponent.h`
- `Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

当前 `HandleHealthChanged()` 和 `HandleMaxHealthChanged()` 是空函数，组件本身也没有
`OnHealthChanged` / `OnMaxHealthChanged` 委托。这样未来 UI 或 Pawn 表现若不直接依赖 AttributeSet，
就没有稳定的组件级监听入口；这与已批准的“HealthComponent 作为 AttributeSet 到 Pawn/UI 的翻译层”
不一致。应在组件上暴露并转发这两个原生事件，继续保证组件不保存第二份数值。

此外，类声明遗漏了已批准的 `meta=(BlueprintSpawnableComponent)`。应使用合法的 UCLASS meta 写法恢复，
而不是因为第一次 UHT 写法错误就删除该能力。

## P2：客户端死亡回调观察到的 Tag 顺序与服务器不一致

文件：`Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

Authority 在广播死亡委托前应用 Tag，但 `OnRep_DeathState()` 先广播委托、最后才重建 Tag。
客户端回调若查询 `State.Death` 会得到错误状态。应先 `ApplyDeathTagsForState()`，再按照状态跨度
广播 Started/Finished。

## P2：死亡 GameplayEvent 的 Target 指向 PlayerState 而非当前 Avatar

文件：`Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

ASC 的 OwnerActor 是 PlayerState，当前 `EventData.Target` 因而不是死亡角色。应使用
`AbilitySystemComponent->GetAvatarActor()`；PlayerState 仍由 ASC ActorInfo 表达，不应冒充事件目标。

## P2：DeathAbility 扩大了 ASC 内部 API，且复用状态未在激活时重置

文件：

- `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
- `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp`

仅为了“取消除自身外的 Ability”，实现把接收 `TFunction` 的
`CancelAbilitiesByFunc()` 从 private 暴露为 public。这不是稳定的领域接口；GAS 已提供
`CancelAbilities(nullptr, nullptr, this)`。应恢复 helper 的 private 可见性，DeathAbility 使用引擎接口。

同时应在每次 `ActivateAbility()` 开始时把 `bDeathFinished` 重置为 false，并校验 ActorInfo、Avatar
与 HealthComponent；缺失关键组件时应明确失败并结束，不能继续广播蓝图死亡事件。
`FinishDeathAndEndAbility()` 作为 BlueprintCallable 入口也必须防护 `CurrentActorInfo`。

## P2：无效状态成员与报告错误需要清理

- `bDeathTagsApplied` 只写不读，不能提供报告所称的幂等保护；幂等来自 DeathState，应删除该成员。
- 对组件自己拥有的死亡 Tag，优先使用 `SetLooseGameplayTagCount(Tag, 0/1)` 明确设置计数，
  避免 Add/Remove 计数被重复路径污染。
- 实施报告中“RestartPlayer 会销毁旧 Pawn”“全局批量 Timer 等价于每人延迟三秒”均不成立，
  修复后必须同步纠正运行链路、成员说明、风险与自检结论。

## 已通过的部分

- Public/Private 目录保持对称。
- HealthComponent、DeathAbility、GameMode 的总体职责边界正确。
- Native GameplayTag 命名与父子层级合理。
- `DeathState` 使用 RepNotify，并考虑了 `NotDead -> DeathFinished` 的复制合并。
- 普通 GA 通过父标签 `State.Death` 统一阻止激活的方向正确。
- 出生属性使用 Authority 上的 Instant GE，未把初始值写死在 AttributeSet 构造函数。
- Dedicated Server 路径没有访问相机、音频、HUD 或 LocalPlayer。
- 本批构建已通过，`git diff --check` 未发现空白错误；运行时链路仍需完成上述修复后再验证。
