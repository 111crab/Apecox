# 当前局部阶段

更新日期：2026-08-10

## 顶层阶段

Phase 1 - 玩家生命周期与 GAS 基线。

已完成并推送：

- Phase 0：`e115f8f chore: establish Apecox project baseline`
- Phase 1A：`86ed380 gas: establish player ASC lifecycle baseline`
- Phase 1B：`eb9e598 gas: establish ability input lifecycle baseline`

## 当前局部任务

Phase 1C：玩家生命、死亡重生、Manny 双视角移动与基础镜头生命周期。

当前状态：**已完成并通过单人、Listen Server 与 Dedicated Server 对应验证，等待 Git 收口。PlayerCameraManager 的 Pitch 钳制生效且重生后保留；完整 Manny 的空手第一人称仍有轻微胸腔透视和跑动观感问题，用户批准延期到第一把步枪双视角表现阶段处理，不阻塞 Phase 1C。**

## 已批准范围

- 一个 `AApecoxPlayerCharacter`、一个 Capsule 和一个 `UCharacterMovementComponent` 负责唯一的移动模拟与网络预测。
- 新增 `FirstPersonMesh`，仅拥有者可见；现有 `GetMesh()` 作为第三人称全身，仅非拥有者可见。
- 新增 `FirstPersonCamera`，采用 UE 5.8 第一人称模板的 Mesh/Camera/FOV 基线。
- 复用 UE 5.8 的 `SKM_Manny_Simple`、`ABP_Unarmed`、`ABP_FP_Copy` 与 `CtrlRig_FPWarp`。
- 新增 `InputTag.Move`、`InputTag.Look.Mouse`、`InputTag.Jump`，复用现有 Native InputAction 配置与绑定入口。
- 本轮支持移动、鼠标观察、跳跃和默认移动速度对应的 Run 动画；第一人称手臂变化来自动画姿态复制，不手写摆动。
- 不新增移动 RPC；继续使用 CMC 原生客户端预测和服务器校正。

Phase 1C 已审查通过的死亡范围继续保留：

- 新建 `UApecoxHealthComponent : UActorComponent`，管理当前 Pawn 的生命监听和死亡状态，不持有 Attribute。
- 新建 `EApecoxDeathState`：`NotDead / DeathStarted / DeathFinished`。
- 新建 `UApecoxDeathAbility : UApecoxGameplayAbility`，由死亡 GameplayEvent 触发。
- `UApecoxVitalAttributeSet` 增加 Health 变化、MaxHealth 变化和 OutOfHealth 委托，但不销毁 Pawn、不请求重生。
- `AApecoxPlayerCharacter` 负责当前身体停止移动、关闭碰撞、解绑 ASC 和销毁。
- `AApecoxGameMode` 只在服务器负责延时 3 秒后 `RestartPlayer`。
- PlayerState、ASC 和 VitalAttributeSet 跨 Pawn 保留；Pawn AbilitySet 随旧 Pawn 撤销并随新 Pawn 重新授予。
- 复活时通过独立的 Instant `PawnInitializationEffect` 恢复 `MaxHealth/Health`。
- 新增稳定 Native GameplayTag：
  - `GameplayEvent.Death`
  - `State.Death`
  - `State.Death.Dying`
  - `State.Death.Dead`
- 测试阶段允许创建 `GE_Apecox_InitializePawnStats`、`GE_Debug_SelfDamage`、`GA_Apecox_Death` 和 `GA_Debug_SelfDamage`。

## 明确不做

- 不加入 Shield、EvolutionProgress、护甲、抗性、伤害类型或正式伤害公式。
- 不加入 `Damage/IncomingDamage` Meta Attribute；调试 GE 暂时直接修改 Health。
- 不加入死亡蒙太奇、布娃娃、HUD、GameplayCue、击杀消息或观战。
- 不迁移 Lyra 的 Experience、GameFeature、GameplayMessageSubsystem 或 PlayerSpawningManager。
- 不增加 `Ability.Behavior.SurvivesDeath`；等出现真实跨死亡 Ability 再设计。
- 不实现枪械、拾取、库存或比赛胜负。

