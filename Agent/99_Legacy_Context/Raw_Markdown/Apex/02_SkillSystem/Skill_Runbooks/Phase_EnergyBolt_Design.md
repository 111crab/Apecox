# Phase 能量弹预设计与运行链路

更新日期：2026-07-29  
状态：C++、命名与最终编译审查已通过，等待 UE 资产配置和 PIE 验证。

## 1. 技能定位

“Phase 能量弹”是 Apex 第一个真实技能样本，也是第一个 ProjectileCast 模板验证技能。

它的目标不是追求复杂玩法，而是一次打通：

- AbilitySet 输入授予。
- SkillDefinition 配置。
- GA 成本与冷却。
- Montage 关键帧。
- 屏幕中心瞄准。
- GAS TargetData 网络传输。
- 服务器权威投射物。
- GameplayEffect 伤害。
- GameplayCue 命中表现。

## 2. 玩家体验

```text
按 Q
-> Phase 播放能量释放动作
-> 动作关键帧从手部生成能量弹
-> 能量弹朝屏幕中心飞行
-> 命中另一名玩家造成 20 伤害
-> 命中角色或世界播放冲击效果
```

临时规则：

| 项目 | 数值/行为 |
|---|---|
| 输入 | Q / Skill1 |
| 法力消耗 | 10 |
| 冷却 | 1.5 秒 |
| 伤害 | 20 |
| 瞄准距离 | 10000 cm |
| 投射物速度 | 2000 cm/s |
| 最大寿命 | 3 秒 |
| 释放中移动 | 允许 |
| 命中次数 | 1 |

## 3. 配置资产关系

```text
DA_AbilitySet_Phase_Core
    -> DA_Skill_Phase_EnergyBolt
        |- CommonConfig
        |   |- GE_Phase_EnergyBolt_Cost
        |   `- GE_Phase_EnergyBolt_Cooldown
        |- PolicyConfig
        |   `- ActivationGroup = ExclusiveReplaceable
        |- PresentationConfig
        |   `- AM_Apex_Phase_EnergyBolt
        `- ProjectileCastConfig
            |- GameplayEvent.Ability.Execute
            |- Spawn Socket
            `- DA_Projectile_Phase_EnergyBolt
                |- BP_Apex_Projectile_Phase_EnergyBolt
                |- GE_Phase_EnergyBolt_Damage
                `- GameplayCue.Ability.Phase.EnergyBolt.Impact
```

根技能资产负责组装，投射物资产负责生成后的世界实体行为。

`ExclusiveReplaceable` 只描述能量弹与其他排他技能的并发关系，不限制角色移动。执行关键帧前 GA 可以被取消；投射物生成后 GA 与投射物生命周期分离。

## 4. 运行链路

### 4.1 输入与激活

```text
IA_Skill1 Started
-> AApexPlayerCharacter::InputAbilityTagPressed
-> UApexAbilitySystemComponent::AbilityInputTagPressed
-> UApexAbilitySystemComponent::ProcessAbilityInput
-> TryActivateAbility
-> UApexProjectileCastAbility::ActivateAbility
```

### 4.2 成本、冷却与动画

```text
UApexProjectileCastAbility
-> GetSkillDefinition()
-> 读取 CommonConfig / PresentationConfig / ProjectileCastConfig
-> CommitAbility()
-> 应用 Mana Cost GE
-> 应用 Cooldown GE
-> PlayMontageAndWait
-> WaitGameplayEvent
```

### 4.3 关键帧与瞄准

```text
AM_Apex_Phase_EnergyBolt
-> UApexAnimNotify_SendGameplayEvent
-> GameplayEvent.Ability.Execute
-> UApexProjectileCastAbility::OnExecutionEventReceived
-> UApexAbilityTask_WaitAimTargetData::ConfirmAimTarget
-> 屏幕中心反投影与 Visibility Trace
-> FGameplayAbilityTargetData_LocationInfo
-> PredictionKey + CallServerSetReplicatedTargetData
```

### 4.4 服务器生成投射物

```text
Server receives TargetData
-> 校验 AimPoint 与最大距离
-> 从 SpawnSocket 读取世界位置
-> 计算朝向 AimPoint 的旋转
-> 创建 Damage GE Spec
-> SpawnDeferred AApexProjectile
-> InitializeProjectile
-> FinishSpawning
```

