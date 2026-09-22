# M1-D 自由移动人机实施报告

日期：2026-09-14

## 结果

完成第一轮人机闭环代码：地图放置的机器人可由服务器 AI Controller 在 NavMesh 内自由移动，继续使用现有 Apecox PlayerState ASC、生命值、实体子弹伤害和死亡能力；死亡后清理旧 AI 生命周期，3 秒后在原始放置点生成满生命新实例并继续漫游。

当前 AI 不搜索或攻击玩家，不创建 Behavior Tree、Blackboard 或 AI Perception 资产。积分、GameState 比赛结果和胜利 HUD 留在本轮人工验收通过后的两个独立批次。

## 实现

- 新增 `AApecoxWanderAIController`：`bWantsPlayerState=true`，Authority-only 随机可达点移动；围绕固定出生中心选择目的地，到达后随机停留，路径失败则延迟重试。
- 新增 `AApecoxBotCharacter`：复用玩家角色 GAS/死亡链，使用移动方向旋转，支持地图放置与运行时生成自动占有，并保存 Authority 出生 Transform。
- 玩家角色死亡链新增虚拟重生分流；玩家继续使用原 Controller 的 `RestartPlayer`，Bot 使用 Class + Spawn Transform 请求重生。
- GameMode 新增 Bot 重生调度。旧 AI Controller 连同旧 PlayerState/ASC 销毁；新 Pawn 依靠 `AutoPossessAI` 获得全新 Controller、PlayerState、ASC 和满生命属性。
- 模块加入 `AIModule` 与 `NavigationSystem` 依赖。

## 验证

- `ApecoxEditor Win64 Development`：UHT、编译和完整链接成功。
- 首次编译发现重生函数形参名与 `APawn::Controller` 同名，UE 将 C4458 作为错误；改为 `RespawnController` 后重新完整构建成功。
- 全量 `Apecox.*` 自动化：发现并执行 62 项，62 Success、0 失败、0 未运行。
- 唯一警告来自既有 `GameplayCueNotifyPaths` 未指定后的 `/Game/` 回退，与本轮 AI 无关。
- 自动化证据：`Saved/Diagnostics/M1D/Bot/Tests/index.json`。
- 运行日志：`Saved/Diagnostics/M1D/Bot/tests.log`。

自动化覆盖现有动画、姿态、镜头、射击、实体子弹、散布、后坐和换弹回归。实际 NavMesh 覆盖、Manny 移动表现以及两次死亡重生需要按 `Current_UE_Manual_Steps.md` 在单人 PIE 验收。

## 首次 PIE 修正

用户发现机器人视觉速度需要与玩家一致，且 Manny 移动时没有播放地面运动姿势。只读资产核对确认 `BP_ApecoxPlayerCharacter` 的 `WalkSpeed` 是 400 cm/s，而原生 C++ 默认值是 600；构造函数写 CMC 时 Blueprint 最终字段尚未生效，因此在 BeginPlay 增加 `RefreshMovementSpeed`，让玩家和 Bot 从出生第一帧按各自 Class Defaults 同步。

`ABP_Apecox_Manny` 虽然包含完整 Locomotion，但用户按清单把父类从 `AnimInstance` 改成 `ApecoxCharacterAnimInstance` 时，UE 编译日志报告原 Blueprint `GroundSpeed` 与父类原生属性冲突，并自动将旧变量改成 `GroundSpeed_0`。这会形成两份速度状态，不能把“父类已正确修改”当成运行时状态机已正确消费 C++ 状态。

当前机器人改用模板原始 `ABP_Unarmed`：它与 `SKM_Manny_Simple` 同属一套资产，直接包含 `BS_Idle_Walk_Run`、`MM_Idle`、Jump、Fall Loop 和 Land，不存在本次重父类冲突。持续步行仍由 Locomotion Blend Space 驱动，不使用 Montage。Bot 的 Walk Speed 固定为与玩家当前配置一致的 400。

编辑器运行期间本次 C++ 修正曾先以 `-NoLink` 编译，因此当时的 PIE 没有加载新实现；用户报告“修改没有生效”不能用于否定 BeginPlay 同步逻辑。编辑器关闭后已进入完整 DLL 链接和自动化复验。

第二次 PIE 中 Bot 已保存并实际引用 `ABP_Unarmed`，但仍只平移不进入移动状态。二进制资产核对确认 Epic 模板的 `ShouldMove` 同时读取 `GroundSpeed` 与 `GetCurrentAcceleration != 0`；UE 5.8 的 Character Movement 则明确允许 Path Following 默认直接设置速度、忽略加速度。玩家输入会产生 Current Acceleration，AI MoveTo 未必产生，所以同一个 AnimBP 对玩家正常、对 Bot 可能一直保持 Idle。

最终代码在 `AApecoxBotCharacter` 构造和 BeginPlay 两处启用 `bRequestedMoveUseAcceleration`。构造建立原生默认，BeginPlay 防止既有 Bot Blueprint 组件模板覆盖它；同时输出一次 MaxWalkSpeed、该开关和实际 AnimInstance，便于后续直接按运行时数据排查。完整 `ApecoxEditor` 编译和链接成功。全量 `Apecox.*` 自动化再次执行 62 项：61 Success、1 Success With Warnings、0 失败、0 未运行；唯一测试警告仍是既有 `GameplayCueNotifyPaths` 回退。证据位于 `Saved/Diagnostics/M1D/BotAccelFix/Tests/index.json` 和 `tests.log`。

## 资产状态

`Content/Blueprints/Input/Actions/IA_Apecox_Reload.uasset` 已确认存在于磁盘，上一阶段的 R 键输入保存风险解除。本轮 C++ 不直接保存 UE 二进制资产；用户需要复制 `BP_ApecoxPlayerCharacter`、重父类为 `ApecoxBotCharacter`、放置 Nav Mesh Bounds Volume 与 Bot 实例。
