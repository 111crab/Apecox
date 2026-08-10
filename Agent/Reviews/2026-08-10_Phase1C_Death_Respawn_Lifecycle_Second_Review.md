# Phase 1C 二次代码审查：死亡、重生与换 Pawn 生命周期

审查日期：2026-08-10  
结论：**主体修复正确，但仍有 1 个 P1 和 2 个 P2；暂不进入 UE 配置。**

## P1：MaxHealth 连带降低 Health 的事件仍然丢失

文件：`Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`

`PostAttributeChange(MaxHealth)` 已经在 `PostGameplayEffectExecute()` 之前通过
`SetNumericAttributeBase(Health, NewMaxHealth)` 把 Health 压到新上限。因此进入
`PostGameplayEffectExecute()` 的 MaxHealth 分支时，`CurrentHealth > NewValue` 通常已经为 false，
当前 185-215 行的 HealthChanged/OutOfHealth 分支不会执行。

结果：降低 MaxHealth 连带降低 Health 时，服务端组件收不到 HealthChanged；如果 MaxHealth 降到 0，
也不会产生权威死亡事件。

应把 `PostGameplayEffectExecute()` 改为统一收口：

1. 先针对本次被修改属性完成必要钳制。
2. 钳制结束后，用 `HealthBeforeAttributeChange` 与当前 `GetHealth()` 比较并广播 HealthChanged。
3. 用 `MaxHealthBeforeAttributeChange` 与当前 `GetMaxHealth()` 比较并广播 MaxHealthChanged。
4. 不要再次用 `CurrentHealth > NewMaxHealth` 推断是否发生连带变化；GE 前快照已经提供正确依据。

## P2：委托回调后的最终生命状态仍可能被旧缓存覆盖

文件：`Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`

Health 分支先缓存 `NewValue`，广播 OnHealthChanged/OnOutOfHealth 后又直接写
`bOutOfHealth = true`。若未来某个回调实现免死、立即治疗或最后机会效果，Health 已恢复为正数，
当前代码仍会把门控写回 true；后续再次归零时不再广播死亡。

统一广播结束后应使用实时最终值更新：

`bOutOfHealth = (GetHealth() <= 0.0f);`

OutOfHealth 判断也应在 OnHealthChanged 回调之后重新读取 `GetHealth()`，避免回调已救活角色仍触发死亡。
同时只在新旧值实际不同的时候广播 HealthChanged/MaxHealthChanged。

## P2：旧 Pawn 晚解绑分支没有反初始化 HealthComponent

文件：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`

当 ASC Avatar 已经切到新 Pawn 时，旧 Pawn 的 `UninitializeAbilitySystem()` 只移除了 Character 自己的
死亡委托，没有调用 `HealthComponent->UninitializeFromAbilitySystem()`。旧组件会继续绑定持久化
VitalAttributeSet，直到 OnUnregister，期间可能收到属于新 Pawn 的属性事件。

该分支也应调用 `HealthComponent->UninitializeFromAbilitySystem()`。组件内部已有
`ASC Avatar == Owner` 守卫，因此它会解除旧委托，但不会清除新 Pawn 的死亡 Tag。

## 建议顺手修正

- `FinishDeathAndEndAbility()` 找不到 HealthComponent 时应 `ensureMsgf` 并安全结束，不能静默标记
  `bDeathFinished=true`。
- `CancelAbilities` 第二个参数是 `WithoutTags`，不是注释所写的“拥有者过滤”。
- 删除 `PostGameplayEffectExecute()` 中未使用的 `ASC` 局部变量和文件末尾多余空行。
- `RequestPlayerRespawn(APlayerController*)` 对当前“仅玩家、无 AI”原型是可接受的收窄，维持现状即可。

## 已通过

- GE 前快照已移到 `PreGameplayEffectExecute()`。
- 旧 Pawn next-tick 解除占有、0.1 秒销毁和三秒后生成新 Pawn的顺序正确。
- 每名玩家独立 Timer 与弱 Controller 引用正确。
- ASC 死亡 Tag 在清 Avatar 前清理的主路径正确。
- Start/Finish Death 的 Authority、ForceNetUpdate、OnRep Tag 顺序正确。
- HealthComponent 初始化幂等、组件级属性委托、Avatar Event Target 正确。
- DeathAbility 已移除 Commit，并改用 GAS 原生 CancelAbilities。
- Public/Private 目录对称，修复报告中的构建成功，`git diff --check` 无空白错误。
