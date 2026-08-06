# GAS 技能运行时风险清单

*创建日期：2026-07-07*  
*用途：给 Codex、Claude 和后续对话压缩后的恢复使用。凡是修改 GAS 技能运行时、Montage、AnimNotify、投射物、TargetData、网络同步相关代码，都应先读这个文件。*

## 这份清单解决什么问题

V1-A 火球术最小闭环中连续暴露了多个问题：有些能编译通过，但运行时会因为 GAS 生命周期、AbilityTask 回调顺序、Montage 播放失败、AnimNotify 缺失、无效 GE Spec、Listen Server 双路径回调等细节出错。

因此后续不能只检查“代码能不能编译”，还必须检查“异步链路在成功、失败、中断、配置错误、网络角色差异下是否都能收口”。

## 已吸取的经验

1. **CanActivateAbility 不依赖 GetCurrentAbilitySpec()**
   - `CanActivateAbility` 阶段，引擎未必已经把当前 `FGameplayAbilitySpec` 绑定到 Ability 实例。
   - 需要用传入的 `Handle + ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle()` 找到 Spec。
   - 这是 `AbilitySpec.SourceObject` 配置化路线的基础安全点。

2. **软引用判断使用 IsNull，而不是只用 IsValid**
   - `TSoftObjectPtr::IsValid()` 只表示资产已经加载且对象有效。
   - 资产路径已配置但尚未加载时，`IsValid()` 可能是 false。
   - 配置是否存在应先用 `!IsNull()`，再 `LoadSynchronous()` 或后续改 AssetManager 异步加载。

3. **PlayMontageAndWait 的创建成功不等于 Montage 真正播放成功**
   - `CreatePlayMontageAndWaitProxy()` 只是创建 AbilityTask。
   - 真正播放发生在 `ReadyForActivation()` 之后的 Task 激活流程中。
   - 播放失败通常通过 `OnCancelled` 回调暴露，而不是通过工厂函数返回值暴露。
   - 因此调用方在播放后继续访问状态前，必须确认 Ability 没有被同步结束，或显式区分“启动失败”和“战斗中断”。

4. **等待 AnimNotify / GameplayEvent 时，监听应尽量先注册**
   - 如果 Notify 很靠前，先播放 Montage 再注册 `WaitGameplayEvent` 有概率漏掉事件。
   - 需要等待 Montage 事件的服务端逻辑，应优先创建监听，再启动 Montage，或者保证 Notify 不在首帧附近。
   - 如果由于现有代码结构必须先播放，也必须提供 completed fallback，避免永久等待。

5. **所有异步等待都必须有退出路径**
   - 至少覆盖：成功事件、Montage 完成 fallback、Montage 中断取消、Ability 结束清理。
   - 不能出现“配置错误后 Ability 永远不结束”的路径。
   - 如果等待的是资产、任务、网络数据、事件，必须说明失败时怎么收口。

6. **投射物只由服务端生成**
   - 客户端只负责输入意图和本地表现。
   - 伤害、治疗、投射物 Actor、GE 应用必须由权威端决定。
   - 否则会出现作弊风险、重复生成、客户端和服务端世界状态不一致。

7. **传给投射物的 GE Spec 必须有效**
   - `AAuraProjectile::OnSphereOverlap` 会使用 `DamageEffectSpecHandle.Data`。
   - 如果 `DamageEffectClass` 为空或 `MakeOutgoingSpec()` 失败，不能把坏 Spec 传给投射物。
   - 失败时应销毁 deferred projectile，并结束 Ability。

8. **TargetData 类型不能假设只有 SingleTargetHit**
   - 配置化技能后续会支持方向、范围、ActorTarget、区域等多种 TargetData。
   - 读取命中信息时优先用 `HasHitResult()` / `GetHitResult()` 虚函数，不做不安全的 `static_cast`。

9. **伤害等级使用当前 Ability 等级**
   - `UAuraSkillDefinition::DefaultLevel` 只是授予时的默认值。
   - 运行时计算伤害、SetByCaller、ScalableFloat 应使用 `GetAbilityLevel()`。

10. **Listen Server 要防重复回调**
    - Host 同时有客户端路径和服务端路径。
    - TargetData、Notify、Montage 回调都要考虑是否可能被同一次激活触发多次。
    - 必须有类似 `bHasProcessedTargetData`、`bProjectileSpawned` 的幂等保护。

11. **未配置的 SetByCaller 伤害类型应按 0 处理**
    - 一个技能通常只配置自己实际使用的伤害类型，例如火球术只写入 `Damage.Fire`。
    - `ExecCalc_Damage` 可以统一遍历所有伤害类型和抗性，但不能把未配置的类型当成错误。
    - 读取可选伤害类型时使用 `Spec.GetSetByCallerMagnitude(DamageTypeTag, false, 0.f)`，避免 GAS 输出无意义的 SetByCaller Error。

## 异步技能改动前检查

给 Claude 的 Prompt 或 Codex 的审查中，涉及 AbilityTask / Montage / TargetData / 网络时，必须显式回答这些问题：

1. 这个技能的权威结果在哪里产生？客户端有没有产生 Gameplay 结果？
2. 每个 AbilityTask 的成功、失败、取消、Ability 结束清理路径是什么？
3. 如果 Montage 资源没配、路径失效、播放失败，会怎么处理？
4. 如果 AnimNotify 缺失、Tag 配错、事件没收到，会怎么处理？
5. 如果 Montage 被眩晕、死亡、位移、切换状态打断，会怎么处理？
6. Listen Server 下是否会重复处理 TargetData 或重复生成效果？
7. 所有外部配置类、资产、GE、投射物类、ASC、CombatInterface 是否都有空值校验？
8. 这个逻辑在客户端、服务端、模拟代理上分别会执行哪些部分？
9. 是否存在“编译通过但运行时等待永远不结束”的路径？
10. 是否需要在中文注释或报告里解释为什么这样处理？

## 当前 V1-A 火球术验证状态

截至 2026-07-07，火球术配置化运行时已经完成当前 V1-A 最小闭环验证：

1. 单人 PIE 下技能授予、输入触发、Montage 事件、投射物生成、命中和伤害流程正常。
2. Listen Server 下客户端释放火球，Host 能看到投射物和效果。
3. Listen Server 下 Host 释放火球，客户端能看到投射物和效果。
4. 伤害没有出现明显重复应用。
5. `ExecCalc_Damage` 已修复未配置 SetByCaller 伤害类型导致的日志 Error。
6. `AuraEditor Win64 Development` 已通过编译。

仍需留意的后续风险：

1. 当前验证是 Listen Server 级别，不等于 Dedicated Server 完整验收。
2. Montage 被眩晕、死亡、切换状态等战斗中断打断时的策略还没有做成完整通用规则。
3. 后续加入范围、持续、召唤、陷阱类技能时，需要继续扩展 TargetData 和生命周期清单。

## 后续文档约定

每发现一类“编译能过但运行时可能错”的问题，都补充到本文件。不要只写在聊天里，也不要只写在一次性审查结果里。
