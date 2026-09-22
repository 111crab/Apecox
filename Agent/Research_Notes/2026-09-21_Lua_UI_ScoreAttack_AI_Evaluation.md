# PvE Score Attack、UMG 与移动 AI 评估

更新日期：2026-09-21；Unreal Engine 5.8。

## 当前决策

用户取消 Lua/UnLua 集成。后续基础 UI 使用 UMG；准星、命中标记和当前已经稳定的 Canvas 表现可以继续保留，不需要为了统一技术栈立即重写。

面试版本的玩法目标是：玩家出生持枪，与可移动、可攻击、可重生的 AI 对战；双方击杀增加队伍分数，先达到目标分的一方胜利。继续复用 Apecox 的 Character、GAS、Equipment、Projectile、Health、Death、Respawn 和第三人称表现。

## 为什么不直接复制 Lyra AI

Lyra Bot 依赖 Experience、GameFeature、Modular AIController、Lyra PlayerState、队伍接口、PawnData、武器服务、Behavior Tree 与 EQS。直接迁移资产会带入大量缺失的 C++ 类和插件依赖，也无法通过复制 `.uasset` 获得完整运行时。

Apecox 只参考它的职责边界：服务器控制 AI、PlayerState 表示参赛者、导航产生移动、武器系统处理射击、GameState 复制比赛状态。首版直接使用一个小型原生控制器更快，也更容易证明现有系统能够复用。

## 当前最小 AI

`AApecoxWanderAIController` 保留历史类名以兼容已经保存的蓝图引用，当前行为是：

1. 查找 60 米内最近的存活玩家；
2. 目标超过 18 米或没有视线时，通过 NavMesh `MoveToActor` 接近；
3. 进入射程且有视线后停止、面向目标；
4. 通过现有 ASC 输入请求开火；
5. 弹匣为空时调用现有换弹事务；
6. 死亡、失去目标或比赛结束时停止开火和移动。

首版不加入 AI Perception、Behavior Tree、EQS、掩体、侧移、巡逻、听觉、武器拾取和难度系统。这些是闭环通过后的扩展项。

## 地图契约

基础 AI 不绑定具体地图。地图只需满足：

- NavMeshBoundsVolume 覆盖战斗区域并生成连续可达面；
- 有玩家与 AI 出生位置；
- 地面、墙体和掩体碰撞可以支持导航与 Line of Sight；
- 没有只能由玩家跳过的必经断层或 AI 无法返回的深坑。

当前先在 `L_Apecox_DevGym` 验证。通过后创建普通非 World Partition 的轻量 `L_Apecox_Arena`，加入少量掩体、清晰的近中距离交战路线与出生点。不会迁移 Lyra 完整 Shooter 地图。

## 比赛规则职责

- `AApecoxGameMode`：服务器唯一计分裁判、目标分胜负、玩家/AI 重生许可。
- `AApecoxGameState`：复制 MatchPhase、双方队伍分数、TargetScore 和 Winner。
- `AApecoxPlayerState`：复制个人 Kills、Deaths 和 CombatTeam。
- `UApecoxHealthComponent`：每次权威死亡只提交一次 Victim/Killer。
- UI：只观察 GameState、PlayerState、Health 和 Equipment，不执行加分或胜负判断。

默认目标分为 10。玩家击杀 AI 时玩家队加 1，AI 击杀玩家时 AI 队加 1；自杀和环境死亡不增加队伍分。达到目标分后进入 PostMatch，停止 AI 决策、新射击和后续重生。

## 后续 UMG 显示范围

首版必需：

- 玩家当前/最大生命；
- 武器名称；
- 弹匣/备用弹药；
- 玩家队与 AI 队比分、目标分；
- 胜利/失败面板。

推荐但可后置：

- 击杀提示；
- 空仓与换弹提示；
- 受击方向；
- 简单比赛阶段或重新开始提示。

准星中心点、动态扩散、ADS 隐藏和服务器确认命中后的红色 X 已由 Canvas HUD 实现。它们属于高频本地绘制，当前无需迁入 UMG。

## 实施顺序

1. 在 DevGym 完成 AI 追击、射击、换弹、死亡、重生、命中标记和双端验证。
2. 制作轻量 Arena 并调整交战距离、人数和出生位置。
3. 建立 UMG 比赛 HUD，绑定现有复制状态。
4. 完成 Listen Server 双端、冷启动、完整构建与演示收口。
