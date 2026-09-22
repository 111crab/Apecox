# 当前代码设计

更新日期：2026-09-22

本文只描述正式收口后的运行时结构。历史方案、已删除分支和过程性参数保留在 `Reports`、`Reviews` 与 `Research_Notes` 中，不再作为当前实现依据。

## Gameplay Framework 与 GAS

- `AApecoxGameMode` 负责服务器出生、延迟重生、击杀归因、队伍加分和目标分胜负裁定；`AApecoxGameState` 复制比赛阶段、双方比分、目标分和胜方。
- `AApecoxPlayerState` 持有 `UApecoxAbilitySystemComponent` 与 `UApecoxVitalAttributeSet`，并复制 Kills、Deaths、CombatTeam、Health、Shield、MaxShield 与 ShieldEvolutionPoints；ASC 使用 Mixed 复制，Character 是当前 Avatar。护盾成长随 PlayerState 跨 Pawn 重生保留，AI 阵营初始化时明确清零。
- `UApecoxHealthComponent` 绑定 ASC 的生命变化并管理 `NotDead → DeathStarted → DeathFinished`。Authority 每次生命耗尽只向 GameMode 报告一次 Victim/Killer，随后由 `UApecoxDeathAbility` 取消普通能力、清空输入和结束 Pawn 生命周期。
- `UApecoxAbilitySet` 负责 Authority 授予与成组撤销。基础 Pawn AbilitySet 只授予死亡 Ability；步枪 AbilitySet 只授予 `UApecoxProjectileFireAbility`。
- 所有 Apecox GameplayAbility 使用 `InstancedPerActor`。项目不存在运行时 Hitscan 分支和战术自伤调试 Ability。

## 输入、库存与装备

- 输入链为 `InputAction → UApecoxInputConfig → Native GameplayTag → Character/ASC`。
- 开火进入 ASC 的 Pressed/Held/Released 缓存；移动、跳跃、姿态、探头、ADS、检视、换弹、交互和镭射由 Character 按各自事务处理。ADS 只绑定 Enhanced Input 的 `Started`：第一次点击设置持久瞄准意图，松键不处理，第二次点击清除；冲刺、换弹、死亡和卸枪仍强制清除。
- `UApecoxInventoryComponent` 位于 PlayerController，保存 OwnerOnly FastArray 库存和主/副武器槽结构。
- `UApecoxEquipmentComponent` 位于 Character，管理当前武器实例、公开装备摘要、FP/TP 表现和 Montage/SFX 生命周期。
- `UApecoxWeaponDefinition` 保存玩法配置与 AbilitySet；`UApecoxRangedWeaponInstance` 保存弹匣、备用弹药、本地/权威连发状态；`UApecoxWeaponPresentationDefinition` 保存视觉与声音资源。

## 射击、弹道与反馈

- `UApecoxProjectileFireAbility` 继承 `UApecoxRangedFireAbility`，复用输入、预测、射速、装备校验、TargetData 和服务器提交框架。
- `FApecoxRangedShotTargetData` 只传递射击意图、ShotId、BurstId 与 BurstShotIndex。Authority 重新校验原始视线并用确定性随机流生成最终椭圆锥散布。
- `AApecoxWeaponProjectile` 只在 Authority 上模拟真实碰撞和伤害；视点到玩法枪口的额外 Sweep 防止近墙从障碍物背后生成子弹。旧的复制弹丸网格不再承担高速可视化：短距离弹丸可能在首个网络更新前命中，而且位置复制会形成延迟鬼影。
- 每发通过 Authority 校验并成功生成 Projectile 后，Equipment 通过 `NetMulticast, Unreliable` 发送该发的实际玩法起点、散布后方向、随机弹速、重力和初始可视距离。各相关渲染端生成不复制、无碰撞、最长 `0.75s` 的 `AApecoxProjectileTracer`，复用 RAR 发光子弹网格并沿飞行轴拉伸约 2 米、把 Emissive 从 100 提到 250。它只表达服务器已接受的射击，不判定命中、伤害或弹药；Dedicated Server 不创建该表现。
- `FApecoxRangedFireConfig.BaseDamage` 是单发基础伤害真相，当前步枪为 `13`。发射时以原生 `UApecoxWeaponDamageEffect` 创建 Instant Spec，并通过 `SetByCaller.Damage` 写入有符号数值；命中时附加 HitResult 并应用，卸枪不会取消已经发射的子弹。旧 `GE_Weapon_Rifle_Damage_Debug` 不再参与运行时伤害。
- `UApecoxVitalAttributeSet` 在 Authority 的统一结算点把负 Health Modifier 重分配为“护盾优先、溢出生命”，并按实际扣除值向敌方玩家授予等量护盾进化点。过量伤害不产生额外点数，友军、自伤和 AI 来源不成长；击中护盾也会被 Projectile 识别为有效命中。
- Impact GameplayCue 负责 Niagara、命中声与贴花，只做表现，不判定伤害或死亡。
- 枪口表现使用一个 `NS_IG_MuzzleFlash`，其中同时包含火光和烟雾 Emitter。第一人称枪械与 Niagara 使用同一 FirstPerson 图元类型。

