# 火球术运行链路说明

日期：2026-07-08  
状态：V1-A 已实现并完成当前最小验证  
对应资产：`Content/Blueprints/AbilitySystem/SkillDefinitions/DA_Fireball.uasset`

## 这份文档解决什么问题

火球术是方案 C 的第一个验证技能：少量通用 C++ Ability + `UAuraSkillDefinition` 配置资产 + 原生机制片段。

这份文档按真实运行顺序解释：玩家输入如何触发技能，技能如何读取配置，如何采集鼠标目标，如何播放 Montage，如何在服务端生成投射物，如何计算伤害，最后如何更新血量、飘字和表现。

后续每个代表技能都应在实现前先写类似文档，确认思路后再写代码。火球术这份是“事后补齐版”，用于学习和后续对照。

## 玩家视角流程

1. 玩家按下绑定到 `InputTag.1` 的技能键。
2. 角色触发配置化火球 Ability。
3. 本地采集鼠标下的命中点，作为火球飞行方向。
4. 播放施法 Montage。
5. Montage 中的 GameplayEvent Notify 触发 `Event.Montage.FireBolt`。
6. 服务端在 Notify 时机生成火球投射物。
7. 火球 Actor 复制到其他客户端，其他玩家能看到火球。
8. 火球碰撞到目标后，服务端应用伤害 GameplayEffect。
9. `ExecCalc_Damage` 结合伤害类型、抗性、护甲、格挡、暴击等计算最终伤害。
10. `UAuraAttributeSet` 扣除目标生命，触发受击反应和伤害飘字。
11. 投射物命中时播放命中特效和音效。

## 关键配置

火球术不是单独写死一个 `UGameplayAbility` 类，而是通过 `DA_Fireball` 配置出来。

核心字段来自 `UAuraSkillDefinition`：

| 字段 | 当前火球术用途 |
| --- | --- |
| `SkillTag` | 技能身份，例如 `Ability.Fire.Fireball` |
| `AbilityClass` | 指向通用 Ability：`UAuraConfiguredActiveAbility` |
| `InputTag` | 输入触发 Tag，目前使用 `InputTag.1` |
| `DefaultLevel` | 授予技能时的默认等级 |
| `CastMontage` | 施法 Montage，当前复用教程火球相关 Montage |
| `MontageSpawnEventTag` | Montage Notify 发出的事件 Tag：`Event.Montage.FireBolt` |
| `TargetingMode` | 当前 V1-A 使用鼠标命中点：`CursorHit` |
| `DeliveryMode` | 当前使用投射物：`Projectile` |
| `ProjectileClass` | 火球投射物蓝图/类，继承自 `AAuraProjectile` |
| `DamageEffectClass` | 伤害 GameplayEffect，最终进入 `ExecCalc_Damage` |
| `DamageTypes` | 当前主要写入 `Damage.Fire` 的 SetByCaller 数值 |

配置类位置：

- `Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`
- `Source/Aura/Private/AbilitySystem/Data/AuraSkillDefinition.cpp`

## 新增和复用类说明

### `UAuraSkillDefinition`

文件：`Source/Aura/Public/AbilitySystem/Data/AuraSkillDefinition.h`  
父类：`UPrimaryDataAsset`

作用：

1. 表示一个技能的可编辑配置资产。
2. 保存输入、技能类、Montage、投射物、伤害 GE、伤害类型等数据。
3. 未来技能编辑器主要编辑这个资产，而不是生成新的 C++ 类。

为什么继承 `UPrimaryDataAsset`：

1. 它适合作为可被 Asset Manager 管理的“主数据资产”。
2. 后续技能很多时，可以走资产扫描、软引用、异步加载。
3. 技能编辑器保存的结果可以落到这种数据资产或类似配置资产上。

### `UAuraConfiguredActiveAbility`

文件：

- `Source/Aura/Public/AbilitySystem/Abilities/AuraConfiguredActiveAbility.h`
- `Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp`

父类：`UAuraGameplayAbility`  
间接父类：`UGameplayAbility`

作用：

1. 作为“通用主动技能 Ability”。
2. 激活时从 `AbilitySpec.SourceObject` 取回 `UAuraSkillDefinition`。
3. 根据配置执行目标采集、Montage、投射物生成、伤害 Spec 创建等流程。

它的设计重点不是“只服务火球术”，而是验证以后多个配置技能能共享一套运行框架。

