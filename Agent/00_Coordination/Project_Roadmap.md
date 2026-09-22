# Apecox 项目路线图

更新日期：2026-09-21

## 产品方向

Apecox 是面向 UE 游戏客户端岗位展示的第一人称射击垂直切片。玩家固定使用第一人称操控，同时为其他观察者同步第三人称角色与单步枪行为。项目优先证明 Gameplay Framework、GAS、服务器权威武器事务、双视角表现分层、数据驱动配置和自动化验证能力；内容数量不是目标。

## 已交付里程碑

### M0 工程与运行时基础

状态：完成。

- UE 5.8、DX12/SM6、Enhanced Input、Gameplay Ability System、开发地图和 Public/Private 模块结构。
- GameMode、GameState、PlayerController、PlayerState、Character、ASC、AttributeSet 与 HealthComponent 职责分层。
- PlayerState 持有 ASC，Pawn 作为 Avatar；死亡 Ability 与延迟重生闭环。
- OwnerOnly Inventory、主/副槽结构、Character Equipment、Definition/Instance/Presentation 分层。

### M1 第一把步枪可玩闭环

状态：2026-09-18 正式收口。

- 出生即通过权威库存事务装备步枪、保留权威拾取能力、第一人称装备与静止可重生人机。
- 持枪移动、冲刺、跳跃、蹲伏、趴下、探头、ADS、检视和左手 IK。
- Projectile 自动射击、弹匣/备用弹药、普通/空仓换弹、确定性连发散布和分层后坐。
- 镜头压枪保留、近墙遮挡、固定一倍镜、镭射与动态准星。
- 枪口火光/烟雾、Impact 粒子、命中声、弹孔、脚步、落地、姿态、装备和机械换弹声音。
- 自动化、全蓝图编译、资产引用扫描和完整 Editor 构建均已通过。

### M2 第三人称单步枪行为同步

状态：2026-09-19 完成并收口。

- 已完成并人工验收远端出生持枪、Idle、四向移动、冲刺、跳跃/落地、蹲伏和上下 Aim Offset。
- 已完成左手 IK、人物/枪械开火以及普通/空仓/移动换弹同步；第一人称表现未发生回归。
- 第三人称使用 Lyra Manny 与 Rifle 配套资产，复用 Apecox 现有 CMC、GAS、Equipment、弹药和换弹事务，不迁移 Lyra 玩法框架。
- 同一镭射可见状态已经同步成远端挂件、光束和落点；完整构建、73/73 自动化、双端生命周期和第一人称 ADS 动态重合均已通过。
- 权威实体 Projectile 与多人本地曳光已经分层；单人及 Listen Server 双向人工认证通过，当前自动化基线为 76/76。

### M3 PvE Score Attack

状态：2026-09-22 已形成稳定原型；最新 AI 积极度、护盾门槛和按命中揭示血条等待最终人工观感验收。

- 最小服务器战斗 AI：寻找最近存活玩家、NavMesh 追击、视线与射程判断、GAS 开火、现有换弹、死亡和重生。
- 权威击杀归因、个人 K/D、玩家队/AI 队比分、默认 15 分目标和 PostMatch 胜方。
- 腰射中心点与服务器确认命中后在腰射/ADS 共用的红色 X 命中标记。
- 当前 Canvas HUD 常驻显示玩家队/AI 队比分，达到目标分后显示 `VICTORY / DEFEAT`，避免 PostMatch 停火和 AI 停止被误判为运行故障。
- Lua 已取消；比赛界面后续使用 UMG。
- 玩家初始 `100` 生命与白色 `25` 护盾；对敌实际伤害累积护盾进化点，白到蓝需 `500`，蓝到紫再需 `1000`。红色血包与蓝色护盾电池已接入权威拾取事务，玩家成长跨 Pawn 重生保留，AI 不使用护盾。
- AI 已从全图直追改为“争夺点移动/环视 -> 局部发现 -> 反应延迟 -> 短连发/换弹 -> 丢失目标后返回争夺点”的轻量状态逻辑。
- HUD 已显示玩家生命、护盾品质与下一等级点数、武器弹药、比分和胜负；敌人血条只在本地玩家实际造成伤害后短暂揭示。
- 当前使用用户自制地图与 `L_Apecox_DevGym` 验证玩法，不迁移 Lyra 完整地图或 AI 框架。
- `ApecoxEditor Win64 Development` 完整构建和全量 `Apecox.*` 自动化 `83/83` 已通过；最新人工项见 `Current_UE_Manual_Steps.md`。

