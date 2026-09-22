# Phase 2B-1 自动步枪腰射 Hitscan 修复复审

复审日期：2026-08-11  
结论：**仍有阻断项，暂不进入 UE 人工配置与 PIE 验证。**

## 发现

### P1：无效 TargetData 分支没有结束 Ability

文件：`Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp`

- `OnTargetDataReady()` 在 `InData.IsValid(0) == false` 时直接 `return`。
- `CurrentActorInfo` 或 ASC 无效时也直接 `return`。
- 非 Owner、非 Authority 的兜底分支同样没有结束。

这与“一次激活 = 一发”的生命周期约定冲突。异常数据会让 Spec 保持 Active，TargetData 委托和当前 Shot 状态也可能残留；Held 输入随后无法重新激活该能力。

### P1：派生类在 GAS ScopeLock 检查之前提前清理

文件：`Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp`

当前 `EndAbility()` 先移除 TargetData 委托、消费 TargetData、清空 `CurrentShotId`，最后才调用 `Super::EndAbility()`。UE 5.8 原生 `UGameplayAbility::EndAbility()` 会先执行：

1. `IsEndAbilityValid()`；
2. `ScopeLockCount > 0` 时加入 `WaitingToExecute`；
3. 只有可以真正结束时才执行清理。

当前顺序会在 Ability 仍处于 ScopeLock 时提前拆掉派生状态，形成“Ability 还 Active，但派生资源已经清理”的半结束状态，也没有满足上一轮修复 Prompt 的明确要求。

### P1：非 Instant Damage GE 仍会被实际应用

文件：`Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp`

代码检测到 `DurationPolicy != Instant` 后只输出 Warning，没有使后续应用条件失效。随后仍会进入 `bHitActor && TargetASC && DamageGE` 分支并调用 `ApplyGameplayEffectSpecToTarget()`。

这与报告中的“拒绝应用并按 Miss 处理”不一致，也可能留下本阶段没有跟踪和移除机制的 Active GE。

### P2：修复报告中的 UE 配置路径与既定授予架构冲突

修复报告最后写了“GA 蓝图创建及 ASC DefaultAbilities 注册”和 `GA_RifleFire`。当前项目的既定路径是：

1. 新建 `UApecoxAbilitySet` DataAsset；
2. 在 `GrantedAbilities` 中直接选择原生 `UApecoxHitscanFireAbility`，输入标签为 `InputTag.Weapon.Fire`；
3. 将该 AbilitySet 配到 `DA_Weapon_Rifle.EquippedAbilitySets`；
4. 装备时以 WeaponInstance 作为 SourceObject 授予，卸下时按句柄撤销。

`UApecoxHitscanFireAbility` 构造函数已经固定 `LocalPredicted + WhileInputActive`，本轮不需要额外创建 GA 蓝图，也不应绕过 Weapon AbilitySet 改用 ASC DefaultAbilities。

## 已确认修复正确的部分

- `CurrentWeaponInstance` 已按 OwnerOnly 复制。
- TargetData 已改为堆分配并交给 Handle 管理。
- 本地 RPM 门控已进入 `CanActivateAbility()`。
- 目标 ASC 已通过 `UAbilitySystemGlobals` 获取，兼容 PlayerState ASC。
- Stage 2 Trace 不再越过意图点或回退 Stage 1 命中。
- Authority 在认可射击后执行 Fire Cue。
- `CommitServerShot()` 已增加 Authority 守卫、内部重检和 bool 返回。
- `StructUtils` 插件/模块依赖已移除，`ModularGameplay` 保留。
- 子代理报告显示 UE 5.8 完整构建通过。

## 当前停止点

完成上述 3 个 P1 修复、修正报告并再次完整构建后，由 Codex 做一次窄范围终审。通过后再覆盖 UE 人工操作清单，进入资产配置和单/多人验证。