### `UAuraAbilitySystemComponent`

文件：

- `Source/Aura/Public/AbilitySystem/AuraAbilitySystemComponent.h`
- `Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`

父类：`UAbilitySystemComponent`

作用：

1. 授予旧教程 Ability。
2. 授予新的 `UAuraSkillDefinition` 配置技能。
3. 处理输入 Tag 到 AbilitySpec 的匹配和激活。

新增重点函数：

| 函数 | 作用 |
| --- | --- |
| `AddCharacterSkillDefinitions` | 遍历配置技能资产，创建 `FGameplayAbilitySpec`，把 SkillDefinition 放入 `SourceObject`，把输入 Tag 放入 `DynamicAbilityTags`，然后 `GiveAbility` |
| `AbilityInputTagHeld` | 玩家按住/触发输入时，寻找拥有对应 `InputTag` 的 AbilitySpec 并 `TryActivateAbility` |

### `AAuraCharacterBase`

文件：

- `Source/Aura/Public/Character/AuraCharacterBase.h`
- `Source/Aura/Private/Character/AuraCharacterBase.cpp`

父类：`ACharacter`  
接口：`IAbilitySystemInterface`、`ICombatInterface`

作用：

1. 角色基类。
2. 保存旧 `StartupAbilities` 和新 `StartupSkillDefinitions`。
3. 在 `AddCharacterAbilities` 中把两套技能都授予 ASC。

### `UTargetDataUnderMouse`

文件：

- `Source/Aura/Public/AbilitySystem/AbilityTasks/TargetDataUnderMouse.h`
- `Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp`

父类：`UAbilityTask`

作用：

1. 在本地客户端读取鼠标命中点。
2. 把 `FGameplayAbilityTargetDataHandle` 通过 GAS 复制给服务端。
3. 服务端收到后广播 `ValidData`，让 Ability 继续执行。

这是火球术“客户端意图 -> 服务端权威”的关键桥梁。

### `AAuraProjectile`

文件：

- `Source/Aura/Public/Actor/AuraProjectile.h`
- `Source/Aura/Private/Actor/AuraProjectile.cpp`

父类：`AActor`

作用：

1. 表示飞行中的火球投射物。
2. `bReplicates = true`，由服务端生成并复制给客户端。
3. 命中时播放命中特效/音效。
4. 服务端命中目标后应用 `DamageEffectSpecHandle` 中保存的伤害 GE Spec。

### `UExecCalc_Damage`

文件：

- `Source/Aura/Public/AbilitySystem/ExecCalc/ExecCalc_Damage.h`
- `Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp`

父类：`UGameplayEffectExecutionCalculation`

作用：

1. 读取 GE Spec 中的 SetByCaller 伤害。
2. 根据伤害类型匹配目标抗性。
3. 结合护甲、护甲穿透、格挡、暴击等属性计算最终伤害。
4. 把结果写入 MetaAttribute：`IncomingDamage`。

### `UAuraAttributeSet`

文件：

- `Source/Aura/Public/AbilitySystem/AuraAttributeSet.h`
- `Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp`

父类：`UAttributeSet`

作用：

1. 保存生命、法力、抗性、护甲等属性。
2. 属性通过 RepNotify 同步给客户端。
3. `PostGameplayEffectExecute` 处理 `IncomingDamage`，扣血、死亡、受击反应和飘字。

## 从输入到技能激活

### 1. 输入绑定

文件：`Source/Aura/Private/Player/AuraPlayerController.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `SetupInputComponent` | `AAuraPlayerController` | 通过 `UAuraInputComponent::BindAbilityActions` 把输入动作绑定到 Pressed / Released / Held 回调 |
| `AbilityInputTagPressed` | `AAuraPlayerController` | 左键输入会进入目标/移动判断；非左键技能可直接交给 ASC |
| `AbilityInputTagHeld` | `AAuraPlayerController` | 在目标锁定或 Shift 施法时，把输入 Tag 传给 `UAuraAbilitySystemComponent` |
| `AbilityInputTagReleased` | `AAuraPlayerController` | 释放输入时通知 ASC，并处理左键短按移动 |

火球术当前通过 `InputTag.1` 走技能触发，不走左键移动逻辑。

### 2. ASC 根据 InputTag 激活 Ability

文件：`Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `AbilityInputTagHeld` | `UAuraAbilitySystemComponent` | 遍历 `GetActivatableAbilities()`，找到 `DynamicAbilityTags` 中包含当前 InputTag 的 AbilitySpec |
| `TryActivateAbility` | `UAbilitySystemComponent` | GAS 原生函数，按 SpecHandle 激活对应 Ability |

