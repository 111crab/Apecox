# Phase 2B-1 自动步枪腰射 Hitscan 最终审查

审查日期：2026-08-11  
结论：**代码审查通过，可以进入 UE 人工配置与单/多人验证。**

## 最终确认

- Owner Client 已能取得 OwnerOnly 复制的当前 WeaponInstance，并与 AbilitySpec SourceObject 做精确装备校验。
- 自定义 TargetData 使用堆分配并由 `FGameplayAbilityTargetDataHandle` 接管，没有栈地址所有权错误。
- 一次 Ability 激活只处理一发；本地 RPM/弹匣门控位于 `CanActivateAbility()`，完成或拒绝后都会结束。
- TargetData 的异常类型、空数据和兜底网络角色路径均有明确收口。
- 派生 `EndAbility()` 在自定义清理前执行 `IsEndAbilityValid()` 和 ScopeLock 延迟检查；延迟委托绑定派生版本。
- 目标 ASC 通过 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor()` 获取，兼容 Character Avatar 暴露 PlayerState ASC 的项目架构。
- 服务端两阶段 Trace 的最终命中只来自 Gameplay Fire Origin 到 IntentPoint 的第二阶段，不越过意图点，也不回退相机命中。
- `CommitServerShot()` 具备 Authority 守卫、ShotId/RPM/弹匣重检和 bool 事务结果。
- 只有 Instant Damage GE 才能创建并应用伤害 Spec；空或非 Instant GE 均按合法 Miss 处理。
- Owning Client 预测 Fire Cue；Authority 接受事务后执行 Fire Cue；Impact Cue 只使用 Authority 最终 HitResult。
- 武器 GA 仍通过 Weapon AbilitySet 授予，以 WeaponInstance 为 SourceObject；没有引入 ASC DefaultAbilities 或每武器 GA 蓝图旁路。
- `StructUtils` 插件/模块依赖已删除，`ModularGameplay` 保留。

## 构建

子代理报告 UE 5.8 `ApecoxEditor Win64 Development` 增量构建成功：0 错误、0 新增警告。Codex 已核对第二轮实际代码，没有重新重复构建。

## 本阶段保留边界

- V1 使用服务器当前世界重新 Trace，不包含 Server-Side Rewind。
- 弹匣暂未做完整客户端数值预测；极端延迟下，弹匣归零边界可能先出现一次被服务器拒绝的预测表现。
- 服务器当前按请求到达时间验证 RPM；网络抖动容差和窗口化射速审计留到网络模拟专项。
- 本轮没有 GameplayCueNotify/VFX 资产，因此先验证 Cue 调用链、射击事务、伤害、死亡和重生；远端可视表现进入后续表现阶段。

这些是已知 V1 边界，不阻止本轮编辑器验证。