## 散布、后坐与准星

- 连发散布按 BurstShotIndex 采样 RAR 曲线，叠加移动项并应用 ADS 倍率；客户端与 Authority 使用相同种子得到可复核方向。
- 镜头后坐只在 Kick 阶段把 RAR 曲线的 Pitch/Yaw 增量写入真实 `ControlRotation`。这些增量与玩家鼠标输入共同形成新的准星方向，停火、弹匣打空和内部弹簧恢复都不反向修改该方向。每轮新连发的第 1 发会清除上一轮尚未归零的内部镜头弹簧值和速度，但保留真实 `ControlRotation`；因此短暂停顿后重新开火不会把上一轮静默恢复误写成向下跳动。RAR 曲线 X 分量代表 Roll，只参与内部采样，不写入玩家 `ControlRotation`，因此不会留下地平线侧倾。
- 枪械模型后坐仍独立回稳，并在 AnimGraph 中先作用于 `ik_hand_gun`，再由左手 FABRIK 把左手重新贴回握点。
- HUD 准星根据移动与腾空状态扩散，并在腰射保留中心点；它只表达当前精度趋势，不参与弹道计算。ADS 隐藏腰射准星。服务器最终确认实体 Projectile 对有效目标造成护盾或生命伤害时，线宽 `1.5` 的红色四段 X 短暂叠加在原腰射准星上，ADS 也显示同一 X；Miss、墙体碰撞和仅成功发射不触发。Projectile 最终回执同时携带实际受伤 Actor，拥有者 HUD 只揭示自己命中的那个 AI 血条；再次命中刷新约 `4s` 显示期，未攻击目标、超时、死亡和重生后的目标保持隐藏。生命数值仍直接读取目标 HealthComponent，不复制第二份生命。左下战斗面板显示生命、当前/最大护盾、白/蓝/紫品质与下一等级剩余点数；右下显示枪名、弹匣和备用弹药；顶部显示双方 `0/15` 比分。数据分别直接读取 PlayerState/AttributeSet、WeaponInstance 与复制的 GameState；`PostMatch` 仍显示 `VICTORY / DEFEAT`。

## 护盾成长与地图补给

- 玩家初始 `100` 生命。护盾采用分段成本：白色到蓝色需要 `500` 点，蓝色到紫色再需要 `1000` 点，因此累计等级为白色 `0..499 → 25`、蓝色 `500..1499 → 50`、紫色 `1500 → 75`；点数在 `1500` 封顶。普通伤害升级只立即提供新增的 `25` 容量，不回满先前损失的护盾。
- `AApecoxCombatPickup` 是可直接放入地图的复制 Actor。服务器 Overlap 只接受存活的 Players 阵营角色；可用状态控制两端碰撞、网格、标签与灯光，默认 `20s` 后重生。
- Health Pack 默认恢复 `100` 当前生命并钳制到 MaxHealth，不提高 MaxHealth；满血不消耗。Shield Battery 先增加 `300` 进化点，再把升级后的护盾补满；紫色满盾时不消耗。AI 不会触发两种补给。
- Pickup 的默认圆柱、文字与灯光仅用于玩法验证。`PickupMesh` 是独立表现插槽，用户导入美术后可在实例上替换 Static Mesh，不改服务器规则或网络状态。当前演示映射为 `SM_Box_2` 血包与 `SM_Box_9` 护盾电池；Actor 在组件级动态材质实例上写入 `Emissive_Color` 和 `Emissive_Power`，按类型自动显示红/蓝，不修改 `SciFi_Props` 原始材质资产。