关键点：

1. 输入键不直接写死在 Ability 类里。
2. 输入 Tag 被存在 `FGameplayAbilitySpec.DynamicAbilityTags`。
3. 这样同一个 `UAuraConfiguredActiveAbility` 可以通过不同 SkillDefinition 绑定到不同按键。

## 技能授予链路

文件：`Source/Aura/Private/Character/AuraCharacterBase.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `AddCharacterAbilities` | `AAuraCharacterBase` | 在服务端把旧 StartupAbilities 和新 StartupSkillDefinitions 都授予 ASC |

文件：`Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `AddCharacterSkillDefinitions` | `UAuraAbilitySystemComponent` | 为每个 SkillDefinition 创建 `FGameplayAbilitySpec` |

`AddCharacterSkillDefinitions` 做了几件关键事：

1. 校验 `SkillDefinition` 不为空。
2. 校验 `AbilityClass` 和 `InputTag` 有效。
3. 创建 `FGameplayAbilitySpec(SkillDef->AbilityClass, SkillDef->DefaultLevel)`。
4. 设置 `AbilitySpec.SourceObject = SkillDef`。
5. 设置 `AbilitySpec.DynamicAbilityTags.AddTag(SkillDef->InputTag)`。
6. 调用 `GiveAbility(AbilitySpec)`。

`SourceObject` 是配置化路线的核心：同一个 GA 类被授予多次时，可以通过不同 SourceObject 区分“这是火球术、治疗术、陷阱术”等。

## Ability 内部运行链路

核心文件：`Source/Aura/Private/AbilitySystem/Abilities/AuraConfiguredActiveAbility.cpp`

### 1. 激活前校验

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `CanActivateAbility` | `UAuraConfiguredActiveAbility` | 通过 Handle 从 ASC 查找 AbilitySpec，再从 `SourceObject` 取 `UAuraSkillDefinition`，检查必需配置是否有效 |

为什么不用 `GetCurrentAbilitySpec()`：

在 `CanActivateAbility` 阶段，引擎不保证当前 Ability 实例已经绑定到 Spec，所以代码使用传入的 `Handle + ASC->FindAbilitySpecFromHandle()` 来查找 Spec。

### 2. 激活入口

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `ActivateAbility` | `UAuraConfiguredActiveAbility` | 缓存 SkillDefinition，调用 `CommitAbility`，创建 `UTargetDataUnderMouse` 等待鼠标目标数据 |

`CommitAbility` 当前负责 GAS 标准的激活提交。后续如果加入消耗和冷却，也会进入这条链路。

### 3. 鼠标目标采集

