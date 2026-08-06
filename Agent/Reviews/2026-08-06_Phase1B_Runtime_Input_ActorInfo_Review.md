# Phase 1B 首次运行问题审查

审查日期：2026-08-06

结论：蓝图中的 `Wait Input Release` 连接正确；当前失败来自一项 UE 输入资产配置错误和一项 C++ ActorInfo 状态误判。

## 1. Q 按下后立刻 Release

当前 `UApecoxInputComponent` 绑定为：

- `ETriggerEvent::Triggered` -> Ability Input Pressed
- `ETriggerEvent::Completed` / `Canceled` -> Ability Input Released

但 `IA_Ability_Tactical` 又配置了瞬时 `Pressed` Trigger。UE 5.8 中 `UInputTriggerPressed` 只在输入跨过阈值的一帧返回 `Triggered`；持续按住时，下一帧返回 `None`，于是 Action 立即产生 `Completed`。这与实际观察完全一致。

用户已明确：IA 只负责物理输入，不允许绑定“瞬发、按住、蓄力、松发”等技能激活方式。同一个技能未来可能因角色配置而采用不同触发行为。

因此修复为：

- IA 和 IMC Mapping 的 Trigger 均保持空。
- C++ 改用 `Started -> Pressed`、`Completed/Canceled -> Released`，只采集一次物理按下边沿和一次释放边沿。
- 瞬发、按住期间生效、松开发射、蓄力分档等规则属于 GA 流程或每次技能授予的配置，不属于 IA。

这也避免默认 Digital Action 在按住期间每帧 `Triggered` 时重复派发 GAS `InputPressed` Generic Event。

当前 `ActivationPolicy` 位于 GA CDO，只能描述 GA 类级默认策略；它尚不能单独满足“同一个 GA 类按角色采用不同触发方式”。该能力应在后续技能配置设计中通过每次授予/Spec 可读取的配置解决，本轮不临时扩充枚举或字段。

## 2. ASC AvatarActor ensure

日志中的真实项目错误是：

```text
[Apecox] ASC AbilitySystemComponent has unexpected AvatarActor ApecoxPlayerState_0 when BP_ApecoxPlayerCharacter_C_0 is binding.
```

UE 5.8 的 `UAbilitySystemComponent::InitializeComponent()` 会默认执行 `InitAbilityActorInfo(Owner, Owner)`。ASC 挂在 PlayerState 时，角色正式绑定前的 `Owner=PlayerState, Avatar=PlayerState` 是正常过渡状态，不应触发 ensure。

修复：`AApecoxPlayerCharacter::InitializeAbilitySystem()` 显式接受 `ExistingAvatar == ApecoxPS`，随后正常调用 `InitAbilityActorInfo(ApecoxPS, this)`；仍保留对未知 Avatar 和旧 Character Avatar 的防护。

## 3. 日志其余项目

- `aqProf.dll`、`VtuneApi*.dll`：可选性能分析器 DLL 未安装，非项目故障。
- `No GameplayCueNotifyPaths`：当前没有限定 GameplayCue 搜索目录，功能可运行，但项目开始建立 Cue 目录时应在 `DefaultGame.ini` 配置，避免大型项目扫描整个 `/Game`。
- `MVVMK2Node_* parent not found`、缺少编辑器 SVG、`r.MotionVectorSimulation`：UE 5.8 编辑器/插件级警告，与本轮 GAS 输入失败无关。
- `DeleteFile recovered during retry`：重试已成功，无需处理。

## 4. 当前验证状态

代码修复复审结果：**通过，可以进入 UE 修改与复测。**

- `UApecoxInputComponent` 已改为 `Started + Completed/Canceled`，未把技能玩法固化到 IA。
- `InitializeAbilitySystem()` 只放行 `ExistingAvatar == ApecoxPS` 这一合法默认过渡状态，旧 Character 与未知 Avatar 防护仍保留。
- 本轮没有新增公开 API、枚举、GameplayTag，也没有扩大到其他源码。
- 子代理 `-NoLink` 编译结果为 `Succeeded`。

Phase 1B 尚未通过运行验证，不能暂存或提交。用户仍需清空 IA/IMC Trigger、完成 Rider 完整链接，并重新执行单人和两人 Listen Server 验证。