## 第一人称表现

- `ABP_Apecox_FirstPersonArms` 的原生父类是 `UApecoxFirstPersonAnimInstance`。它读取移动、腾空、姿态、探头、ADS、Look Sway、Turning、Weapon Recoil 和左手 IK 数据。
- RAR 动画负责持枪 Idle/Walk/Sprint/Jump/Crouch/Prone/Lean；开火、装备、换弹和检视通过成对的 Arms/Weapon Montage 播放。
- 检视、换弹和装备事务会按需关闭左手 IK，结束或中断后恢复。常态射击由 FABRIK 跟随武器握点，避免后坐时左手脱离护木。
- 空手状态隐藏第一人称手臂；项目不继续维护空手第一人称动画表现。
- ADS 使用持久输入意图 `bAimInputRequested`、即时规则状态 `bIsAiming` 与平滑 `AimAlpha`。固定一倍镜、FOV、灵敏度、镭射汇聚和瞄准姿势均由当前武器表现配置驱动。第一人称 Look Sway 的最终输出按 `1-AimAlpha` 衰减，完整 ADS 时精确归零，使随武器模型移动的瞄具红点与汇聚到相机中心的镭射落点保持同一屏幕参考；腰射摆动和内部弹簧状态继续正常更新。
- 左右探头由完整 RAR 上半身 Lean Pose 与相机侧移/Roll 共同组成，并带墙体 Sweep 限制。

## 第三人称表现与同步

