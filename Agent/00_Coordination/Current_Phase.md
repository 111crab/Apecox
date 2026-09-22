# 当前阶段

更新日期：2026-09-22

## 状态：M3 PvE Score Attack，AI 积极度、护盾新门槛和按命中揭示血条等待人工验收

当前目标是完成可玩的第一人称 PvE 面试 Demo。玩家自制演示地图；代码侧已把 Weapon、Projectile、Damage、Shield、Pickup、Death、Respawn、Score、GameMode 与清晰战斗 HUD 串成闭环。Lua 已取消，当前继续使用原生 Canvas HUD；第三人称检视和趴姿继续不做。

### 本轮已经实现

- 玩家初始 `100` 生命与白色 `25` 护盾。护盾进化点由 `AApecoxPlayerState` 持有，因此同一场比赛死亡换 Pawn 后仍保留；白/蓝/紫依次为 `25/50/75`。白到蓝需要 `500` 点，蓝到紫再需要 `1000` 点，即累计阈值 `500/1500`，紫色封顶。
- 步枪基础伤害改为 `FApecoxRangedFireConfig.BaseDamage=13`。Authority 用原生 Instant GE 的 `SetByCaller.Damage` 冻结每发伤害，不再依赖旧蓝图 GE 内硬编码的 `20`；最终结算护盾优先、溢出生命，击中护盾同样产生命中确认。
- 玩家对敌方造成多少实际护盾或生命伤害，就获得多少进化点；过量伤害不计入。AI 没有护盾、成长或拾取资格。
- 新增地图可放置的 `AApecoxCombatPickup`：血包默认恢复 `100` 生命；护盾电池先增加 `300` 进化点，再补满升级后的护盾；Authority 判定、复制可用状态，默认 `20s` 重生。圆柱/文字/灯光仅为占位，`PickupMesh` 可直接替换用户自选美术。
- HUD 左下显示放大的生命、护盾条、白/蓝/紫品质和下一等级剩余点数；右下显示枪名与弹药；顶部放大显示双方 `0/15` 比分。比赛目标分的 C++ 与 `BP_ApecoxGameMode` 资产默认值均已更新为 `15`。
- AI 伤害开关默认恢复为 `apecox.AI.DamageEnabled=1`，正常比赛具有杀伤；只有调试观察时临时设为 `0`。
- `AApecoxWanderAIController` 已从全地图追击改为轻量“争夺点巡逻 → 本地发现 → 射程战斗 → 脱战返回”循环。地图用 `AApecoxAIObjectivePoint` 标记 `3–6` 个争夺点；AI 只在 `2400 cm` 内且有视线时获取玩家、`1800 cm` 内开火、超过 `3000 cm` 或丢失视线约 `2s` 后返回争夺点。该实现复用现有 NavMesh/GAS/Projectile/Reload/Animation，不引入 Lyra Experience、行为树或 EQS。
- AI 积极度已经上调：争夺点停留缩短为 `1–2s`，反应时间、点射间停顿和战斗换位间隔均缩短，单轮点射略延长；局部索敌和脱战边界保持不变。
- 敌人血条不再因进入 60 米范围自动出现。实体 Projectile 的权威回执新增实际受伤目标，拥有者本地 HUD 只显示自己命中的那个 AI，约 `4s` 后隐藏，再次命中刷新；Host 与 Client 的揭示状态互相独立。
- `AApecoxCombatPickup` 现能在自身动态材质实例上自动写入发光颜色。当前明确使用 `/Game/SciFi_Props/Models/SM_Box_2` 作为红色血包、`/Game/SciFi_Props/Models/SM_Box_9` 作为蓝色护盾电池；原始 `SciFi_Props` 材质不被修改。

- `AApecoxWanderAIController` 保留旧类名以兼容蓝图引用，内部已升级为最小服务器战斗 AI：寻找最近存活玩家、NavMesh 追击、视线判断、独立随机反应时间、短点射/停火、每轮稳定瞄准误差、随机横向与径向换位，并继续通过现有 GAS 输入开火和空仓换弹。
- `AApecoxBotCharacter` 在 Authority 被占有后装备 `StartingWeaponDefinition`，继续复用现有 Equipment、Projectile、弹药、第三人称动画、伤害、死亡和重生链路。Bot 已改用与玩家相同的 Lyra `SKM_Manny`，以保证 `weapon_r` Socket 和开火 Montage Skeleton 一致。
- `AApecoxPlayerState` 复制个人击杀、死亡和阵营；`AApecoxGameState` 复制比赛阶段、玩家队分数、AI 队分数、目标分和胜方。
- `AApecoxGameMode` 是唯一计分裁判。默认目标分为 15；达到后进入 `PostMatch`，AI 停止决策，禁止开始或提交新射击，停止继续安排重生。
- HUD 腰射准星新增中心点；实体 Projectile 最终确认造成有效伤害时，较细的红色四段 X 叠加在原腰射准星上，ADS 也显示同一命中标记。
- 首轮无伤 PIE 已确认 AI 持枪、Projectile、枪口/命中、追击、横向换位、死亡停火和玩家命中反馈全部通过；正式可玩版本现默认启用真实 AI 伤害。
- AI 换弹不播放的根因是服务器 AIController 同样属于 LocalController，旧表现分流误把 Bot 当作第一人称拥有者。现在只有“玩家控制且本地控制”的 Pawn 使用 FP Arms，AI 始终走可见 TP Character/Weapon Montage。
- ADS 已从按住语义改为单击切换：第一次 `Started` 进入，松键不处理，第二次 `Started` 退出；冲刺、换弹、死亡和卸枪仍清除瞄准意图。
- AI 第三人称换弹、ADS 单击切换和随机战斗节奏已经全部通过人工验证。
- 运行中“有弹药但不能射击、所有 AI 同时静止”已确认不是随机故障，而是玩家达到目标分后进入 `PostMatch`。Canvas HUD 常驻显示双方比分，并在结束时显示 `VICTORY / DEFEAT` 与重新开始提示；比赛数据源仍是 `AApecoxGameState`。当前目标分已由当时的 10 调整为 15。
- PostMatch 比分和胜负提示已经人工验证通过。
- 双窗口发现客户端自身 FP Arms/Weapon 消失而只剩 TP 镭射挂件。根因是 `EquippedWeaponState` 可能早于客户端 Controller 占有到达，首次 RepNotify 会把自身 Pawn 暂时误判为非本地观察对象。`PawnClientRestart` 现只在 `NM_Client` 本地 Pawn 上重建一次 Equipment 表现，使两种复制顺序都收敛为 FP Arms/Weapon，并清除早到摘要误建的 TP 镭射；Listen Server 与 Standalone 不重复重建。

### 当前验证结果与下一步