### 4.5 命中结算

```text
AApexProjectile::HandleImpact
-> 仅服务器处理第一次命中
-> 忽略 Owner / Instigator
-> 查找目标 ASC
-> Target ASC ApplyGameplayEffectSpecToSelf
-> Health 100 -> 80
-> Execute Impact GameplayCue
-> Destroy Projectile
```

## 5. 关键职责

| 类型 | 职责 |
|---|---|
| `UApexSkillDefinition` | 单个技能组装入口。 |
| `UApexProjectileCastAbility` | 按下、Commit、动画、事件、TargetData、生成这一段施法生命周期。 |
| `UApexAbilityTask_WaitAimTargetData` | 本地获取瞄准点并通过 GAS 预测键发给服务器。 |
| `UApexProjectileDefinition` | 投射物速度、寿命、碰撞、命中 GE 和 Cue。 |
| `AApexProjectile` | 世界中的复制 Actor，负责飞行、碰撞和命中。 |
| Cost/Cooldown/Damage GE | 属性修改与持续时间数据。 |
| GameplayCue | 命中视觉/声音表现，不参与伤害。 |
| Montage Notify | 决定动画哪一帧推进 GA。 |

`UApexProjectileCastAbility` 固定上述生命周期，并只提供少量受控 C++ 钩子。普通能量弹直接使用模板；将来扇形多发等可复用流程变体才派生 GA，追踪、穿透和命中后生成法术场则留给 CombatEntity。

## 6. Phase 资产候选

### 动画

首选源 Montage：

```text
/Game/ParagonPhase/Characters/Heroes/Phase/Animations/Primary_Attack_A_Medium_Montage
```

复制并改名：

```text
/Game/Blueprints/Characters/Phase/Animation/Montages/AM_Apex_Phase_EnergyBolt
```

### Socket

人工比较：

```text
Muzzle_01
FX_Hand_R1
```

### VFX

```text
P_PhaseProjectileRibbons
P_PhasePrimaryHandR
P_PhasePrimaryCastR
P_PhasePrimaryImpact
P_PhasePrimaryHitWorld
```

第三方资产不直接修改；Montage 副本、Projectile Blueprint、GE 和 Cue 全部进入 `/Game/Blueprints`。

## 7. 为什么目标选择和投射物命中是两层

施法瞄准只回答：

```text
玩家想把投射物射向哪里？
```

投射物碰撞回答：

```text
投射物飞行途中真正碰到了什么？
```

因此屏幕中心 TargetData 不直接决定受伤者。服务端投射物的真实碰撞才决定命中和伤害，避免客户端直接声称“我命中了某人”。

## 8. 为什么不使用 SetByCaller

第一版三个 GE 都使用固定资产数值：

- Cost GE：Mana -10。
- Cooldown GE：Duration 1.5。
- Damage GE：Health -20。

这足以验证生命周期和网络，不需要为临时参数增加 `SetByCaller.*` 技术键。等等级曲线、法术强度和抗性模型确定后，再设计长期动态数值入口。

## 9. 验证完成的定义

只有同时满足以下条件才算技能完成：

- 单人下输入、成本、冷却、Montage、投射物、世界命中表现正确。
- Listen Server 和 Client 都能释放。
- 其他端能看到释放动作、投射物和命中表现。
- 服务器只有一个权威投射物。
- 命中另一玩家后 Health 正确同步为 80。
- 释放者不会被自己的投射物伤害。
- 配置缺失或模板类型错误能在 Data Validation 或运行日志中明确指出。

### 实测结果（2026-07-29）

- 单人 PIE：通过。
- Listen Server + Client PIE：通过。
- Montage、关键帧生成、飞行、世界/玩家命中、GameplayCue、20 点伤害、Mana、Cooldown 和多人复制均正常。
- 第一版 Phase 能量弹运行时闭环完成。

## 10. 后续扩展验证

普通能量弹完成后，下一批不会立即增加更多普通弹。推荐依次验证：

1. 穿透或分裂投射物：检查局部 CombatEntity 行为如何复用。
2. 黑洞投射物/法术场：检查 Projectile 到 AreaField 的转换和特殊交互边界。

这两个样本会检验当前模板是否真的可扩展，而不是只对一个能量弹有效。