- 玩家仍只用第一人称操控；第三人称层只向其他观察者表达同一套 Gameplay/GAS 状态，不另建一套玩法真相。`ABP_Apecox_Rifle_TP` 使用 Lyra Manny 兼容的步枪移动、跳跃、蹲伏、Aim Offset 与左手 IK，开火和换弹由复制事件/状态驱动第三人称 Montage。表现分流只有“本地玩家控制 Pawn”进入 FP 路径；服务器本地 AI 仍进入 TP 路径，避免 AI 换弹被错误播放到隐藏 Arms。网络客户端的 `EquippedWeaponState` 可能早于 Controller 占有到达，因此本地 `PawnClientRestart` 会幂等重建一次 Equipment 表现；它清除首次 RepNotify 误建的 TP 镭射并恢复 FP Arms/Weapon，不在 Listen Server 或 Standalone 重播装备动作。
- 第三人称枪械使用 Lyra `SK_Rifle`。角色动作只需要正确表达持枪、移动、姿态和武器事务，不追求逐帧复刻 RAR 第一人称动作。
- `UApecoxCharacterAnimInstance` 在每个观察端本地维护表现专用的 `RootYawOffset` 与 `AimYaw=-RootYawOffset`。静止接地时，它用 Actor Yaw 每帧增量的反值保持脚部视觉朝向；移动、腾空或趴姿时使用 Lyra 同参数的临界阻尼弹簧回到 0。首次绑定 Pawn/重生只建立当前 Yaw 基线，避免冷启动跳变。这些值不复制，也不修改 Actor、ControlRotation、Projectile 或镭射的玩法方向。
- 第三人称 AnimGraph 的水平瞄准层级为 `FullBodyAdditivePreAim -> Rotate Root Bone -> RifleLocomotion Cached Pose -> 站立/蹲伏 Aim Offset -> 左手 IK`。`RootYawOffset` 接 `Rotate Root Bone.Yaw`，`AimYaw` 接两套 Aim Offset 的 `Yaw`；左手 IK 继续最后求解，避免根偏移破坏握持。
- 静止接地且 `Abs(RootYawOffset)>35` 时，`bShouldTurnInPlace` 请求 90 度原地转身，`bTurnInPlaceLeft` 选择方向。该阈值由 Lyra 默认 50 度提前，以减少快速视角输入先撞到 RootYaw Clamp 的脚滑。站立/蹲伏分别使用 Lyra Rifle 的左右 90 度序列；不使用 180 度版本，也不把转身做成 Montage。
- `TurnInPlacePlayRate` 根据本帧 Actor Yaw 角速度和当前根偏移压力在 `1.0..1.6` 间调整。上限保持自然，避免原先 `3.0x` 造成高频跺小碎步；快速连续转向主要通过同方向状态重入接力，而不是无限提高单条序列速率。
- 四个 Turn 状态必须启用 `Always Reset on Entry`。持续大角度输入会在上一轮 Turn 尚有回混权重时重新进入同一个状态；UE 默认不会重置仍处于 Active 的状态，Sequence Player 会停留在上一轮末尾。强制重置后，每次 90 度请求都从序列起点重新消费，可以连续接力转身。
- 转身序列的 `TurnYawWeight` 表示当前混合权重。AnimInstance 先以该权重还原 `RemainingTurnYaw`，再用相邻帧差值逐步消费 `RootYawOffset`。同状态重入时曲线会从接近 0 跳回正负 90 度；该跳变只用于重建基线，不能作为真实旋转再次写入。`bShouldInterruptTurnInPlace` 在转身中检测到明显反向输入时要求状态机先退出旧方向，再由当前 `RootYawOffset` 选择新方向。移动、腾空、趴姿和 Pawn 重绑会清理曲线历史。状态机的 Sequence Root Motion 不驱动 Character 胶囊。
- `UApecoxEquipmentComponent::bLaserPresentationEnabled` 是公开复制的“当前实际可见镭射”摘要。拥有者立即更新第一人称表现，并且只在可见状态发生变化时提交 RPC；服务器校验已装备且武器支持镭射后复制给观察者，避免逐帧 RPC 和复制每帧命中点。
- 非拥有者为第三人称枪械创建本地纯表现 `AApecoxWeaponPresentationActor`。RAR 镭射挂件先用 Lyra `SK_Rifle` 的 `Muzzle` Socket 和 `ThirdPersonLaserAttachmentTransform` 完成一次性定位，再保持世界变换转挂到不参与枪械机械回弹的 `Root` 骨骼；实体挂件因此仍跟随整把枪和角色手部，但不会随每发 Barrel/Muzzle 动画抽动。
- Trace 起点和物理方向都来自实体挂件的发射 Socket。用户已经用 `Yaw=-90°` 把它校准到枪管前向，所以冲刺、换弹等动画转动枪械时，射线随实体枪管转动，不追随鼠标/镜头方向；Beam/Dot 由各观察端本地计算。第一人称 ADS 仍可按 `AimAlpha` 汇聚到本地视线，第三人称不使用观察者相机汇聚。
- 用户打开镭射后，冲刺、换弹、检视和装备表现会暂时关闭 Beam/Dot，并保留开启意图，动作结束后自动恢复。实体挂件在持枪期间始终留在枪身。死亡清理由三层保证：死亡委托主动销毁、死亡状态阻止 RepNotify 重新生成、第三人称表现 Actor 每帧发现 Owner 已死亡/隐藏/销毁时自行收口；这避免 Host/Client 不同复制顺序留下本地非复制挂件。

## 移动、声音与互斥规则

