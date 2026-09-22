# Claude Code 第二轮修复 Prompt：Phase 2B-1 Hitscan 生命周期与 GE 校验

你是 Apecox 项目的具体实施代理。本轮只修复 Codex 终审发现的 3 个代码阻断项和 1 个报告错误，不增加任何玩法功能，不操作 UE 资产、MCP、Git 或 Rider 项目文件。

开始前阅读：

```text
D:/UnrealProject/Apecox/Agent/Reviews/2026-08-11_Phase2B1_Hitscan_Fire_Fix_Review.md
D:/UnrealProject/Apecox/Agent/Reports/2026-08-11_Phase2B1_Hitscan_Fire_Fix_Report.md
D:/UnrealProject/Apecox/Source/Apecox/Private/AbilitySystem/Abilities/Weapons/ApecoxHitscanFireAbility.cpp
E:/UE_5.8/Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/Abilities/GameplayAbility.cpp
```

## 1. 所有 TargetData 终止分支必须结束 Ability

修复 `UApecoxHitscanFireAbility::OnTargetDataReady()`：

- `InData.IsValid(0) == false` 时必须结束当前 Ability。
- `CurrentActorInfo` 或 ASC 无效时也必须尽最大可能安全结束；不得静默留下 Active Spec。
- 非 Owner、非 Authority 的兜底路径也必须结束。
- 保持 invalid type、missing source、commit failure 等现有结束路径。

不要复制散落的清理代码；最终仍统一进入派生类 `EndAbility()`。

## 2. EndAbility 必须先遵守 UE 5.8 的有效性与 ScopeLock 模式

在派生类执行任何委托移除、TargetData 消费或临时状态清理之前：

1. 调用 `IsEndAbilityValid(Handle, ActorInfo)`，无效则直接返回。
2. 如果 `ScopeLockCount > 0`，将**派生类自身**的 `EndAbility` 通过 `FPostLockDelegate::CreateUObject` 加入 `WaitingToExecute`，然后返回。
3. 只有不在 ScopeLock 中时，才执行：移除 TargetData delegate、消费 replicated TargetData、清理 `CurrentShotId`，最后调用 `Super::EndAbility()`。

参考 UE 5.8 原生 `UGameplayAbility::EndAbility()`，但 Waiting delegate 必须绑定 `UApecoxHitscanFireAbility::EndAbility`，不能绑定基类版本，否则延迟执行时会跳过派生清理。

## 3. 非 Instant Damage GE 必须真正拒绝应用

修复 `ProcessAuthoritativeShot()` 的 GE 校验：

- 使用明确布尔值或等价结构表示 `DamageEffectClass` 是否存在且其 CDO 的 `DurationPolicy == Instant`。
- 只有 `bHitActor && TargetASC && bValidInstantDamageEffect` 时才允许构造 Spec、应用 GE 并返回 `ConfirmedHit`。
- 非 Instant GE 必须只输出一次明确 Warning，不得调用 `MakeOutgoingSpec` 或 `ApplyGameplayEffectSpecToTarget`，该发按 `Miss` 确认。
- 空 `DamageEffectClass` 仍是合法 Miss。
- 不增加 Active GE 跟踪、SetByCaller、伤害公式或新配置字段。

## 4. 修正修复报告的人工配置口径

更新：

```text
D:/UnrealProject/Apecox/Agent/Reports/2026-08-11_Phase2B1_Hitscan_Fire_Fix_Report.md
```

删除“创建 GA 蓝图并注册到 ASC DefaultAbilities”的错误建议。正确口径是：

1. 创建 `UApecoxAbilitySet` DataAsset（资产名遵守 `DA_` 前缀）。
2. `GrantedAbilities` 直接选择原生 `UApecoxHitscanFireAbility`，`InputTag = InputTag.Weapon.Fire`。
3. 将 AbilitySet 加入 `/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle` 的 `EquippedAbilitySets`。
4. `DA_Weapon_Rifle.InstanceClass = UApecoxRangedWeaponInstance`。

本轮不创建或修改任何 `.uasset`。

## 5. 构建与静态检查

运行完整构建：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

构建后确认：

- 无效/空 TargetData 不存在直接静默返回的 Active Ability 路径。
- 派生 `EndAbility()` 在任何自定义清理前检查 `IsEndAbilityValid` 和 `ScopeLockCount`。
- 非 Instant Damage GE 不可能进入 `ApplyGameplayEffectSpecToTarget()`。
- 不创建 GA 蓝图，不改用 ASC DefaultAbilities。
- 构建 0 错误；既有无关警告可如实记录。

## 报告与停止条件

在原修复报告末尾增加“第二轮修复”章节，列出实际改动、构建结果和静态检查结果。完成以上内容后停止，等待 Codex 终审。
