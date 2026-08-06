# Apex Phase 能量弹与 ProjectileCast 第一轮代码审查

审查日期：2026-07-29

结论：**编译通过，但存在多项 P1 运行时与多人时序错误，暂不进入 UE 资产配置和 PIE。**

## P1：ActivationGroup 没有接入 GAS 生命周期

文件：

```text
Source/Apex/Public/AbilitySystem/ApexAbilitySystemComponent.h
Source/Apex/Private/AbilitySystem/ApexAbilitySystemComponent.cpp
```

问题：

1. 没有覆写 `NotifyAbilityActivated()` 和 `NotifyAbilityEnded()`。
2. 自定义的 `NotifyAbilityActivated_Group()` / `NotifyAbilityEnded_Group()` 没有任何调用者。
3. `ExclusiveBlocking` 在有 `ExclusiveReplaceable` 活跃时被错误阻止；正确行为是允许激活并取消 Replaceable。
4. 取消函数没有 Ignore Handle；如果真正接到激活通知，新激活的 Replaceable 会把自己一起取消。
5. `SetCanBeCanceled(false)` 没有保护，Replaceable 可以变成不可替换。
6. 枚举缺少隐藏 `MAX`，Data Validation 也没有检查非法值。

影响：当前 `PolicyConfig.ActivationGroup` 只是无效配置，既不阻塞，也不替换其他技能。

## P1：LocationInfo 被错误读取为 HitResult

文件：

```text
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:125
```

Task 创建的是：

```text
FGameplayAbilityTargetData_LocationInfo
```

但 GA 使用 `HasHitResult()` / `GetHitResult()` 读取。LocationInfo 的正确接口是：

```text
HasEndPoint()
GetEndPoint()
```

影响：AimLocation 保持 `FVector::ZeroVector`，投射物会朝世界原点发射。

## P1：Event 与 TargetData 的网络先后顺序不安全

文件：

```text
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:104
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:116
```

只有 `OnAimTargetDataReceived()` 会尝试生成投射物：

- Event 先到、Data 后到：可以进入生成。
- Data 先到、Event 后到：Event 回调不会再次检查缓存 Data，永远不生成。

客户端 Notify 与服务器 Notify 独立运行，TargetData 完全可能先于服务器动画事件到达，因此这不是理论边缘情况。

修复方向：Event 和 Data 回调都调用同一个 `TryExecuteProjectileSpawn()`，只有服务端且两项均就绪时执行一次。

## P1：GA 状态没有重置，并存在永久 Active 路径

文件：

```text
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp
```

问题：

1. `bExecutionEventReceived`、`bAimDataReceived`、`bProjectileSpawned` 和缓存 TargetData 没有在每次激活时重置。
2. 第二次激活时 Event/Data 回调会被旧状态直接忽略。
3. `OnMontageCompleted()` 只写日志，不结束或推进 Ability。
4. Montage 为空时只写日志，Ability 已经 Commit，随后永久 Active。
5. Event 后 Montage 被打断而 Data 尚未到达时，没有明确的收束路径。

影响：第一次或第二次释放就可能卡住技能 Spec，后续无法再次激活。

## P1：服务器没有验证客户端 Aim TargetData

文件：

```text
Source/Apex/Private/AbilitySystem/Tasks/ApexAbilityTask_WaitAimTargetData.cpp
```

问题：

1. 距离钳制只发生在客户端。
2. 服务器没有检查坐标是否为有限值。
3. 服务器没有检查 Avatar 到 AimPoint 的最大距离。
4. Listen Server 本地玩家仍调用 `ServerSetReplicatedTargetData()`，留下不必要的缓存。
5. `OnTargetDataReplicated()` 先 Consume，再广播传入的引用，存在引用失效风险。
6. 没有处理 TargetData Cancelled，失败时远端 GA 可能永久等待。

影响：恶意或错误客户端可以提交任意位置；网络异常时技能无法清理。

## P1：投射物生成与自伤路径缺少运行时保护

文件：

```text
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:179
Source/Apex/Private/CombatEntities/Projectile/ApexProjectile.cpp:75
```

问题：