- Character 的移动速度真相为普通 `400 cm/s`、冲刺 `650 cm/s`、蹲伏 `230 cm/s`、趴姿 `170 cm/s`；C++ 默认值与 `BP_ApecoxPlayerCharacter` 覆盖值保持一致。`UApecoxCharacterAnimInstance::GroundSpeed` 直接读取实际水平速度，不做归一化。第三人称 Rifle Blend Space 的 Walk 行位于 `400`，因此普通移动稳定命中完整 Walk 动画；`600` 与 `900` 两行使用同一组 Jog 动画并分别设置 `1.0` 与 `1.5` 播放倍率，冲刺 `650` 在两行间插值得到约 `1.083` 倍 Jog，匹配 600 cm/s 作者速度到 650 cm/s 玩法速度。轴上限保留 `900` 只是采样域，不代表角色必须达到 900。
- 冲刺中按开火会解除冲刺，再按当前方向以走路或静止状态射击；按住开火时不能重新冲刺。
- 趴姿移动中按开火会清除移动输入和水平速度，切到静止趴姿射击；按住开火时不提交爬行输入。
- 趴姿按跳跃只尝试恢复站立；头顶空间不足时保持趴姿。
- 脚步按水平移动距离触发，落地使用真实 `Landed`，蹲伏/趴下声音只在姿态真正改变后播放。
- 普通和空仓换弹使用纯机械 SoundWave，不播放 RAR SoundCue 中混入的“Reloading”人声。

## 人机、比赛与保留依赖

- `AApecoxBotCharacter` 复用玩家的 ASC、生命、死亡和重生路径。Authority 被占有后直接把 `StartingWeaponDefinition` 装入现有 Equipment，因此 AI 与玩家共享同一个 Projectile Ability、弹药、换弹、Damage GE 和第三人称表现，不另造假武器。
- `AApecoxWanderAIController` 类名因 Blueprint 序列化兼容保留。无战斗目标时，Authority 从地图中的 `AApecoxAIObjectivePoint` 按权重随机选择争夺点，沿 NavMesh 到达后停留 `1–2s` 并每约 `0.6s` 改变环视方向，再选下一个点；多个 Controller 使用独立 `FRandomStream`，允许偶尔汇合但不会被同一全局序列驱动。
- 战斗 AI 使用三段距离：仅在 `2400 cm` 内且有 Line of Sight 时获取最近存活玩家；`1800 cm` 内才开火；已锁定目标超过 `3000 cm` 或持续丢失视线约 `2s` 后脱战并恢复争夺点巡逻。当前积极度为反应 `0.25–0.55s`、点射 `0.3–0.6s`、停火 `0.35–0.8s`、战斗换位间隔 `0.55–1.2s`；每轮仍保留稳定瞄准误差、随机横向/径向换位、ASC 开火和空仓换弹。首版不含 Behavior Tree、EQS、掩体决策和复杂路径战术。
- 正常比赛中 `apecox.AI.DamageEnabled` 默认是 `1`。由 Bot 发射的 Projectile 执行权威飞行、碰撞、Impact、13 点伤害和生命周期；调试观察时可以临时设为 `0`，只跳过 Damage GE，玩家武器和其他伤害来源不受影响。
- 玩家射击 TargetData 使用 PlayerController ViewPoint；服务器 AI 没有 PlayerController，因此使用 Pawn 的 `GetPawnViewLocation()` 与 `GetBaseAimRotation()`。两条路径最终仍经过同一 Authority 视点、角度、射速和弹药校验。
- AI 与玩家普通速度均为 `400 cm/s`，继续消费已经验收的 Rifle Blend Space；Bot PlayerState 的 `CombatTeam=AI` 是稳定的计分归属，Pawn 类型只作为 PlayerState 初始化前的回退。
- `NavigationSystem` 与 `AIModule` 用于 `MoveToActor` 和 NavMesh；`ModularGameplay` 继续用于 `UApecoxWeaponStateComponent : UControllerComponent`。
- `PostMatch` 后 GameMode 不再安排重生，AI 停止决策，Ranged Fire Ability 在本地预测和 Authority 提交两处拒绝新射击。

## 当前验证边界

- 正式收口以 Standalone/单人第一人称体验为产品验收主线。
- 第三人称移动、跳跃、蹲伏、垂直瞄准、开火、换弹、镭射、水平 Aim/RootYaw 补偿和 90 度 Turn-in-place 已完成 Listen Server 双端人工验证。
- 自动化覆盖姿态、相机、移动与开火互斥、Projectile、散布、后坐、换弹及关键生命周期；视觉、声音和动画最终观感仍由 PIE 烟雾检查确认。
