# Phase 1C 最终代码审查：死亡、重生与换 Pawn 生命周期

审查日期：2026-08-10
结论：**代码审查通过，可以进入 UE 资产配置与运行验证。**

## 最终通过项

- `PreGameplayEffectExecute()` 正确保存 GE 前 Health/MaxHealth 快照。
- `PostGameplayEffectExecute()` 在钳制后统一比较最终属性；MaxHealth 连带压低 Health 时不再漏事件。
- 生命归零只由 Authority 产生一次 `GameplayEvent.Death`，客户端 OnRep 不产生权威死亡。
- HealthComponent 不保存第二份生命值，负责 AttributeSet 到 Pawn/UI 事件和死亡状态的翻译。
- DeathState、Dying/Dead Tag、`ForceNetUpdate()` 和合并复制重放的顺序正确。
- DeathAbility 不依赖 Cost/Cooldown，使用 GAS 原生取消接口并保留自身。
- 旧 Pawn 在死亡完成后的下一 Tick 解除占有并短延迟销毁，Controller 在重生时已经没有 Pawn。
- GameMode 为每名玩家建立独立三秒计时，不会批量提前复活。
- HealthComponent 在主解绑和旧 Pawn 晚解绑路径都解除 VitalAttributeSet 委托，不干扰新 Avatar。
- ASC/AttributeSet 继续由 PlayerState 持有；新 Pawn 重新绑定同一 ASC，并通过独立 Instant GE 恢复出生属性。
- Public/Private 目录对称；三轮修复报告的最终构建成功；`git diff --check` 无空白错误。

## 非阻塞剩余风险

当前 `PostGameplayEffectExecute()` 在调用 `OnOutOfHealth` 前缓存了
`bShouldBeOutOfHealth`。如果未来某个监听者专门在 `OnOutOfHealth` 回调内部直接把 Health 恢复为正数，
回调返回后门控仍会暂时使用回调前值。本阶段没有复活、免死或最后机会机制，当前死亡/重生原型不受影响。

未来引入此类机制时，将最后一行改为基于回调后的实时值即可：

```cpp
bOutOfHealth = (GetHealth() <= 0.0f);
```

该项记录为后续健康/护盾管线扩展时的技术债，不阻塞 Phase 1C 验证。

## 验证入口

用户只需执行：

`Agent/00_Coordination/Current_UE_Manual_Steps.md`

先验证单人，再验证 Listen Server，最后验证 Dedicated Server。运行验证通过前不提交 Git。