1. `SourceASC` 没有空指针检查便直接 `MakeEffectContext()`。
2. `ImpactEffectSpecHandle` 无效时仍可完成生成。
3. Spawn Socket 不存在时没有失败处理。
4. `OnProjectileStop()` 直接进入 `HandleImpact()`，没有忽略 Owner/Instigator。
5. 没有通过 `IgnoreActorWhenMoving()` 从移动碰撞中排除释放者。
6. 客户端也执行碰撞回调并修改 `bHasImpacted`，权威边界不够清楚。
7. 初始化函数无法向生成者返回失败，无法按要求销毁半成品。

影响：可能空指针崩溃、生成无伤害投射物、出生时撞到自己并自伤/销毁。

## P2：Data Validation 没有按已批准架构实现

文件：

```text
Source/Apex/Public/AbilitySystem/Shared/ApexSkillExecutionConfig.h
Source/Apex/Private/AbilitySystem/Data/ApexSkillDefinition.cpp
```

问题：

1. 没有实现 `UApexGameplayAbility::ValidateSkillDefinition()`。
2. SkillDefinition 没有委托 GA 模板校验 Montage 和模板专属字段。
3. ActivationGroup 没有验证。
4. `FApexSkillExecutionConfig` 被加入虚函数 `IsValidConfig()`，把校验行为塞进数据结构，偏离“配置是数据、模板负责解释与校验”的边界。
5. 错误信息无法精确指出缺失的 Montage、EventTag、Socket 或 ProjectileDefinition。

修复方向：ExecutionConfig 保持空数据基结构；模板 CDO 实现校验并向 `FDataValidationContext` 添加具体错误。

## P2：受控模板仍可被 Blueprint 激活逻辑绕开

文件：

```text
Source/Apex/Public/AbilitySystem/Abilities/ApexGameplayAbility.h:78
Source/Apex/Private/AbilitySystem/Abilities/Projectile/ApexProjectileCastAbility.cpp:30
```

问题：

1. `GetRequiredExecutionConfigStruct` 被做成 `BlueprintNativeEvent`，没有必要。
2. ProjectileCast 的 Native `ActivateAbility()` 调用了 `Super::ActivateAbility()`；Blueprint 子类仍可能执行 K2 Activate 路径。

修复方向：配置类型和权威执行钩子保持普通 C++ virtual；ProjectileCast 使用 `NotBlueprintable` 或其他明确限制，不调用会进入 K2 Activate 的 Super 路径。

## P2：存在未说明的范围外改动

实际额外修改：

```text
Apex.uproject
Source/Apex/Private/AbilitySystem/ApexAbilitySet.cpp
Source/Apex/Public/AbilitySystem/Shared/ApexSkillExecutionConfig.h
```

判断：

- `Apex.uproject` 启用 StructUtils 是 `TInstancedStruct` 在 UE 5.8 下的必要插件依赖，可以保留，但报告必须说明它超出原 Prompt，是 Prompt 遗漏的必要修正。
- ExecutionConfig 拆成独立头文件合理，但目录建议从模糊的 `Shared` 调整到 `AbilitySystem/Data`。
- `ApexAbilitySet.cpp` 的构造函数改写完全不必要，原 UE 5.8 构造函数合法且此前已编译，应恢复原写法。

报告中的“未超出范围”与实际不符，且未按规范列出完整成员变量、函数签名和暴露方式，需要在修复报告中补齐。

## 可保留部分

- 类职责总体仍符合 `SkillDefinition -> GA Template -> AbilityTask -> Projectile -> GE/Cue`。
- `GameplayEvent.Ability.Execute` Native Tag 命名正确。
- Notify 的 Authority / AutonomousProxy 过滤方向正确。
- ProjectileDefinition 的基础字段和独立资产边界正确。
- Cost/Cooldown 继续走 GAS 原生 GE 和 `CommitAbility()`，方向正确。
- Authority-only Spawn、服务端 GE 应用和 GameplayCue 表现边界方向正确。

## 审查结论

当前不建议用户生成 Rider 工程文件或进入 UE 编辑器配置资产。

下一步：

1. 子代理按 Fix Prompt 修复上述问题。
2. 重新编译。
3. Codex 第二轮代码审查。
4. 通过后向用户提交 C++ 类、成员变量、函数和 Tag 的命名审阅表。
5. 用户确认后再生成 `Current_UE_Manual_Steps.md`。
