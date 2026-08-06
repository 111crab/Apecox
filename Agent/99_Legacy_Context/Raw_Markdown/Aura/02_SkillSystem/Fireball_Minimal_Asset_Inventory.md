# 火球术最小闭环资产与代码盘点

*创建日期：2026-07-07*
*状态：已完成初步盘点，等待用户确认是否交给 Claude 实施。*

## 目的

本文档用于回答一个很具体的问题：V1-A 第一个技能“火球术”是否已经具备足够资产与旧代码基础，可以进入最小配置化实现。

结论：可以。教程项目已经提供了火球术需要的核心资产，第一版不需要用户额外寻找素材。下一步应让 Claude 实现配置技能路径，并优先复用现有火球投射物、伤害 GE、动画、音效和 Niagara。

## 推荐的第一版验证技能

技能名：火球术 / Fireball
定位：鼠标指向投射物，命中后造成火焰伤害。
推荐验证输入：`InputTag.1`。

推荐先用 `InputTag.1` 的原因：
- 当前左键在 `AAuraPlayerController` 中同时承担移动、选中目标和技能触发语义。
- 旧教程 `GA_FireBolt` 可能已经占用某个输入 Tag，如果新配置火球和旧 GA 使用同一输入，会导致一次按键激活两个技能。
- 用 `InputTag.1` 可以先验证新配置技能路径，不破坏旧教程火球和左键移动逻辑。

后续如果用户希望火球回到左键，可以在确认旧教程 GA 下线或迁移后再调整。

## 可复用代码

### 投射物

- `Source/Aura/Public/Actor/AuraProjectile.h`
- `Source/Aura/Private/Actor/AuraProjectile.cpp`

现状：
- `AAuraProjectile` 已经 `bReplicates = true`。
- 服务端通过 `DamageEffectSpecHandle` 对命中的目标 ASC 应用 GE。
- 客户端可以看到复制来的投射物。
- 已有命中音效、飞行循环音效和命中 Niagara 字段。

建议：
- 第一版继续复用 `AAuraProjectile` 和现有 `BP_FireBolt`。
- 不要在第一版重写投射物生命周期。
- 后续再把速度、生命周期、碰撞半径、穿透/弹跳等做成 Delivery 配置。

### 旧教程火球 GA

- `Source/Aura/Public/AbilitySystem/Abilities/AuraProjectileSpell.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraProjectileSpell.cpp`
- `Content/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/GA_FireBolt.uasset`

现状：
- `UAuraProjectileSpell` 负责服务端生成投射物。
- 它从 GA 自身字段读取 `ProjectileClass`、`DamageEffectClass` 和 `DamageTypes`。
- `GA_FireBolt` 是 `UAuraProjectileSpell` 的蓝图子类，并使用 `Event.Montage.FireBolt` 触发投射物生成。

建议：
- 不删除旧 `GA_FireBolt`。
- 新的 `UAuraConfiguredActiveAbility` 可借鉴 `SpawnProjectile` 和 SetByCaller 写法。
- 关键变化是：数据从 `UAuraSkillDefinition` 读取，而不是从每个 GA 蓝图类读取。

### 鼠标目标

- `Source/Aura/Public/AbilitySystem/AbilityTasks/TargetDataUnderMouse.h`
- `Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp`

现状：
- 本地客户端采集鼠标命中点。
- 通过 GAS TargetData 发送到服务端。
- 服务端等待并消费 TargetData。

建议：
- 第一版继续复用该 AbilityTask。
- 目标模式先只支持 CursorHit / MousePoint，不急着做完整 TargetActor 体系。

### 伤害 GE 与执行计算

- `Content/Blueprints/AbilitySystem/GameplayEffects/GE_Damage.uasset`
- `Source/Aura/Public/AbilitySystem/ExecCalc/ExecCalc_Damage.h`
- `Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp`

现状：
- `GE_Damage` 配合 `UExecCalc_Damage`。
- 伤害通过 SetByCaller 写入，例如 `Damage.Fire`。
- `ExecCalc_Damage` 会读取目标抗性、护甲、格挡、暴击等属性。

建议：
- 第一版继续复用 `GE_Damage`。
- `UAuraSkillDefinition` 中保留 `TMap<FGameplayTag, FScalableFloat> DamageTypes`，火球配置为 `Damage.Fire`。

## 可复用资产

### 核心蓝图资产

| 用途 | 资产 |
| --- | --- |
| 旧教程火球 GA，仅作参考 | `Content/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/GA_FireBolt.uasset` |
| 火球投射物蓝图，建议复用 | `Content/Blueprints/AbilitySystem/GameplayAbilities/Fire/FireBolt/BP_FireBolt.uasset` |
| 通用伤害 GE，建议复用 | `Content/Blueprints/AbilitySystem/GameplayEffects/GE_Damage.uasset` |

二进制扫描显示：
- `BP_FireBolt` 是 `/Script/Aura.AuraProjectile` 的蓝图子类。
- `BP_FireBolt` 引用了火球飞行/命中特效和 FireBolt 音效。
- `GA_FireBolt` 是 `/Script/Aura.AuraProjectileSpell` 的蓝图子类。

### 动画

