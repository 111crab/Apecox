# Montage 与 GameplayEvent 触发点决策

日期：2026-07-08  
状态：已与用户讨论并采用  
适用范围：所有需要“动画某一帧触发玩法逻辑”的技能

## 决策结论

如果一个技能的玩法时机必须绑定到动画某一帧，例如发射投射物、打开命中窗口、应用伤害、开放连招窗口，那么仍然需要在 Montage 时间轴中添加 AnimNotify / GameplayEvent Notify。

这一步不会因为 `UAuraSkillDefinition` 增加或减少字段而消失，因为“哪一帧触发”属于动画资产自身的信息。

## 职责划分

```text
Montage Notify 负责：在动画时间轴的哪一帧发事件。
SkillDefinition 负责：这个技能关心哪个事件，事件到了以后做什么。
```

普通表现类通知，例如音效、脚步、纯 VFX，可以继续使用普通 AnimNotify。它们不需要通知 ASC，也不需要 Ability 等待。

Ability 关键触发点，例如 `SpawnProjectile`、`ApplyEffect`、`OpenHitWindow`，使用 GameplayEvent Notify 更合适，因为 Ability 可以通过 `UAbilityTask_WaitGameplayEvent` 等待对应事件。

## Tag 类型不要混淆

GameplayTag 在 GAS 中有多种用途，但含义不同：

| 类型 | 示例 | 含义 |
| --- | --- | --- |
| 输入 Tag | `InputTag.1` | 玩家输入用于激活哪个 AbilitySpec |
| 状态 Tag | `State.Skill.Channeling` | ASC 当前拥有的持续状态 |
| 事件 Tag | `Event.Montage.Cast.Release` | 一次性 GameplayEvent 的事件名 |
| SetByCaller Tag | `SetByCaller.Healing` | GE Spec 中查找运行时数值的 key |

Montage 中的 GameplayEvent Notify 不是“授予 ASC 一个 Tag”，而是向 ASC 发送一次性事件。Ability 事先等待这个事件，收到后继续执行。

## V1 采用方式

为了减少简单技能的配置负担，V1 采用以下规则：

1. 简单 Montage 技能可以使用默认事件：`Event.Montage.Cast.Release`。
2. 如果 `SkillDefinition` 没有显式填写 Montage 事件 Tag，后续通用实现可以 fallback 到默认事件。
3. 复杂技能必须显式配置事件 Tag，例如多段伤害、连招窗口、命中窗口开关。
4. 技能编辑器阶段应扫描 Montage 中已有的 GameplayEvent Notify，并提示用户选择，而不是要求手打 Tag。

## 对当前技能的影响

火球术当前使用显式事件 `Event.Montage.FireBolt`，保持不变。

生命涌动本轮不使用 Montage，它的触发时机来自输入释放和引导时长，因此不需要 Montage Notify。

后续新增依赖 Montage 的技能时，再考虑把 `MontageSpawnEventTag` 逐步升级为更通用的 `TriggerPoint` / `EffectApplicationTiming` 模型。