## 已完成版本边界

- 玩家固定使用第一人称操控；空手第一人称手臂隐藏。
- 第三人称 Character、武器和动作负责向其他玩家表达同一玩法行为；不提供第三人称操控，也不要求逐帧复刻第一人称。
- M2 收口时的人机静止靶标边界已经由 M3 重新开启；当前人机会自主追击、射击、换弹、死亡和重生。
- 运行时只保留 Projectile 射击模型；Hitscan 试验分支已删除。
- 比赛权威规则已经存在；胜负 UMG 和演示 Arena 属于 M3 后续交付。

## 已暂缓能力包

这些内容不是正式收口版本的未修缺陷。以后恢复开发时，每次只重新启用一个能力包并建立新的设计、测试和人工验收范围。

| 能力包 | 暂缓内容 |
| --- | --- |
| AI 扩展 | Behavior Tree/EQS、AI Perception、掩体、侧移和完整难度系统 |
| 比赛 UI | UMG 玩家生命、武器信息、比分、目标分和胜负面板 |
| 网络加固 | 新一轮多人回归、Dedicated Server、延迟/丢包、带宽与预测纠正专题 |
| 武器生态 | 切枪、通用配件、第二把武器、共享弹药类型 |
| 英雄战斗扩展 | 小技能、大招、被动与临时特殊武器；基础护盾、进化点和补给拾取已进入 M3 |
| 扩展移动 | 第一/第三人称滑铲、低墙翻越；自由贴墙攀爬暂不进入面试版 |
| 工具链 | 通用技能编辑器、SkillGraph 或完整数据解释器 |

第三人称趴姿已登记并搁置。出生/重生瞬时闪手、水平独立瞄准和 Turn-in-place 已完成；用户已经取消第三人称检视同步，拾取/装备动作仍不是当前完成条件。

## 重新开启项目时的顺序

1. 在 `Current_Phase.md` 中写明唯一的新交付目标和明确排除项。
2. 先重新审计当前资产与 C++ 接口，再更新 `Current_Code_Design.md`；不要直接复用历史 Prompt 中的旧字段。
3. 功能实现后依次完成 Editor 构建、定向自动化、全量 `Apecox.*` 自动化和精简 PIE 清单。
4. 只有功能边界再次稳定时才做下一轮全项目清理。

## 当前证据入口

- 当前状态：`Current_Phase.md`
- 当前结构：`Current_Code_Design.md`
- 最终人工检查：`Current_UE_Manual_Steps.md`
- 正式收口报告：`../Reports/2026-09-18_Project_Closure_Cleanup_Report.md`
- 正式收口自动化报告：`Saved/Diagnostics/ProjectClosure/Tests/index.json`
- 第三人称镭射构建与自动化报告：`Saved/Diagnostics/2026-09-19_ThirdPersonLaser`
- 第三人称镭射运行时修复报告：`Saved/Diagnostics/2026-09-19_ThirdPersonLaserRuntimeFix`
- 第三人称镭射最终生命周期修复报告：`Saved/Diagnostics/2026-09-19_ThirdPersonLaserLifecycleFix/Tests`
- 第一人称 ADS 镭射动态重合报告：`Saved/Diagnostics/2026-09-19_ADSLaserLock`
- 多人子弹曳光报告：`Saved/Diagnostics/2026-09-21_ProjectileTracer`
- PostMatch 比分与胜负提示报告：`Saved/Diagnostics/2026-09-21_PostMatchHUD`
- 客户端本地占有后 FP 表现重建报告：`Saved/Diagnostics/2026-09-22_ClientFirstPersonRestart`