| 用途 | 资产 |
| --- | --- |
| 推荐施法 Montage | `Content/Assets/Characters/Aura/Animations/Abilities/AM_Cast_FireBolt.uasset` |
| 原始施法 Sequence | `Content/Assets/Characters/Aura/Animations/Abilities/Cast_FireBolt.uasset` |

二进制扫描显示：
- `AM_Cast_FireBolt` 引用了 `Event.Montage.FireBolt`。
- `AM_Cast_FireBolt` 引用了 `sfx_FireBolt`。
- 该 Montage 长度约 0.58 秒。

建议：
- 第一版使用 `AM_Cast_FireBolt`。
- `UAuraConfiguredActiveAbility` 可以播放 Montage，并等待 `Event.Montage.FireBolt` 事件后生成投射物。
- 如果 Montage 或事件配置缺失，代码应有 fallback：直接生成投射物并写日志，避免技能完全不可用。

### Niagara / 材质

| 用途 | 资产 |
| --- | --- |
| 推荐火球主体/飞行表现 | `Content/Assets/Effects/Projectiles/FireBolt/NS_Fireball.uasset` |
| 火焰变体 | `Content/Assets/Effects/Projectiles/FireBolt/NS_Fire_1.uasset` 到 `NS_Fire_4.uasset` |
| 推荐命中爆炸 | `Content/Assets/Effects/Projectiles/FireExplosion/NS_FireExplosion1.uasset` |
| 爆炸备选 | `Content/Assets/Effects/Projectiles/FireExplosion/NS_FireExplosion.uasset` |
| 火球材质 | `Content/Assets/Effects/Projectiles/FireBolt/Materials/M_Fireball.uasset` |
| 火焰材质 | `Content/Assets/Effects/Projectiles/FireBolt/Materials/M_Fire.uasset` |

建议：
- 第一版优先复用 `BP_FireBolt` 上已经配置好的 Niagara，避免额外修改表现资产。
- 如需新配置，优先使用 `NS_Fireball` 和 `NS_FireExplosion1`。

### 音效

| 用途 | 资产 |
| --- | --- |
| 施法/发射音效 | `Content/Assets/Sounds/Abilities/FireBolt/sfx_FireBolt.uasset` |
| 飞行循环音效 | `Content/Assets/Sounds/Abilities/FireBolt/sfx_FireBoltHiss.uasset` |
| 命中音效 | `Content/Assets/Sounds/Abilities/FireBolt/sfx_FireBolt_Impact.uasset` |
| 3D 衰减设置 | `Content/Assets/Sounds/Abilities/FireBolt/att_InWorld.uasset` |

建议：
- 第一版继续使用 `BP_FireBolt` 中已有音效配置。
- 如果后续把音效挪进 `UAuraSkillDefinition`，应使用软引用。

### UI 图标

| 用途 | 资产 |
| --- | --- |
| 火球图标 | `Content/Assets/UI/Spells/FireBolt.uasset` |
| 白色火球图标 | `Content/Assets/UI/Spells/FireBolt_white.uasset` |
| 火系背景 | `Content/Assets/UI/Globes/MI_FireSkillBG.uasset` |

建议：
- 第一版运行时可以先不接 UI。
- `UAuraSkillDefinition` 可以预留 `Icon` 软引用字段，但不作为火球闭环必需项。

## 第一版 `DA_Fireball` 建议配置

路径建议：
- `Content/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball.uasset`

字段建议：
- `DisplayName = 火球术`
- `SkillTag = Ability.Fire.Fireball`
- `InputTag = InputTag.1`
- `AbilityClass = UAuraConfiguredActiveAbility`
- `DefaultLevel = 1`
- `TargetingMode = CursorHit`
- `DeliveryMode = Projectile`
- `ProjectileClass = BP_FireBolt`
- `DamageEffectClass = GE_Damage`
- `DamageTypes = { Damage.Fire: 50 }`
- `CastMontage = AM_Cast_FireBolt`
- `MontageSpawnEventTag = Event.Montage.FireBolt`
- `Icon = FireBolt`

## 需要注意的风险

1. **输入重复激活**
   如果旧 `GA_FireBolt` 和新 `DA_Fireball` 都绑定同一个 InputTag，一次按键可能触发两个技能。

2. **蓝图资产引用需要编辑器验证**
   文本扫描能看到引用关系，但最终仍应在 UE 编辑器中打开 `BP_FireBolt`、`GA_FireBolt`、`AM_Cast_FireBolt` 验证字段是否有效。

3. **软引用与第一版加载**
   运行时模型倾向软引用，但第一版可以对少量 Montage/Icon 做同步加载，前提是代码注释说明这是 V1-A 折中。

4. **Listen Server 双表现**
   `AAuraProjectile::OnSphereOverlap` 中服务端和客户端都会播放命中特效/音效，需要 Claude 验证 Listen Server Host 是否出现重复表现。

5. **旧代码中文注释已经存在**
   新代码注释要继续学习型风格，但应避免把显而易见的语法解释写得过多。

## 结论

火球术最小闭环不缺资产。下一步可以让 Claude 实施：

- 新增 `UAuraSkillDefinition`。
- 新增 `UAuraConfiguredActiveAbility`。
- 新增配置技能授予路径。
- 创建或指导创建 `DA_Fireball`。
- 使用现有 `BP_FireBolt`、`GE_Damage`、`AM_Cast_FireBolt`、FireBolt 音效和 Niagara 跑通第一条配置化技能链路。