- `ApecoxEditor Win64 Development` 完整构建成功。
- 护盾、成长、AI 排除、血包、电池、13 点 Projectile 伤害与保存后的步枪配置契约专项共 17/17 成功、0 失败。
- 本轮 `ApecoxEditor Win64 Development` 完整构建成功；全量 `Apecox.*` 自动化 83/83 成功、0 失败、0 AutomationController 警告。更新后的护盾专项已覆盖累计 `500/1500` 阈值与 300 点电池；证据位于 `Saved/Diagnostics/2026-09-22_AggressiveAIShieldHealthReveal/All/index.json`。
- `BP_ApecoxGameMode.TargetScore` 冷加载写入并保存为 `15`。
- AI 换弹分流、ADS 单击切换和随机战斗节奏加入后，`ApecoxEditor Win64 Development` 完整构建成功；全量 `Apecox.*` 78/78 成功、0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-21_AIReloadToggleADSRandomCombat`。
- 全新无界面 UE 进程确认 Bot 默认步枪、战斗 AIController、持枪 AnimBP、Lyra `SKM_Manny`、`weapon_r` Socket 和 `400 cm/s` 速度均持久保存；开发图有 2 个 Bot、2 个 PlayerStart 和 NavMeshBoundsVolume。
- 当前只执行 `Current_UE_Manual_Steps.md`：把既有 `BP_ApecoxAIObjectivePoint` 的停留时间改为 `1–2s`，再验证 AI 积极度、累计 `500/1500` 护盾门槛以及按本地命中揭示约 `4s` 的敌人血条。

### 当前明确排除

- 不接入 Lua/UnLua，也不迁移 Lyra Experience、GameFeature、完整 AI/队伍或地图体系。
- 首版 AI 不做掩体决策、复杂路径战术、听觉、武器拾取或完整难度系统；当前争夺点巡逻和随机战斗参数都集中在 AIController，PIE 通过后再决定最终精度与伤害。
- 伤害成长护盾已经实现。后续扩展按“滑铲 → 低墙翻越”排序；自由贴墙攀爬暂不进入面试版范围。

## 历史记录

以下内容记录 M1/M2 已完成阶段及当时边界。出现“AI 静止”或“积分暂缓”等文字时，表示当时的收口状态，已由上方 M3 当前状态取代，不再作为当前实现说明。

## 2026-09-21 多人子弹曳光实施

- 保留 `AApecoxWeaponProjectile` 作为服务器唯一碰撞、重力、伤害和 Impact 真相；曳光不参与弹药、命中或 GameplayEffect。
- Authority 接受一发后，把该发的实际玩法起点、确定性散布后的方向、随机弹速、重力和初始可视距离通过 Equipment 的 `NetMulticast, Unreliable` 发送。高频纯表现允许丢弃单个事件，不使用 Reliable 堵塞网络。
- 每个相关渲染端生成本地 `AApecoxProjectileTracer`：不复制、无碰撞、复用 `SM_IG_Projectile_Bullet`/`MI_IG_Bullet_Glow`，沿 X 拉伸约 2 米，Emissive 设为 250，最大寿命 0.75 秒。Dedicated Server 跳过生成。
- 原实体 Projectile 的旧可见网格保持隐藏，避免服务器复制位置稍晚到达后出现第二条滞后鬼影。其碰撞、Movement、生命周期和调试轨迹保持不变。
- `ApecoxEditor Win64 Development` 完整构建成功；`Apecox.Weapon.Projectile` 13/13、全量 `Apecox.*` 76/76 成功，0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-21_ProjectileTracer`。
- 用户已确认单人观感、Host/Client 双向可见性、方向、生命周期及原有射击反馈全部通过。本轮正式收口，人工清单清空。

## 当前第三人称目标

- 使用 Lyra 的 Manny 步枪动画作为第三人称素材来源，优先覆盖持枪 Idle、四向 Walk/Jog、Jump、Crouch、Aim Offset、Equip、Fire 和 Reload。
- 保留 Apecox 的 Character、CMC、GAS、Equipment、Projectile、弹药与换弹事务；不迁移 Lyra Experience、GameFeature、武器玩法或完整 AnimBP 运行时。
- 第一人称继续使用现有 RAR Arms/Weapon 动画。第一和第三人称共享玩法状态与动作时序，但允许使用不同动画资产和不同骨架表现图。
- 玩家始终使用第一人称操控和 FP Arms/Weapon；其他玩家观察该角色时看到完整 Manny 和 TP Weapon。当前产品取消空手主流程，玩家出生即通过正式库存事务获得并装备唯一一把步枪。

## 第三人称当前完成状态

- 已使用 Lyra `SKM_Manny`、`SK_Rifle` 和对应骨架动画建立 `ABP_Apecox_Rifle_TP`；第三人称资产不与 RAR 第一人称骨架交叉播放。
- 已完成并人工验收出生持枪、Idle、四向移动、冲刺、跳跃/下落/落地、蹲伏、上下 Aim Offset、左手 IK、人物/枪械开火以及普通/空仓换弹。
- 第一人称继续使用 RAR Arms/Weapon；第三人称只表达相同玩法语义，不追求与第一人称动作逐帧一致。
- 当前用户要求的第三人称核心行为均已实现并完成人工验收。
- 第三人称趴姿已明确登记并搁置；水平独立瞄准与 Turn-in-place 已完成。出生/重生瞬时闪手已由用户确认修复。

## 第三人称实施顺序（进度）

1. 已完成：Owner 只看 FP、其他观察者只看 TP，远端步枪 Mesh 正确附着。
2. 已完成：Lyra Manny Rifle Idle、四向移动、冲刺、跳跃、蹲伏和精简 TP AnimBP。
3. 已完成：Aim Offset、左手 IK、开火和换弹；动作继续由现有 Equipment/Reload 玩法事务驱动。
4. 已完成：第三人称镭射同步复用同一开关意图和复制状态，只为远端增加世界表现。
5. 已完成：第三人称水平瞄准 Gate A 与 90 度 Turn-in-place Gate B；持续大角度、站立/蹲伏反向、移动中断和 400/650 速度映射均已收口。第三人称趴姿和移动 AI 继续搁置。

## 2026-09-19 第三人称水平瞄准与 Turn-in-place 调查

- 已冷加载导出 Lyra `ABP_Mannequin_Base`、`ABP_ItemAnimLayersBase`、`ABP_RifleAnimLayers`、六条转身序列和 Apecox 当前 `ABP_Apecox_Rifle_TP`，没有修改两边资产。
- Lyra 的 Character 与 Apecox 一样立即跟随 Controller Yaw；静止脚部保持来自 `RootYawOffset` 与 `Rotate Root Bone` 的动画补偿。`AimYaw=-RootYawOffset`，不是直接使用接近 0 的 `BaseAimRotation-ActorRotation` 水平差。
- Lyra Rifle 的原地转身阈值为 `Abs(RootYawOffset)>50`，实际默认使用站立/蹲伏左右四条 90 度序列。四条序列都含 `RemainingTurnYaw` 和 `TurnYawWeight` 曲线；Lyra 用曲线差值逐帧消耗视觉根偏移，不让 Root Motion 改变胶囊。
- Apecox 当前图已确认是 `Locomotion -> UpperBody Reload -> Fire Additive -> Aim Offset -> 左手 IK` 的完整结构。后续 `Rotate Root Bone` 应插在 `FullBodyAdditivePreAim` 与 `RifleLocomotion Cached Pose` 之间，水平 `AimYaw` 接入现有站立/蹲伏 Aim Offset，Turn 状态进入既有 Locomotion 状态机。
- 不迁入 Lyra 的完整 AnimBP、Linked Layer、Experience、GameFeature 或 CMC。下一轮按两道验证门实施：先做水平 Aim/RootYaw 补偿，再迁入四条 90 度序列并接入曲线驱动 Turn-in-place。
- 完整调查见 `Agent/Research_Notes/2026-09-19_Lyra_HorizontalAim_TurnInPlace.md`；证据位于 `Saved/Diagnostics/ThirdPersonTurnInPlace`。

## 2026-09-19 水平瞄准验证门 A 实施