文件：`Source/Aura/Private/AbilitySystem/AbilityTasks/TargetDataUnderMouse.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `Activate` | `UTargetDataUnderMouse` | 本地控制端直接采集；非本地端等待客户端复制来的 TargetData |
| `SendMouseCursorData` | `UTargetDataUnderMouse` | 本地客户端读取鼠标命中点，封装成 `FGameplayAbilityTargetData_SingleTargetHit`，通过 `ServerSetReplicatedTargetData` 发送给服务端 |
| `OnTargetDataReplicatedCallback` | `UTargetDataUnderMouse` | 服务端收到客户端 TargetData 后广播 `ValidData` |

### 4. TargetData 回到 Ability

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `OnTargetDataReceived` | `UAuraConfiguredActiveAbility` | 从 TargetData 提取命中位置，并根据客户端/服务端角色执行不同逻辑 |
| `ExtractTargetLocationFromData` | `UAuraConfiguredActiveAbility` | 使用 `HasHitResult()` / `GetHitResult()` 提取命中位置，避免写死 SingleTargetHit 类型 |

客户端路径：

1. 播放 Montage，作为本地表现。
2. 不生成投射物，不应用伤害。

服务端路径：

1. 决定是否需要等待 Montage Notify。
2. 如果配置了 `MontageSpawnEventTag`，先注册 `WaitGameplayEvent`。
3. 再启动 Montage。
4. Notify 到达时生成投射物。
5. 如果 Montage 或 Notify 失败，按容错策略 fallback。

## Montage 和 AnimNotify 链路

火球术当前用 Montage 作为施法表现和投射物生成时机。

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `PrepareCastMontageTaskIfConfigured` | `UAuraConfiguredActiveAbility` | 加载 `CastMontage` 软引用，创建 `UAbilityTask_PlayMontageAndWait`，绑定完成/中断回调，但暂不启动 |
| `StartPreparedCastMontageTask` | `UAuraConfiguredActiveAbility` | 调用 `ReadyForActivation()` 真正启动 Montage |
| `OnMontageSpawnEvent` | `UAuraConfiguredActiveAbility` | 收到 `Event.Montage.FireBolt` 后生成投射物 |
| `OnMontageCompleted` | `UAuraConfiguredActiveAbility` | Montage 正常结束但没收到 Notify 时，fallback 生成投射物 |
| `OnMontageInterrupted` | `UAuraConfiguredActiveAbility` | 区分 Montage 启动失败和真正战斗中断 |

为什么要“先注册 WaitGameplayEvent，再启动 Montage”：

如果 AnimNotify 很靠前，先播放 Montage 再注册事件监听，服务端可能漏掉 Notify。当前实现先注册等待，再启动 Montage，降低漏事件风险。

当前 Notify 类型要求：

1. Montage 轨道上需要能发送 GameplayEvent。
2. Event Tag 必须匹配 `DA_Fireball.MontageSpawnEventTag`。
3. 当前火球术使用 `Event.Montage.FireBolt`。

## 投射物生成链路

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `ExecuteProjectileSpawn` | `UAuraConfiguredActiveAbility` | 只在服务端生成投射物，计算发射方向，创建 GE Spec，写入 SetByCaller 伤害，赋给 Projectile |

关键点：

1. 投射物只由服务端生成。
2. 客户端看到投射物，是因为 `AAuraProjectile::bReplicates = true`。
3. 投射物生成前会创建 `DamageEffectSpecHandle`。
4. 伤害数值通过 `AssignTagSetByCallerMagnitude` 写入 Spec。
5. 如果 `DamageEffectClass` 无效或 Spec 创建失败，投射物会被销毁，避免带着坏 Spec 进入碰撞。

## 投射物命中和表现

文件：`Source/Aura/Private/Actor/AuraProjectile.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `BeginPlay` | `AAuraProjectile` | 设置寿命，绑定球体碰撞事件，播放循环飞行音效 |
| `OnSphereOverlap` | `AAuraProjectile` | 命中目标时播放 Impact 音效/Niagara；服务端对目标 ASC 应用伤害 Spec |
| `Destroyed` | `AAuraProjectile` | 投射物销毁时补播命中特效/音效，并停止循环音效 |

当前表现来源：

1. 施法表现：`CastMontage`。
2. 飞行表现：投射物蓝图/类自身资源。
3. 命中表现：`AAuraProjectile` 中的 `ImpactEffect`、`ImpactSound`。
4. 循环音效：`LoopingSound` 和 `LoopingSoundComponent`。

注意：

当前还没有把 Niagara、音效、GameplayCue 抽象进 `UAuraSkillDefinition`。这一步会留给后续表现系统或技能编辑器阶段。

## 伤害计算链路

### 1. 投射物应用 GE

文件：`Source/Aura/Private/Actor/AuraProjectile.cpp`

命中时：

```text
AAuraProjectile::OnSphereOverlap
-> UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor)
-> TargetASC->ApplyGameplayEffectSpecToSelf(*DamageEffectSpecHandle.Data.Get())
```

### 2. GE 进入 ExecCalc

