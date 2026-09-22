# Apecox Unreal Engine Project Context

更新日期：2026-09-22

## 项目事实

- 项目路径：`D:/UnrealProject/Apecox`
- 项目文件：`Apecox.uproject`
- Unreal Engine：5.8，安装路径 `E:/UE_5.8`
- 主运行时模块：`Apecox`
- 默认地图：`/Game/Blueprints/Maps/L_Apecox_DevGym`
- 当前状态：M3 PvE Score Attack。玩家与 AI 均出生持枪；最小服务器战斗 AI 已接入 NavMesh 追击、视线、随机换位、随机点射、实体 Projectile、换弹、死亡、重生和计分。AI 战斗与 PostMatch HUD 已人工通过；当前等待双窗口客户端 FP Arms/Weapon 生命周期修复的 PIE 复验，完整构建与 78/78 自动化通过。
- Git 远端：`https://github.com/111crab/Apecox.git`；项目自建二进制资产使用 Git LFS，第三方商业资产默认不提交。

## 当前产品边界

- 玩家始终使用第一人称操控，Owning Player 使用 FP Arms/Weapon；空手第一人称手臂隐藏。当前新增目标是在不改写玩法真相的前提下，让其他玩家看到同步的 TP Manny/Weapon 表现；不新增本地第三人称操控或相机切换。
- 当前完整体验是：拾取步枪、移动与姿态、ADS/镭射/检视、Projectile 自动射击、散布/后坐、换弹与弹药 HUD、命中反馈、静止人机死亡与重生。
- 当前只保留 Projectile 射击模型。旧 Hitscan 分支、战术自伤调试输入和自由漫游逻辑已在正式收口时删除。
- 人机已经是可移动、可射击、可换弹、可死亡重生的最小战斗 AI。GameMode/GameState 已有玩家队与 AI 队计分和目标分裁定；Canvas HUD 已提供临时比分与胜负反馈。正式比赛 UMG、简单 Arena、第二把武器/配件、伤害成长护盾、滑铲、低墙翻越和英雄技能仍未完成。
- 第一人称单人 PIE 继续作为稳定回归基线；第三人称整合使用 Listen Server/Client 互相观察。第三人称只表达同一套 Gameplay/GAS 状态，不新增本地第三人称操控，也不要求逐帧复刻第一人称动作。

## 运行时结构

- `AApecoxPlayerState` 持有 ASC 与 VitalAttributeSet，Character 是 Avatar。
- `UApecoxAbilitySet` 负责 Authority 授予和撤销；基础 Pawn Set 只含死亡 Ability，步枪 Set 只含 Projectile Fire Ability。
- PlayerController 上的 Inventory 保存 OwnerOnly 物品与槽位；Character 上的 Equipment 管理当前装备、FP/TP 表现，以及公开复制的第三人称镭射可见状态。
- 网络客户端可能先收到 Equipment 摘要、后完成本地占有。Owning Client 在 `PawnClientRestart` 后幂等重建一次 Equipment 表现，保证摘要/Controller 任意到达顺序都得到 FP Arms/Weapon；该修复不在 Listen Server、Standalone 或 AI 上执行。
- Weapon Definition 保存配置；RangedWeaponInstance 保存弹药和 Burst 状态；Presentation Definition 保存 Mesh、Montage、VFX、SFX、ADS 和镭射数据。
- Projectile 在 Authority 上碰撞与结算，TargetData 传递经过验证的射击意图；散布由客户端/Authority 可复核的确定性种子计算。已接受射击的实际起点、散布后方向、弹速、重力和距离通过一次不可靠 Multicast 驱动各端本地无碰撞 Tracer；Dedicated Server 不创建视觉对象。
- 第一人称 AnimBP 读取 Character 的移动、姿态、探头、ADS、Look Sway、Turning、后坐和左手 IK 状态；第三人称 AnimBP 使用 Lyra Manny 兼容的步枪移动、跳跃、蹲伏、Aim Offset 与左手 IK，开火和换弹由复制表现驱动。
- 第三人称水平 Aim/Turn-in-place 保持 Actor 立即跟随 Controller Yaw，只在 AnimInstance 本地维护 `RootYawOffset` 与 `AimYaw=-RootYawOffset`。`Abs(RootYawOffset)>35` 时由只读请求变量选择 Lyra 站立/蹲伏左右四条 Rifle 90 度序列；`TurnInPlacePlayRate` 在 `1.0..1.6` 内有限调整，持续大角度依靠 `Always Reset on Entry` 重入接力。`RemainingTurnYaw` 重播跳变只重建曲线基线，反向输入通过独立中断变量先退出旧方向状态。不迁入 Lyra 完整 AnimBP 运行时，也不复制逐帧动画偏移。
- 第三人称镭射只复制当前可见布尔值；观察端先用 Lyra `SK_Rifle` 的 `Muzzle` 定位 RAR 挂件，再转挂稳定 `Root` 骨骼。Beam 起点和方向均来自实体挂件发射 Socket，不复制逐帧命中点，也不使用观察者相机做 ADS 汇聚。冲刺、换弹、检视和装备表现暂时关闭 Beam/Dot 并保留开启意图。死亡委托、死亡状态生成门禁与表现 Actor Owner 生命周期检查共同清理本地非复制挂件。
- `AApecoxWanderAIController` 名称为 Blueprint 序列化兼容保留，当前在 Authority 执行最近玩家选择、NavMesh 追击、Line of Sight、随机换位、随机点射/停火、瞄准误差、GAS 开火和空仓换弹。