- `UApecoxCharacterAnimInstance` 已新增只读 `RootYawOffset` 与 `AimYaw`。首次绑定 Pawn 只记录当前 Actor Yaw；静止接地时累加相反 Yaw 增量，站立限制为 `-120..100`、蹲伏限制为 `-90..80`；移动、腾空或趴姿时按 Lyra 的 Stiffness `80`、Critical Damping `1`、Mass `1`、Target Velocity Amount `0.5` 回零。
- 该状态完全属于观察端动画表现，不复制，也不写 Actor Rotation、ControlRotation、CMC、Projectile 或镭射方向。第一人称 AnimBP 继承同一基类但不消费这两个变量，因此第一人称画面保持不变。
- `ApecoxEditor Win64 Development` 完整构建成功。新增 `Apecox.Animation.ThirdPersonHorizontalAimContract` 冷加载当前 TP AnimBP，确认其仍继承共享原生 AnimInstance，且两个变量可供蓝图读取并以 0 初始化。
- `Apecox.Animation.*` 自动化 8/8 成功，0 失败、0 警告；证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonHorizontalAimGateA/Tests`。
- 当前只需按 `Current_UE_Manual_Steps.md` 在现有 AnimGraph 中插入一个 `Rotate Root Bone`，并把同一个 `AimYaw` 接到站立、蹲伏两套 Aim Offset 的 `Yaw`。通过小角度转向、移动释放、蹲伏、腾空与动作回归后，再开始门 B。

## 2026-09-19 原地转身验证门 B 实施

- 用户已确认 Gate A 小角度偏移只转动上半身，移动释放、蹲伏、腾空和原有动作层均正常。
- 已通过 Lyra 原生 Migrate 迁入 `MM_Rifle_TurnLeft_90`、`MM_Rifle_TurnRight_90`、`MM_Rifle_Crouch_TurnLeft_90`、`MM_Rifle_Crouch_TurnRight_90`。四条序列的 Lyra Notify 与 Animation Modifier 用户数据已清除，`RemainingTurnYaw`、`TurnYawWeight`、Root Motion 元数据与 Manny Skeleton 均保留；迁移附带且清理后无引用的 `TurnYawAnimModifier` 已删除。
- `UApecoxCharacterAnimInstance` 新增 `bShouldTurnInPlace` 与 `bTurnInPlaceLeft`。静止接地且 `Abs(RootYawOffset)>35` 时请求相应方向；移动、腾空或趴姿不请求转身。
- 原生更新逐帧读取 `TurnYawWeight`，用它解除 `RemainingTurnYaw` 的状态混合权重，再用相邻帧曲线差值消耗 `RootYawOffset`。首个有效曲线帧只建立基线，避免状态混入时一次性跳转约 90 度；转身中继续移动鼠标仍会同时累计新的 Actor Yaw 增量。
- Actor、胶囊、ControlRotation、射击与镭射方向仍由既有玩法链路控制；序列 Root Motion 不改变胶囊。状态机只选择第三人称表现，左右方向和阈值不在蓝图重复计算。
- `ApecoxEditor Win64 Development` 完整构建成功；`Apecox.Animation.*` 9/9 成功。新增冷加载测试确认四条序列、两条必需曲线、Manny Skeleton、Root Motion 元数据和清理后的零 Notify。测试证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonTurnInPlaceGateB/Tests`，资产审计位于 `Saved/Diagnostics/ThirdPersonTurnInPlace/turn_cleanup_audit.json`。
- 站立/蹲伏左右四个状态、四条进入转换和四条返回转换均已完成；该阶段原始接线要求已从当前人工清单移除。
- 首轮人工验证确认慢速转身正常，但快速水平甩动鼠标时上半身会达到 Clamp 极限，双脚静止并随胶囊滑动。Apecox 将阈值由 50 度提前到 35 度，并让四条序列读取 `TurnInPlacePlayRate`。后续人工反馈证明 `3.0x` 上限会造成高频跺小碎步，因此当前上限收敛到 `1.6x`，连续输入改由状态重入接力承担。
- 用户进一步确认：分多次抬起鼠标转向时正常，单次持续大角度输入只播放一轮 Turn，随后双脚停止并滑转。冷加载导出当时的 AnimBP 确认四个 Play Rate 连线和转换条件均已保存，但四个 Turn 状态仍使用 UE 默认的 `Always Reset on Entry=False`。持续请求会在旧 Turn 状态仍有 0.08 秒回混权重时重入同一状态，内部 Sequence Player 因状态仍 Active 而不复位；停顿输入则给旧权重足够时间归零，所以能再次正常播放。用户随后已把四个 Turn 状态统一设为 `Always Reset on Entry=True`。
- 冷启动导出已确认用户完成的四个 `Always Reset on Entry=True` 均持久保存。最新人工反馈为：站立反向正常，蹲姿先持续转向再直接反向会重现脚停滑转；同时转身速率过高导致腿部像跺小碎步。原生逻辑现已识别 `RemainingTurnYaw` 在同状态重播时从接近 0 跳回正负 90 度的非物理断点并只重建基线；新增 `bShouldInterruptTurnInPlace` 检测动作中反向输入，让四条返回转换立即退出旧方向。人工清单已经删除所有完成项，只保留把该变量 OR 进四条返回规则的单项操作与本轮复验。
- 连续重入/反向修复后 `ApecoxEditor Win64 Development` 完整构建成功；`Apecox.Animation.*` 9/9 成功、0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonTurnRestartReversalFix/Tests`。
- 最终人工复验发现 `CrouchTurnRight90 → Crouched` 的时间比较被接成 `0 <= Time Remaining`，导致右转状态立即退出；恢复为 `Time Remaining <= 0.08` 后，蹲姿连续转向与反向正常。
- 同轮确认角色蓝图实际速度是普通 `400 cm/s`、冲刺 `650 cm/s`。旧 Blend Space 按 C++ 过时默认值布局，普通 400 会混合 Walk@220 与 Jog@600，两个不同步态周期叠加形成小碎步。用户把五个 Walk 采样点统一移到 400 后表现正确。冷加载确认轴域仍为 0..900、Jog 行仍为 600@1.0 与 900@1.5；650 冲刺在同一 Jog 动画的倍率之间插值，结构有效。
- C++ 的 Walk/Sprint/Crouched/Prone 默认值已统一为与玩家蓝图一致的 `400/650/230/170`，消除原生实例与蓝图实例的双重速度真相；现有玩家蓝图行为不变。冷加载测试新增五个 Walk 采样必须位于 400、旧 220 行必须不存在、玩家蓝图 400/650 必须与运行时探针一致的断言。
- `ApecoxEditor Win64 Development` 完整构建成功；`Apecox.Animation.*` 9/9、全量 `Apecox.*` 75/75 成功，0 失败。证据位于 `Saved/Diagnostics/2026-09-21_LocomotionSpeedAlignment`。

当前闭环包括：

- Authority 在玩家出生/重生时自动创建并装备默认步枪；`F` 拾取保留为库存能力，但不再是当前单步枪主流程的前置步骤。
- 持枪站立、移动、冲刺、跳跃、蹲伏、趴下、左右探头及对应第一人称动画。
- `Gameplay Ability System` 驱动的自动射击输入、服务器校验、弹匣、备用弹药和换弹事务。
- 权威实体子弹、本地发光曳光表现、近墙出生阻挡、重力、连续碰撞、确定性连发散布、分层枪械/镜头后坐。
- ADS、固定一倍镜、镭射、移动与腾空准星扩散。
- 开火、空仓、换弹、检视、装备、脚步、落地和姿态声音。
- 枪口火光与烟雾、Impact 粒子、命中声和弹孔。
- Health、死亡 Ability、玩家与静止人机死亡/延迟重生；HUD 在人机头顶显示生命条和精确数值。
- 持久瞄准方向镜头后坐：RAR 曲线的 Pitch/Yaw 在 Kick 阶段写入真实 `ControlRotation`，与鼠标压枪共同形成新的准星方向；停火后只让内部弹簧和枪械模型回稳，不再反向修正玩家视角。曲线 Roll 不写入 `ControlRotation`，避免地平线侧倾。

## 收口后的战斗反馈补丁

- 修正腰射镜头后坐的方向语义。前一版把程序造成的 Pitch/Yaw 当作停火后应撤销的临时偏移，导致准星平滑回到射击前方向；当前实现只在 Kick 阶段累计真实瞄准角，内部恢复不再写回 `ControlRotation`。RAR 曲线的 Roll 被限制为表现数据，不倾斜玩家地平线。
- 修复短暂停火后立即重新开火时的准星下跳。新连发的第 1 发现在重建内部镜头弹簧坐标，从零采样本轮 RAR 曲线；玩家已经形成的真实准星方向保持不变。
- 初版复刻 RAR 的发光弹体出生缩放，但 Apecox 的高速与多人复制使近距离可读性不足。2026-09-21 已由权威射击参数派生的本地短寿命 Tracer 取代该显示方式，仍复用 `SM_IG_Projectile_Bullet` 与 `MI_IG_Bullet_Glow`，不改变实体 Projectile 玩法。
- 为静止人机增加屏幕投影生命条和 `当前 / 最大` 数值，直接读取已复制的 `UApecoxHealthComponent`，不新增第二份生命状态。

## 本次正式收口清理

- 删除已被实体子弹取代的 Hitscan Ability、TargetData、配置入口及实现文件；测试夹具统一使用 Projectile 分支。
- 删除 `X` 键战术调试输入、调试自伤 Ability/Effect 和旧输入生命周期演示资产；Pawn AbilitySet 仅保留死亡 Ability。
- 删除未使用的旧第一人称 AnimBP 和 Retargeter；当前只保留实际被角色引用的 `ABP_Apecox_FirstPersonArms`。
- 机器人维持静止靶标。保留 `AApecoxWanderAIController` 类名以兼容 `BP_ApecoxBotCharacter` 的序列化引用，但类中已移除随机寻路、计时器、NavMesh 和移动动画补丁。
- 移除 `NavigationSystem` 模块依赖、重复 DX12 配置、未使用的 `MaxStackSize`、战术 GameplayTag 和无资产引用的左手 IK 布尔字段。
- 移除 UE 5.8 已弃用的 NonInstanced Ability 检查；Apecox GameplayAbility 基类统一使用 `InstancedPerActor`。

## 验证结果

- `ApecoxEditor Win64 Development` 在战斗反馈补丁后完整构建成功。
- 全蓝图编译：Apecox 蓝图 0 错误、0 警告、0 加载失败。命令执行时还报告 14 条 InfimaGames 供应商 Manny PoseAsset 与源动画版本不一致警告；这些不是 Apecox 蓝图错误，也未改动供应商资产。日志：`Saved/Diagnostics/ProjectClosure/compile_blueprints.log`。
- 资产注册表复核：49 个 `/Game/Blueprints` 资产已重新扫描；必需资产全部存在并能加载，6 个已删除资产均不存在，已删除分支残留引用为 0。结果：`Saved/Diagnostics/ProjectClosure/closure_asset_verification.json`。
- 全量 `Apecox.*` 自动化：67 个测试节点全部成功，0 失败、0 未运行。后坐力专项 6 项覆盖末发 Kick 保留为瞄准方向、静默内部恢复、快速重新开火的新连发基线、Roll 隔离和生命周期 Reset 不移动视角。最终报告：`Saved/Diagnostics/2026-09-18_QuickRefireRecoil/FinalTests/index.json`。
- 后坐力方向、快速重新开火、可视实体子弹和 AI 生命条均已人工验证通过。`Current_UE_Manual_Steps.md` 已移除全部已通过项，等待第三人称首批实现后再加入新的精简验证。

## 有意保留的兼容名称

- `AApecoxWanderAIController`：虽然当前不再 Wander，但蓝图已经序列化该原生类路径；改名只会制造重定向和资产风险。
- `DA_Phase1B_AbilitySet`：当前是玩家/人机基础 Pawn AbilitySet，只包含死亡 Ability；保留稳定资产路径。
- `LeftHandIKEffectorTransform`：当前 AnimBP 中仍有一个未接线的变量节点。删除原生反射字段会令蓝图产生无效节点，因此在不重做二进制 AnimGraph 的情况下保留。
- `GE_Weapon_Rifle_Damage_Debug`：名称来自早期调试阶段，但它现在是 `DA_Weapon_Rifle` 正式引用的权威伤害 GE。收口阶段不为纯命名收益改动已稳定的二进制引用链。

## 本次版本之外

以下内容已经由用户明确暂缓，不构成本次收口缺陷：

- 自由移动人机、行为树与 AI 攻击。
- 积分胜利 GameMode、HUD 胜利提示和独立击杀确认。
- 多人网络专项回归、Dedicated Server、延迟/丢包和预测加固。
- 切枪、通用配件系统、第二把武器和共享弹药类型。
- 护盾、进化资源、英雄小技能/大招/被动。

后坐力方向、快速重新开火、可视实体子弹和 AI 生命条均已人工验证通过。当前活动已经转为上述第三人称单步枪动画整合；下一轮先完成远端 TP Mesh/武器附着与 Lyra 持枪移动动画基线，再下发新的精简双端人工清单。

## 2026-09-18 首批实现进度

- 已从 Lyra 迁入 `SKM_Manny`、`SK_Mannequin`、Rifle Idle、四向 Walk/Jog 和 Jump/Fall/Land 序列。
- 已从 13 条动画序列中清除 LyraGame 专用脚步通知与 Animation Modifier 元数据；Apecox 继续使用自身的距离驱动脚步声音。清理后全部序列可在未启用 LyraGame 的 Apecox 中加载，审计为 0 错误。
- 已移除迁移附带、依赖 Lyra `PhysicalMaterialWithTags` 的 PhysicsAsset/PhysicalMaterial 引用。本阶段角色碰撞继续由 Character Capsule 负责，不使用 Lyra Ragdoll。
- 已创建 `/Game/Blueprints/Characters/Animations/ThirdPerson/BS_Apecox_Rifle_TP_Locomotion`：Direction `-180..180`、Ground Speed `0..900`、20 个 Idle/Walk/Jog 采样点；900 速度行使用 1.5 倍 Jog 播放率。
- 已创建 `/Game/Blueprints/Characters/Animations/ThirdPerson/ABP_Apecox_Rifle_TP`，Target Skeleton 为 Lyra `SK_Mannequin`，父类为 `UApecoxCharacterAnimInstance`。
- 本轮人工工作只需把 Blend Space 接到 AnimGraph，并在 `BP_ApecoxPlayerCharacter` 上把远端 Mesh/Anim Class 换成 `SKM_Manny` 与新 TP AnimBP。详细步骤见 `Current_UE_Manual_Steps.md`。
- 当前验收只覆盖 Rifle Idle 与四向 Walk/Jog 的双端可见性和同步。Jump、Crouch、Aim、Fire、Reload、IK 按后续小轮次加入。

## 2026-09-18 第三人称基础移动验收与武器附着调查

- 用户已验证 Owner 第一人称未回归、远端 Manny/TP Weapon 可见、Rifle Idle 与四向 Walk/Jog 正常，本轮基础移动同步通过。
- 验收中发现 RAR TP 武器虽然附着右手，但与手部位置和轴向严重不匹配。切换为 Lyra `SK_Rifle + weapon_r + Yaw -90°` 后，武器 Mesh 和附着轴向已与 Lyra 原始配置一致，但角色仍呈普通站姿；因此附着配置是必要条件，却不是持枪姿势的完整实现。
- 已迁入 `/Game/Weapons/Rifle/Mesh/SK_Rifle` 及最小依赖，Apecox 独立加载审计为 0 错误、0 警告、无 LyraGame 运行时依赖。
- Lyra 原工程确实在更完整的 Linked Anim Layer 管线中使用 `MM_Rifle_Hipfire_OverridePose`、`UpperBodyMask` 和多层动态 Pose；但 Apecox 当前 Blend Space 已直接使用完整 Rifle Idle/Walk/Jog 序列，不能把 Lyra 的单个静态覆盖节点脱离其完整管线上下文照搬。

## 2026-09-18 取消空手主流程并补齐 Lyra 上半身持枪层

- `BP_ApecoxPlayerCharacter` 的 `StartingWeaponDefinition` 已保存为 `/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle`。
- 新 Pawn 在 Authority 的 `PossessedBy` 完成 ASC 初始化后，通过 `Inventory -> Equipment` 正式事务创建并装备默认步枪；这会复用既有弹药、GAS、复制摘要和 FP/TP Presentation，不创建只用于外观的假枪。
- 若 Controller 已保留 Primary 实例，则新 Pawn 直接重新装备该实例，不复制武器也不重置弹药；死亡清库存后的正常重生则创建一把新的默认步枪。
- 已从 Lyra 迁入 `/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Hipfire_OverridePose` 用于验证。实际 PIE 的权重 A/B 测试确认它不适合作为当前精简 AnimGraph 的常驻覆盖层；现行人工步骤已改为删除该层并让完整 Rifle Blend Space 直接输出。
- `ApecoxEditor Win64 Development` 完整构建成功；改动后全量 `Apecox.*` 自动化 67/67 成功。命令行已复读 `BP_ApecoxPlayerCharacter` CDO，默认武器引用确认为 `DA_Weapon_Rifle`。剩余验证只需要在 PIE 覆盖真实出生/重生和双端动画表现。

## 2026-09-18 第三人称当前验收边界

- 当前只处理 Rifle Idle、四向移动、四向冲刺，以及移动中的持续持枪。现有 `BS_Apecox_Rifle_TP_Locomotion` 用速度轴区分 Idle、Walk/Jog 和冲刺速度层。
- 用户把 `Layered blend per bone.Blend Weights 0` 从 `1` 改为 `0` 后，腿部移动立即恢复，证明 Blend Space 与运行时速度输入正常，异常来自额外的 `MM_Rifle_Hipfire_OverridePose` 覆盖层。
- 当前 Blend Space 内全部序列本身就是 Lyra Rifle 持枪动画，已经同时包含下半身移动和上半身持枪姿势。本轮最终图改为 Blend Space 直接输出，删除重复的静态 Hipfire Override 覆盖。
- 离线根骨轨迹检查确认 Walk 素材约为 `300 cm/s`、Jog 约为 `600 cm/s`。该段初版按 C++ 当时的 600/900 默认值布置；最终冷加载确认玩家蓝图实际为普通 400、冲刺 650，现行 Walk 行已改为 400，Sprint 650 在 600@1.0 与 900@1.5 的相同 Jog 动画之间插值。此前加入的 Stride Warping 没有可见收益，保持移除。
- 第三人称开火时枪械大幅旋转已登记，但按用户要求本轮不处理。后续需要换成与 Lyra `SK_Rifle_Skeleton` 兼容的 TP Weapon Montage，并把人物开火动作放入上半身 Slot。
- 左手精确贴合、Jump、Crouch、Prone、Aim Offset、Fire、Reload 和 Laser 均列入后续小轮次，不进入本轮验收。
- 最终 AnimGraph 仍按 `Rifle Locomotion → Aim Offset → UpperBody 动作 Slot → Left Hand IK` 扩展；静态持枪不再作为常驻覆盖层重复叠加，开火和换弹等有明确生命周期的动作以后再进入 UpperBody Slot。
- 新观察：进入游戏或重生的极短时间内，本地镜头顶部可能闪过两只抬高的手，随后消失。用户要求暂缓处理；当前只记录为出生/重生瞬时可见性问题，不修改角色代码，也不进入本轮验收。
- 编辑器冷启动后，内容浏览器与 AnimBP 预览曾显示静止持枪 Pose；打开 Blend Space 或断开、重连输出后恢复。根因已经定位到首版 Python 资产创建脚本：脚本直接写入 `sample_data` 后保存，跳过了 Blend Space 编辑器初始化时建立并序列化三角剖分/网格采样数据的原生流程。迁移后的 Lyra 动画、移除通知/Modifier/PhysicsAsset 等清理项不负责 Blend Space 求值，不是本问题主因。
- 已通过原生 Blend Space 编辑器初始化路径重建并保存 `BS_Apecox_Rifle_TP_Locomotion`，资产从 `9,521` 字节增长到 `41,826` 字节；随后刷新、编译并保存 `ABP_Apecox_Rifle_TP`。自动化 `Apecox.Animation.ThirdPersonBlendSpaceColdLoad` 已在全新无界面 UE 进程中通过，未打开资产编辑器便能在 Idle、Walk、左右 Jog 与 Sprint 输入点返回有效且归一化的动画样本。
- 用户已按要求在冷启动后、不打开两个动画资产的条件下直接进入双端 PIE，远端移动动画从首次运行开始正常播放。本轮第三人称 Rifle Idle、四向移动和冲刺基线正式通过。出生/重生瞬时闪手仍按用户要求暂缓。

## 2026-09-19 第三人称跳跃阶段启动

- 已复核 Lyra `ABP_Mannequin_Base` 与 `ABP_RifleAnimLayers`：Lyra 的基础状态机负责 JumpStart、JumpStartLoop、JumpApex、FallLoop 和 FallLand，Rifle Linked Layer 只提供对应 Rifle Sequence。Apecox 不复制其 Linked Layer、距离预测和落点预测框架。
- Apecox 已有 `Is Falling` 与 `Vertical Speed`，均直接来自 Character Movement；远端通过现有 CMC 移动复制获得相同语义，无需新增 Gameplay 状态或 RPC。
- 本轮使用已经迁入并清理完成的 `MM_Rifle_Jump_Start`、`MM_Rifle_Jump_Start_Loop`、`MM_Rifle_Jump_Fall_Loop` 与 `MM_Rifle_Jump_Fall_Land`，不再迁移新资产。以五状态精简状态机覆盖 Grounded、起跳、上升循环、下降循环和落地。
- 当前等待用户按 `Current_UE_Manual_Steps.md` 完成 `SM_Rifle_Locomotion` 接线，并只验证静止跳、移动跳和走落平台三条链路。
- 用户已确认状态选择基本正确；当前观察到 JumpRiseLoop 与 FallLoop 的左右腿主导姿势不同，素材切换合理，但首版过渡视觉偏僵硬。复核 Lyra 原图后确认其 JumpStart→JumpStartLoop 为 `0.0` 秒自动切换、FallLoop→FallLand 为 `0.30` 秒，Locomotion 输出后还接有 `Inertialization`。
- 当前调参改为：JumpStart→JumpRiseLoop `0.0`；JumpRiseLoop→FallLoop 使用 `0.18` 秒 Inertialization；FallLoop→Land `0.30`；Land→Grounded `0.15`。状态条件保持不变，先验证正确混合能否消除僵硬，再决定是否需要迁入 Lyra `MM_Rifle_Jump_Apex`。
- 用户复核后认为上升/下落的左右腿相位可以接受，但落地状态在物理接地后持续过久。根因是首轮只复制了 Lyra 的 `FallLoop→FallLand = 0.30` 混合时长，却没有复制其 `GroundDistance < 200` 的提前落地预测；Apecox 使用 `NOT Is Falling`，到转移成立时已经接地，因此相同的 `0.30` 会全部滞后到接地后。
- 当前修正只调整落地节奏：`FallLoop→Land` 缩短为 `0.10`，`MM_Rifle_Jump_Fall_Land` 在状态内使用 `Play Rate 1.50`，`Land→Grounded` 自动过渡缩短为 `0.10`。其余已确认的跳跃状态与过渡保持不变。若仍不自然，下一步应补 Ground Distance/落点预测来提前进入 Land，而不是继续无边界缩短动画。
- 用户确认落地衔接已经可接受，但希望更快恢复 Grounded。保持 `Land→Grounded` 自动规则和 `0.10` 秒 Crossfade，不再缩短混合时间；将 `Land` 状态内的 `MM_Rifle_Jump_Fall_Land` 播放倍率由 `1.50` 调到 `1.80`，把落地恢复阶段由约 `0.27` 秒缩短到约 `0.22` 秒。
- 用户已验证 `Play Rate 1.80` 的落地恢复节奏通过，第三人称 Jump/Fall/Land 阶段收口。

## 2026-09-19 第三人称蹲伏阶段启动

- 已核对 Lyra Rifle 蹲伏资产：`BS_MM_Rifle_Crouch_Walk` 是 Direction `-180..180` 的一维 Blend Space，包含前、后、左、右和后向闭环 5 个采样；静止使用独立的 `MM_Rifle_Crouch_Idle`。
- 首轮不使用 `MM_Rifle_Crouch_Entry` 和 `MM_Rifle_Crouch_Exit`。两者分别约 `0.73` 秒和 `0.90` 秒，直接加入当前精简状态机会在移动中长时间锁腿；先用 CMC 蹲伏真相驱动 Idle/四向 Walk，并通过短 Crossfade 表达姿态变化。
- Apecox 已有 `UApecoxCharacterAnimInstance::bIsCrouching`，直接读取 `CharacterMovement->IsCrouching()`；远端继续依靠 CMC/Character 原生复制，不新增 RPC、GameplayTag 或 AnimBP 自有蹲伏变量。
- 已通过 Lyra 原生 Migrate 迁入 `MM_Rifle_Crouch_Idle`、`BS_MM_Rifle_Crouch_Walk` 及其四条方向序列。没有用 Python 重建 Blend Space。下一步在 Apecox 编辑器关闭后清除 5 条新序列的 Lyra 专用通知和 Modifier 元数据并审计冷加载，然后下发精简 AnimBP 接线清单。
- 已在 Apecox 独立冷启动进程中完成资产清理与审计：5 条序列的 Notify 与 Animation Modifier 元数据均清零；二进制扫描不存在 `LyraGame`、`AnimNotify_Lyra` 或 Modifier 残留字符串；原生 `BS_MM_Rifle_Crouch_Walk` 未重建并能直接返回 `-180/-90/0/90/180` 五个有效采样。
- 根骨轨迹实测确认前、后、左、右蹲走素材的作者速度均约为 `300 cm/s`；玩家蓝图实际蹲速为 `230 cm/s`。当前没有加入 Stride Warping 或播放倍率，用户已确认四向蹲走观感通过，因此保持现状，不因作者速度差异重新打开该阶段。
- 当前下发 `Crouched` 单状态方案：内部用 `Is Moving` 在 `MM_Rifle_Crouch_Idle` 与 `BS_MM_Rifle_Crouch_Walk` 之间混合；以 `Is Crouching` 和 `Is Falling` 连接 Grounded、Crouched 与现有 FallLoop。首轮只验收远端持枪蹲伏 Idle、四向移动和走落平台。
- 用户已验证静止蹲伏、四向蹲走、脚步速度匹配和本地第一人称回归通过。走落平台失败并非 AnimBP 问题，而是 UE `UCharacterMovementComponent` 默认关闭 `bCanWalkOffLedgesWhenCrouching`，使蹲姿胶囊在悬崖边缘被移动逻辑主动拦停。
- 已在角色构造函数中明确设置 `CMC->bCanWalkOffLedgesWhenCrouching = true`，继续复用 CMC 原生预测、服务器校正和 Falling 切换；现有 `Crouched→FallLoop` 动画转移无需修改。用户已验证蹲姿能够正常走下平台。`ApecoxEditor Win64 Development` 正式构建成功，`Apecox.Movement.CombatRules.CrouchedMove` 专项自动化成功，第三人称蹲伏阶段收口。

## 2026-09-19 第三人称 Aim Offset 阶段启动

- 已核对 Lyra 原始实现与资产。Apecox 本轮使用 Lyra 原生 `AO_MM_Rifle_Idle_Hipfire` 和 `AO_MM_Rifle_Crouch_Idle`，不复制 Lyra 的 Linked Anim Layer、Turn-in-place 或完整瞄准框架。
- 两套 Aim Offset 已通过 Unreal 原生 Migrate 迁入；每套都保留 15 个原生采样点，Yaw 范围 `-180..180`、Pitch 范围 `-90..90`。30 条采样序列的 Lyra 专用通知和 Animation Modifier 元数据已经清除，冷加载审计为 0 错误，二进制扫描无 `LyraGame`、`AnimNotify_Lyra` 或 Modifier 残留。
- Apecox 已有 `UApecoxCharacterAnimInstance::AimPitch`，它使用 `BaseAimRotation - ActorRotation` 计算俯仰差；第三人称远端角色可通过 Character 原生视角复制获得相同语义，本轮不新增 RPC 或 AnimBP 自有瞄准变量。
- 首轮只把 `Aim Pitch` 接入两个 Aim Offset，Yaw 固定为 `0.0`。现有角色已经随控制器 Yaw 转身，重复输入 AimYaw 会造成上半身侧扭；水平独立瞄准与 Turn-in-place 留到后续专门阶段。
- 复核 Lyra 导出的真实图后，确认 `ABP_Mannequin_Base` 把包含 JumpStart、JumpLoop、FallLoop 和 Land 的完整 Locomotion Pose 直接传给 `FullBody_Aiming`，没有 `Is Falling` 绕过。Aim Offset 是叠加姿势，不需要单独的空中瞄准动画。Apecox 因此取消首版“腾空绕过”设计，主图改为 `Rifle Locomotion Cached Pose → 站立/蹲伏 Aim Offset → Inertialization → Output`；腾空使用站立 Rifle Aim Offset，地面蹲伏使用 Crouch Aim Offset。
- 第三人称趴姿明确登记并搁置。Lyra Manny Rifle 资产集没有配套的趴姿持枪 Idle/四向爬行动画，后续需单独寻找或重定向资产并建立 Prone 状态；当前 Aim Offset 阶段不使用蹲姿替代趴姿。
- 当前人工工作只需按 `Current_UE_Manual_Steps.md` 修改 `ABP_Apecox_Rifle_TP` 主 AnimGraph，并验证站立、移动、蹲伏时的上下瞄准和跳跃回归。
- 用户已验证站立、四向移动、蹲伏、水平转向、腾空 Aim Offset、跳跃动作和第一人称回归均正常。唯一新增缺口是跳跃过程中叠加 Aim Offset 后，左手没有持续贴合步枪护木。
- 根因不是 Aim Offset 或 Jump 序列缺少空中动画，而是 Apecox 精简主图在 Aim Offset 后直接输出，尚未实现 Lyra 的手部骨骼约束层。Lyra 在 `FullBody_Aiming` 后继续执行 `Hand IK Retargeting → Copy Bone (VB IK_Hand_L_weaponSpace 到 ik_hand_l) → Two Bone IK (hand_l 到 ik_hand_l)`，以右手/`weapon_r` 为枪械基准重新约束左手。
- 当前人工清单已收缩为上述最小 Lyra 手部 IK 链。它直接使用已迁入 `SK_Mannequin` 中存在的 `VB IK_Hand_L_weaponSpace`、`ik_hand_gun` 和 `ik_hand_l`，不新增枪械 Socket，不复用第一人称 `LeftHandGripPoint`，也不要求手调第三人称握持点。
- 用户已验证该手部 IK 链通过：站立、移动、蹲伏与腾空 Aim Offset 下左手均能持续贴合枪身，跳跃和地面动作未发生回归。Aim Offset 与左手 IK 阶段正式收口。

## 2026-09-19 第三人称开火阶段启动

- 已读取 Lyra 原生开火资产。人物 `AM_MM_Rifle_Fire` 使用 `SK_Mannequin`，时长约 `0.533` 秒，Slot 为 `FullBodyAdditivePreAim`；其底层 `MM_Rifle_Fire` 是 Mesh Space Rotation Offset Additive。Lyra 将该 Slot 放在 Aim Offset 之前，再执行瞄准与手部骨骼约束。
- Lyra 枪械使用独立的 `AM_Weap_Rifle_Fire`，底层为 `Weap_Rifle_Fire`，目标骨架明确是 `SK_Rifle_Skeleton`，时长同为约 `0.533` 秒。
- Apecox 当前 `ThirdPersonWeaponMesh` 已是 Lyra `SK_Rifle`，但 `ThirdPersonWeaponFireMontage` 仍引用 RAR 第一人称 `AM_Apecox_Rifle_FP_Weapon_Fire`，其目标骨架为 `SKEL_RAR_AssaultRifle`。这条跨骨架引用是开火时整枪大幅旋转的直接原因。
- 当前人物 Montage 已包装兼容 Manny 的 `MM_Rifle_Fire`，但 Slot 仍是 `DefaultSlot`；`ABP_Apecox_Rifle_TP` 也尚无对应 Slot 节点。因此下一步会把人物 Montage 改到 `FullBodyAdditivePreAim`，并在现有 Aim Offset 之前接入同名 Slot。
- `PlayMontageOnMesh` 已加入骨架兼容性保护：Montage 与目标 Skeletal Mesh 的 Skeleton 不一致时直接拒绝播放，避免错误配置再次把整把枪旋转。待编辑器关闭后构建验证。
- 已通过 Lyra 原生 Migrate 把 `Weap_Rifle_Fire` 与 `AM_Weap_Rifle_Fire` 迁入 Apecox；没有重新生成动画二进制。`DA_WeaponPresentation_Rifle.ThirdPersonWeaponFireMontage` 已保存为该 Lyra 武器 Montage。
- `AM_Apecox_Rifle_TP_Character_Fire` 已重写并冷启动复读，唯一 Slot Track 为 `FullBodyAdditivePreAim`；其 Manny Skeleton 与底层 `MM_Rifle_Fire` 保持不变。
- `ApecoxEditor Win64 Development` 完整构建成功。新增 `Apecox.Animation.ThirdPersonFirePresentationColdLoad` 会检查人物 Slot、武器 Mesh/Montage 非空和武器骨架一致性；它与既有 Blend Space 冷启动测试共 2 项，均在全新无界面 UE 进程中成功。
- 当前只需在 `ABP_Apecox_Rifle_TP` 中把 `Slot (FullBodyAdditivePreAim)` 插到 `SM_Rifle_Locomotion` 与 `Save Cached Pose (RifleLocomotion)` 之间。这样开火 Additive 在 Aim Offset 之前求值，而左手 IK 仍在最后重新锁回枪身。

## 2026-09-19 第三人称开火验收与换弹阶段启动

- 用户已验证静止/移动/蹲伏/腾空开火、左手贴合、枪械机械动作和第一人称回归全部通过；开火人工步骤已从当前清单移除。
- Lyra 原人物换弹 `AM_MM_Rifle_Reload` 同时包含 `UpperBody` 主姿势和 `UpperBodyAdditive` 轨，人物与 `AM_Weap_Rifle_Reload` 枪械动画均为 `2.2s`。其人物 Montage 还包含 `AN_PlayWeaponMontage`、`AN_Reload` 和 Lyra 音频 Notify，不能直接用于 Apecox 的玩法事务。
- 已通过 Lyra 原生 Migrate 迁入 `MM_Rifle_Reload`、`MM_Rifle_Reload_Additive`、`Weap_Rifle_Reload` 与 `AM_Weap_Rifle_Reload`。三条序列的 Lyra Animation Modifier 元数据和 Notify 已清除，动画曲线保留。
- 已用 Unreal 原生 `AnimMontageFactory` 从 `MM_Rifle_Reload` 创建干净的 `AM_Apecox_Rifle_TP_Character_Reload`，唯一 Slot 为 `UpperBody`，没有 Lyra 专用 Notify。`DA_WeaponPresentation_Rifle` 已配置人物与枪械第三人称换弹 Montage。
- `UApecoxEquipmentComponent` 新增公开复制的 1 字节换弹表现摘要：服务器仍从 OwnerOnly 武器实例读取普通/空仓换弹真相，Owner 与 Simulated Proxy 只消费该公开状态。第一人称继续播放原 RAR 普通/空仓 Montage；远端第三人称显式同步播放 Manny 与 `SK_Rifle` Montage。
- Lyra TP 原动画长度 `2.2s` 会分别缩放到 Apecox 普通换弹 `2.366667s` 与空仓换弹 `2.8s`；扣弹提交点和结束许可仍完全由服务器 `ReloadConfig` 计时器决定，Montage 与 Notify 不修改弹药。
- `UApecoxCharacterAnimInstance::ThirdPersonLeftHandIKAlpha` 使用 `1 - DisableLHandIK`。该原生 Lyra 曲线只在约 `0.33–0.77s` 的取弹匣阶段释放左手 IK，其他时间继续使用已验证的手部约束。
- `ApecoxEditor Win64 Development` 完整构建成功。`Apecox.Animation.ThirdPerson*` 冷启动自动化 3/3 成功；`Apecox.Weapon.Reload` 玩法回归 7/7 成功。
- 用户随后已完成 `UpperBody` + `UpperBodyLowerBodySplitMask` 层和左手 IK Alpha 接线；本阶段的最终验收结果记录在下一节。

## 2026-09-19 第三人称换弹验收与剩余范围

- 用户已验证第三人称普通换弹、空仓换弹、移动/蹲伏换弹、左手 IK 释放与复位、人物与枪械动画同步以及第一人称回归全部通过。换弹阶段正式收口，人工步骤已从 `Current_UE_Manual_Steps.md` 移除。
- 当前第三人称单步枪主线已经覆盖出生持枪、Idle、四向移动、冲刺、跳跃、蹲伏、上下瞄准、左手贴合、开火和换弹。
- 用户此前明确要求的第三人称表现中，只剩镭射尚未同步。代码审计确认当前 `ToggleLaserPresentation`、镭射挂件、Beam 和 Dot 只驱动 `FirstPersonWeaponPresentationActor`。
- 第三人称趴姿继续按用户要求搁置。出生/重生瞬时闪手已经修复；水平独立瞄准/Turn-in-place 已完成。第三人称检视同步已经由用户取消，拾取/装备动作不作为当前完成条件。
- 自由移动 AI、积分胜利与 HUD 胜利提示、网络专项加固、切枪/配件/第二把武器、护盾/进化/英雄技能仍属于暂缓能力包，不与当前第三人称单步枪主线混在一起。

## 2026-09-19 第三人称镭射同步实施

- 用户确认出生/重生瞬时闪手已经修复，该项从剩余问题移除。
- 现有第一人称镭射仍由本地 `bLaserRequestedOn` 即时驱动；新增 `bLaserPresentationEnabled` 只复制当前实际可见状态。Autonomous Proxy 只在用户开关或生命周期导致可见性变化时发送可靠 RPC，不在 Tick 中重复发送相同值。
- 非拥有者视角为 `ThirdPersonWeaponMeshComponent` 创建本地纯表现镭射宿主，复用 RAR 的 `SM_RAR_ATT_AssaultRifle_Laser`、`SM_IG_Laser_Beam` 和 `MI_IG_Laser_Dot`。宿主 Actor 不复制、不承载玩法状态，只消费服务器公开摘要。
- Lyra `SK_Rifle` 没有 RAR 的 `SOCKET_Laser`，但提供 `Muzzle` Socket。Data Asset 新增 `ThirdPersonLaserAttachmentSocketName=Muzzle` 和相对挂载变换，初始位置为 `Location=(-35, 3.5, -4)`，用于把 RAR 挂件从枪口锚点移回护木侧面。
- 第三人称 Trace 不读取第一人称 `AimAlpha`；第一人称 ADS 继续使用相机中心汇聚，避免远端表现与本地瞄具红点相互耦合。
- 已新增冷加载自动化，检查第三人称武器 Socket、RAR 挂件发射 Socket、表现资产、可生成的轻量宿主类以及镭射 RepNotify 属性。
- `ApecoxEditor Win64 Development` 完整构建成功；第三人称表现自动化 4/4 成功；全量 `Apecox.*` 共 71 项全部成功，其中 70 项无警告、1 项仅附带既有的外网连通性超时警告。报告位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaser`。
- 当前仅等待 `Current_UE_Manual_Steps.md` 中的双端可见性、动作门控、挂件位置和生命周期人工验收。
- 首轮人工验证确认实体挂件与复制显隐正常，但 Beam 沿斜后/近似垂直方向发射。根因是 RAR 挂件自身 `SOCKET_Laser` 在原护木坐标系内带 `Yaw=90°`，不能直接作为 Lyra 第三人称枪管前向。现已改为仍以挂件发射口作为 Trace 起点、以 Lyra `Muzzle` Socket 的 X 轴作为第三人称方向源；挂件位置与方向计算解耦，后续外观微调不会再改变射线朝向。
- 方向修复后 `ApecoxEditor Win64 Development` 完整构建成功；第三人称专项 4/4、全量 `Apecox.*` 71/71 成功，0 失败。唯一日志警告仍是既有的 Google `generate_204` 连通性超时。证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserDirection`，当前只待画面方向和余下双端行为人工复验。
- 第二轮截图确认 Beam 已沿枪管朝前，但实体挂件偏低且本体方向仍不正确。这证明上一轮修复解决了 Trace 症状，挂载坐标系仍需纠正。根据 RAR 挂件发射 Socket 的 `Yaw=90°`，第三人称挂载变换改为 `Yaw=-90°` 进行精确抵消；高度由 `Z=-4` 上移至 `Z=-2`。用户可在当前编辑器直接修改 `DA_WeaponPresentation_Rifle` 并即时复验，最终数值确认后再构建收口。
- 用户确认 `Location=(-35, 3.5, -2)`、`Rotation=(Roll 0, Pitch 0, Yaw -90)` 后挂件位置与朝向正确。后续验证暴露三项运行时问题：开火时光束随 TP Weapon `Muzzle` 机械动画来回抽动；冲刺/换弹会错误隐藏镭射；死亡时独立挂件宿主未随旧 Pawn 隐藏而漂浮残留。
- 当前修正把第三人称方向改为角色复制的 `BaseAimRotation`，同时保留挂件真实发射口作为起点；镭射开启意图不再受冲刺、换弹或检视门控；`OnDeathStarted` 在所有端立即调用 Equipment 的死亡表现清理，先关闭公开状态再销毁 FP/TP 表现与独立镭射宿主。
- 修正后 `ApecoxEditor Win64 Development` 完整构建成功。新增 `Apecox.Movement.CombatRules.LaserPersistsDuringWeaponActions` 验证冲刺、换弹和检视均保持镭射可用，专项 1/1 成功；全量 `Apecox.*` 增至 72 项并全部成功，0 失败。唯一警告仍是既有的 Google `generate_204` 连通性超时。证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserRuntimeFix`，当前只待四项精简人工验收。
- 后续人工复验纠正了上一版假设：动作期间继续以 `BaseAimRotation` 驱动光束会让枪械已被冲刺/换弹动画转开时，镭射仍追随鼠标方向。当前改为 Beam 起点和方向均取实体挂件发射 Socket；挂件已用 `Yaw=-90°` 校准到枪管，因此射线真实跟随枪管。冲刺、换弹、检视和装备门控恢复为暂时关闭 Beam/Dot 并保留开启意图。
- 连续开火仍会带动实体挂件，根因是它挂在会被枪械 Montage 动画化的 `Muzzle`。当前用 `Muzzle` 只完成一次性摆放，再保持世界变换转挂到 Lyra `SK_Rifle` 的稳定 `Root` 骨骼，使挂件随整枪移动但不消费 Barrel/Muzzle 机械回弹。
- 网络复验发现明确的不对称：Host 杀死 Client 时 Client 无残留；Client 杀死 Host 时 Client 会留下 Host 的本地第三人称挂件。该对象是不复制的观察端表现 Actor，销毁时序不能只依赖一次死亡委托。当前增加死亡状态生成门禁、死亡状态立即关闭，以及表现 Actor 对 Owner 死亡/隐藏/销毁的逐帧自清理三层保证。代码待编辑器关闭后完整构建与自动化验证。
- 最终修正后 `ApecoxEditor Win64 Development` 完整构建成功；全量 `Apecox.*` 72/72 成功、0 失败，其中 71 项无警告，1 项仅包含既有的 Google `generate_204` 网络超时。`LaserActionGating` 和 `ThirdPersonLaserPresentationColdLoad` 均成功。证据位于 `Saved/Diagnostics/2026-09-19_ThirdPersonLaserLifecycleFix/Tests`，当前只待人工清单中的三项双端画面验收。
- 用户已确认第三人称挂件稳定、枪管方向、动作门控和双向死亡清理全部通过，第三人称镭射阶段收口。新增的第一人称细节是：完整 ADS 静止时瞄具红点和镭射重合，但快速移动鼠标时短暂分离。
- 根因是第一人称 Look Sway 在完整 ADS 仍保留 25% 武器滞后：瞄具红点跟随武器模型，镭射落点则汇聚到相机中心。当前改为 Look Sway 最终输出按 `1-AimAlpha` 衰减，完整 ADS 精确归零；内部弹簧仍更新，退出 ADS 时平滑恢复腰射摆动。新增 `Apecox.Camera.LookSway.ADSLock` 自动化，代码待完整构建验证。
- ADS 动态重合修正后 `ApecoxEditor Win64 Development` 完整构建成功；`Apecox.Camera.LookSway.ADSLock` 专项 1/1 成功；全量 `Apecox.*` 增至 73/73 成功、0 警告、0 失败。证据位于 `Saved/Diagnostics/2026-09-19_ADSLaserLock`，当前只待快速转向的单项画面验收。
- 用户确认 ADS 快速转向时瞄具红点与镭射落点稳定重合，腰射 Look Sway 回归正常。本轮人工清单清空，M2 第三人称单步枪行为同步正式收口。