文件：`Source/Aura/Private/AbilitySystem/ExecCalc/ExecCalc_Damage.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `Execute_Implementation` | `UExecCalc_Damage` | 读取 SetByCaller 伤害，结合目标抗性和双方属性计算最终伤害 |

当前关键规则：

1. `Damage.Fire` 来自 `DA_Fireball.DamageTypes`。
2. 未配置的伤害类型按 `0` 处理，不再输出 SetByCaller Error。
3. 目标抗性会降低对应伤害。
4. 护甲、护甲穿透、格挡、暴击会继续修正伤害。
5. 最终伤害写入 `IncomingDamage`。

### 3. AttributeSet 扣血和后续效果

文件：`Source/Aura/Private/AbilitySystem/AuraAttributeSet.cpp`

| 函数 | 类 | 作用 |
| --- | --- | --- |
| `PostGameplayEffectExecute` | `UAuraAttributeSet` | 处理 `IncomingDamage`：清零 MetaAttribute、扣血、判断死亡、触发受击 Ability、显示伤害数字 |
| `ShowFloatingText` | `UAuraAttributeSet` | 通过攻击者的 `AAuraPlayerController` 显示伤害飘字 |
| `OnRep_Health` | `UAuraAttributeSet` | Health 从服务端复制到客户端时触发属性变更通知，供 UI 更新 |

UI 现状：

1. 血量 UI 依赖属性复制和已有 WidgetController 链路，不是火球术单独控制。
2. 伤害数字通过 `AAuraPlayerController::ShowDamageNumber_Implementation` 显示。
3. 技能图标、冷却、消耗、技能栏 UI 当前还没有接入 `UAuraSkillDefinition`。

## 网络责任划分

### 客户端负责

1. 接收玩家输入。
2. 采集鼠标命中点作为意图。
3. 把 TargetData 发送给服务端。
4. 播放本地 Montage 表现。
5. 接收服务端复制来的投射物、属性变化和表现结果。

### 服务端负责

1. 授予技能。
2. 校验技能配置。
3. 接收 TargetData。
4. 决定何时生成投射物。
5. 创建和应用伤害 GE Spec。
6. 执行伤害计算和属性修改。
7. 复制投射物、属性、死亡等结果。

### Listen Server 注意点

Host 同时具备“本地客户端”和“服务端”身份，所以火球术中加入了幂等保护：

| 标志 | 作用 |
| --- | --- |
| `bHasProcessedTargetData` | 防止同一次激活重复处理 TargetData |
| `bProjectileSpawned` | 防止重复生成投射物 |
| `bWaitingForMontageSpawnEvent` | 标记服务端正在等待 Montage Notify |
| `bStartingMontagePlayback` | 区分 Montage 启动失败和真正中断 |
| `bMontageStartupFailed` | 让调用方知道启动阶段同步失败，应 fallback |

## 失败和容错路径

| 失败点 | 当前处理 |
| --- | --- |
| `SourceObject` 不是 `UAuraSkillDefinition` | `CanActivateAbility` 或 `ActivateAbility` 阶段拒绝/结束 |
| `ProjectileClass` 缺失 | `CanActivateAbility` 拒绝激活 |
| `DamageEffectClass` 缺失 | `CanActivateAbility` 拒绝激活 |
| Montage 未配置 | 服务端直接生成投射物 |
| Montage 软引用未加载 | 尝试同步加载；失败则直接生成投射物 |
| Notify 丢失或 Tag 配错 | Montage 完成时 fallback 生成投射物 |
| Montage 真正中断 | 如果投射物未生成，则取消技能 |
| GE Spec 创建失败 | 销毁 deferred projectile，结束 Ability |
| TargetData 类型不是 HitResult | 无法提取目标位置时结束 Ability |
| 未配置某些伤害 SetByCaller | 按 0 处理，不输出错误日志 |

## 当前未完成或后续要补

1. 技能冷却和消耗还没有配置化接入。
2. 技能图标、技能栏、冷却 UI 还没有接入 `UAuraSkillDefinition`。
3. 施法音效、命中特效、GameplayCue 尚未从 SkillDefinition 统一配置。
4. Dedicated Server 没有完整验收。
5. 当前 Targeting 只验证了鼠标命中点；范围、方向、锁定目标、区域选择还没做。
6. 当前 Delivery 只验证了投射物；瞬发、自身 Buff、区域持续、召唤/陷阱还没做。
7. 当前通用 Ability 已经偏长，后续第二、第三个技能验证后应考虑拆成更清晰的机制片段。

## 给后续技能的预设计提醒

下一个技能在写代码前，应先回答：

1. 它的输入 Tag 是什么？
2. 它是否需要 TargetData？需要哪种 TargetData？
3. 它是否需要 Montage？投放时机是否依赖 AnimNotify？
4. 它的 Gameplay 结果在哪里产生？必须是服务端还是可以纯客户端表现？
5. 它使用 GE 还是直接生成 Actor？如果使用 GE，SetByCaller Tag 是什么？
6. 它的表现资源从哪里来？Montage、Niagara、音效、UI 分别怎么接？
7. Listen Server 下是否可能重复执行？
8. 失败路径如何收口？