## 模块依赖

- Public：`Core`、`CoreUObject`、`Engine`、`InputCore`、`EnhancedInput`、`GameplayAbilities`、`GameplayTags`、`GameplayTasks`、`ModularGameplay`。
- Private：`NetCore`、`Niagara`、`AIModule`、`NavigationSystem`。
- `ModularGameplay` 用于 `UControllerComponent`；`AIModule` 与 `NavigationSystem` 用于最小战斗 AI 和 NavMesh 移动。

## 本机参考来源

- Lyra Starter Game：`D:/UnrealProject/LyraStarterGame`
- RAR 及其他项目资产：`D:/UE_Resource`
- 参考项目只用于研究结构、动画、参数和资源依赖；Apecox 保持自己的 Gameplay、GAS、Equipment 与射击事务。

## 渲染事实

- 默认 RHI 为 DX12，目标 Shader Model 为 SM6。
- Lumen、Virtual Shadow Maps 与 Mesh Distance Fields 开启。
- Hardware Ray Tracing 和 Substrate 关闭；Motion Blur 常驻关闭。
- 用户曾遇到 DX12 显存/驱动异常；`-d3d11` 只用于排障，不是项目默认配置。

## 验证基线

- `ApecoxEditor Win64 Development` 完整构建无警告。
- Apecox Blueprint 编译 0 错误、0 警告、0 加载失败。
- 收口资产断言通过：必需资产全部加载、已删除资产不存在、旧分支引用为 0。
- 正式收口基线仍位于 `Saved/Diagnostics/ProjectClosure`。
- 第三人称镭射最终运行时修复后，`ApecoxEditor Win64 Development` 完整构建成功；`Apecox.*` 共 72 项均成功，0 失败；唯一警告是既存的引擎连通性请求超时。
- 初版镭射证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaser`；方向坐标系修复证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserDirection`；旧版运行时证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserRuntimeFix`；最终枪管方向、稳定挂载、动作门控和死亡生命周期修复后的 72/72 回归证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserLifecycleFix/Tests`。
- 第一人称 ADS 动态红点重合修复后，完整构建成功；新增 `Apecox.Camera.LookSway.ADSLock` 专项成功；全量 `Apecox.*` 73/73 成功、0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-19_ADSLaserLock`。
- 原地转身连续重入与蹲姿反向修复后，完整构建成功；`Apecox.Animation.*` 9/9 成功、0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonTurnRestartReversalFix/Tests`。
- 400/650 移动速度与第三人称 Blend Space 对齐后，`ApecoxEditor Win64 Development` 完整构建成功；动画专项 9/9、全量 `Apecox.*` 75/75 成功。冷加载证据与报告位于 `Saved/Diagnostics/2026-09-21_LocomotionSpeedAlignment`。
- 权威路径多人曳光实施后，完整构建成功；Projectile 专项 13/13、全量 `Apecox.*` 76/76 成功，0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-21_ProjectileTracer`。

## 协作入口

- `Agent/00_Coordination/Current_Phase.md`：当前唯一状态入口。
- `Agent/00_Coordination/Current_Code_Design.md`：收口后的现行结构。
- `Agent/00_Coordination/Current_UE_Manual_Steps.md`：最终精简 PIE 烟雾清单。
- `Agent/00_Coordination/Decision_Log.md`：长期决策。
- 历史 `Prompts`、`Reports`、`Reviews` 和 `Research_Notes` 只用于追溯，不能覆盖当前文档。