## 当前执行顺序

1. 用户批准 Manny 第一/第三人称共享移动基线。已完成。
2. Codex 形成当前代码设计和 ClaudeCode Prompt。已完成。
3. 用户审阅本轮类成员、函数和 Tag 名称。已完成。
4. 用户将首轮 Prompt 交给 ClaudeCode 实施、构建并提交中文报告。已完成。
5. Codex 首轮审查。已完成；发现 `OwnerNoSee` 源 Mesh 必须使用 `AlwaysTickPoseAndRefreshBones`。
6. ClaudeCode 执行小修 Prompt 并重新构建。已完成。
7. Codex 复审并覆盖 UE 人工操作清单。已完成，复审通过。
8. 用户添加 UE 5.8 First Person 内容包，配置 Manny、AnimBP、IA/IMC，并验证单人和两人 Listen Server 移动表现。
9. 角色基线通过后，立即恢复 Phase 1C 的 GE 死亡、销毁、重生和 Dedicated Server 验证。
10. 移动、双端表现和死亡重生运行验证完成。已完成。
11. 用户批准常驻关闭 Motion Blur。已完成，`DefaultEngine.ini` 已写入对应设置。
12. 用户批准 `AApecoxPlayerCameraManager`、`Camera` 目录与官方 `ViewPitchMin=-70 / ViewPitchMax=80` 基线。已完成。
13. ClaudeCode 按当前 Prompt 实施并完整构建。已完成。
14. Codex 审查。已完成，通过。
15. 用户验证 Pitch 限制、Listen Server 双端独立约束和重生后的 CameraManager 保留。已完成；钳制有效，空手第一人称残余视觉问题已明确延期。

## 成功标准

- 本地拥有者看到第一人称 Manny 姿态，且不会看到重叠的第三人称全身。
- 远端玩家看到第三人称 Manny 的 Idle、Run、Jump/Fall 表现，不看到对方的第一人称 Mesh。
- 移动、观察、跳跃单人正常；Listen Server 主机与 Client 均可移动，双方能正确观察对方。
- 第一人称手臂/身体会随 Idle、Run、Jump 姿态变化；不要求本轮具备武器动画或独立 Sprint。
- Ability Tactical 输入保持原行为。
- Health 只通过 GE 归零一次，并只产生一次权威死亡事件。
- Death Ability 取消其他活动 Ability、清除输入并阻止死亡期间重新激活普通 GA。
- `DeathState` 在服务器、拥有客户端和远端客户端一致；`OnRep` 能处理 `NotDead -> DeathFinished` 的合并复制。
- 旧 Pawn 在死亡完成后的下一帧解绑和销毁，不在 Death Ability 调用栈中撤销自身 AbilitySet。
- 每名玩家每次死亡只创建一个重生计时器和一个新 Pawn。
- 新 Pawn 复用原 PlayerState、ASC 和 VitalAttributeSet，ASC Avatar 指向新 Pawn。
- 旧 Pawn 的输入缓存、活动 Ability、Pawn AbilitySet 和死亡 Tag 不残留。
- 新 Pawn 满生命，Q 可以再次触发调试自伤。
- 单人、Listen Server + Client、Dedicated Server + 两个 Client 全部通过。
- 本地视角限制为官方基线 `-70 ~ 80` 度且重生后继续生效；它只负责视角边界，不承担完整身体近裁剪问题的根治。
- CameraManager 随 PlayerController 保留，死亡重生后 Pitch 约束仍然有效。

## 当前待确认

无。当前设计已经用户全部批准。

## 当前下一步

Git 收口 Phase 1C。随后讨论 Phase 2 第一把步枪纵向切片：武器/装备职责、第一与第三人称表现通道、拾取装备和腰射闭环。
