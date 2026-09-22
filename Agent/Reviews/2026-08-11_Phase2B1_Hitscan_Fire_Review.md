# Phase 2B-1 自动步枪腰射 Hitscan 代码审查

审查日期：2026-08-11  
结论：**构建通过，但存在 6 个阻断问题；暂不进入 UE 人工配置和运行验证。**

## P1 阻断问题

### 1. Owning Client 无法通过“当前装备实例”校验

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.cpp:74
Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h:147
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp:311
```

`IsSourceWeaponCurrentlyEquipped()` 在所有端比较 `Equipment->GetCurrentWeaponInstance() == SourceWeapon`，但 `CurrentWeaponInstance` 目前只是 Authority 运行时引用，没有复制；Owning Client 上该值为 `nullptr`。LocalPredicted Ability 会在发送预测激活前失败。

修复方向：把 `CurrentWeaponInstance` 作为 Owner-only 私有装备身份复制，仍不用于远端表现；远端继续只看 `EquippedWeaponState` 摘要。

### 2. TargetDataHandle 接管了栈对象地址

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:169
```

UE 5.8 的 `FGameplayAbilityTargetDataHandle::Add()` 明确要求参数由 `new` 创建，内部使用 `TSharedPtr` 接管并释放。当前传入 `&ShotData`，它是栈变量；函数退出后既可能悬空，也可能被共享指针错误释放。

修复方向：为 Handle 分配 `new FApecoxHitscanShotTargetData(...)`，之后只通过 Handle 管理生命周期。

### 3. Ability 不结束，且 RPM 门控位置会制造预测激活流量

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:51
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:214
```

Owning Client 直接发送 TargetData，但不调用本地 TargetData 回调；Authority 处理完成后也没有调用 `EndAbility()`。因此 Spec 会保持 Active，`WhileInputActive` 无法再次激活，自动射击不能成立。错误 TargetData 分支同样可能永久留下 delegate/Active Ability。

另外 RPM 检查位于 `ActivateAbility()` 内：ASC 每帧仍会预测激活并向服务器建立 Ability，再在内部立刻结束/拒绝，造成不必要的网络请求。

修复方向：

- 在 `UApecoxHitscanFireAbility::CanActivateAbility()` 对本地控制端执行 RPM/弹匣门控。
- Local TargetData 使用与 Lyra 相同的统一回调入口；Client 在发送后结束，Authority 在结算/拒绝后结束。
- `OnTargetDataReady()` 建立 `FScopedPredictionWindow`，所有失败路径都必须安全结束并清理 delegate/Replicated TargetData。
- 把 `CommitAbility()` 调整到本地门控之后、服务端 TargetData 验证阶段，不能在等待远端数据前无条件提交。

### 4. 玩家命中无法找到 PlayerState 上的 ASC

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:436
```

Apecox 的既定架构是 PlayerState 拥有 ASC、Character 实现 `IAbilitySystemInterface`。`HitActor->FindComponentByClass<UAbilitySystemComponent>()` 只搜索 Character 自身组件，所以玩家角色会被判断为没有 ASC，伤害 GE 永远不会应用。

修复方向：使用 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor)`（或等价的 AbilitySystemInterface 路径），并把 WeaponInstance 加入 Damage EffectContext 的 SourceObject。

### 5. 第二阶段 Trace 失败后错误回退相机命中

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:582
```

当前第二段从 Gameplay Fire Origin 沿目标方向继续完整 `MaxRange`，可能越过相机意图点打中后方对象；若第二段未命中，又直接返回第一段相机命中，等于绕过了“枪口被遮挡/物理弹道未命中”的检查。

修复方向：第二段只检测到 `IntentPoint`（最多加极小终点余量）；第二段未命中就返回 Miss，绝不回退第一段命中。

### 6. Remote Client 收不到 Fire Cue

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:154
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:341
```

Fire Cue 只在 Owning Client 本地执行。Authority 认可射击后没有执行 Fire Cue，因此不会向其他客户端复制；实施报告中的“Remote Client 收到 Fire Cue”与代码不符。

修复方向：普通客户端预测执行一次，Authority 在接受并扣弹后执行一次；两者使用同一 Ability PredictionKey/Scoped Prediction Window 让 Owning Client 去重。Listen Host 只走 Authority 执行，避免双播。

## P2 应同步修复

### 7. 不必要地启用了已弃用 StructUtils 插件

位置：

```text
Apecox.uproject:25
Source/Apecox/Apecox.Build.cs:11
```

UE 5.8 的 `TInstancedStruct` 头和实现已经位于 CoreUObject；旧 `StructUtils` 插件标记 `DeprecatedEngineVersion = 5.5`。本项目不应为了该字段启用实验性弃用插件。`UControllerComponent` 仍需要 `ModularGameplay`，应保留。

### 8. 服务端弹匣提交接口缺少 Authority 防线

位置：

```text
Source/Apecox/Private/Weapons/ApecoxRangedWeaponInstance.cpp:204
```

`CommitServerShot()` 只靠调用者约定，没有检查 Outer Actor Authority，也没有再次保护弹匣下界。修复时应加入 Authority guard，并让 Commit 在内部重检后返回成功/失败，防止误调用产生负弹药。

### 9. 瞄准夹角计算应钳制 Dot

位置：

```text
Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp:416
```

在调用 `Acos` 前把 DotProduct 钳制到 `[-1, 1]`，避免量化/浮点误差产生 NaN 并绕过比较。

### 10. 人工配置报告缺少 InstanceClass

首把步枪的 `DA_Weapon_Rifle` 必须把 `InstanceClass` 设置为 `ApecoxRangedWeaponInstance`，否则仍会创建通用 `UApecoxWeaponInstance`，Hitscan GA 会拒绝。首轮报告未列出该关键步骤，并使用了不存在的 `BP_RifleDefinition` 名称。

## 保留项

- Definition + `TInstancedStruct` + RangedWeaponInstance 的分层方向正确。
- 自定义 TargetData 字段和 NetSerialize 结构基本正确。
- Shot ID、客户端/服务端独立射速账本的职责方向正确。
- `UControllerComponent` 承载本地未确认射击与 Client RPC 合理；保留 `ModularGameplay`。
- Native GameplayTag 的职责边界正确。

