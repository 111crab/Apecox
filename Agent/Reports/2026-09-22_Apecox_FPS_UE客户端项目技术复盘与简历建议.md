# Apecox FPS：UE 客户端项目技术复盘与简历建议

更新日期：2026-09-22

## 1. 项目应该如何定位

### 一句话版本

Apecox 是一个使用 UE5 C++、GAS 和服务器权威实体弹丸实现的网络 FPS 垂直切片：玩家固定以第一人称操控，同一套 Gameplay 状态同时驱动本地第一人称表现、远端第三人称表现、AI、伤害、死亡、计分与 HUD。

### 这个项目真正有区分度的地方

项目的价值不在于“做了一把能开火的枪”，而在于把以下系统接成了可玩的闭环：

```text
Enhanced Input
  -> InputConfig / Native GameplayTag
  -> Character 事务或 ASC Ability 输入缓存
  -> Equipment / Weapon Instance
  -> 自定义 TargetData 表达射击意图
  -> 服务器重验视点、射速、弹药和装备来源
  -> Authority Projectile 碰撞
  -> GameplayEffect / AttributeSet 结算护盾与生命
  -> HealthComponent / DeathAbility
  -> GameMode 计分与重生
  -> GameState / PlayerState 复制
  -> FP / TP 动画、VFX、曳光、命中反馈和 HUD
```

这条链路同时体现 Gameplay Framework、GAS、数据驱动、网络同步、动画系统、表现与逻辑分离，以及客户端反馈设计，适合作为 UE 客户端面试的主项目。

## 2. 总体架构与职责边界

| 层 | 主要类型 | 项目职责 |
| --- | --- | --- |
| 比赛规则 | `AApecoxGameMode`、`AApecoxGameState` | 服务器裁定击杀、重生、比分和胜负；GameState 向所有端复制公共比赛状态 |
| 玩家持久状态 | `AApecoxPlayerState` | 持有 ASC、AttributeSet、K/D、队伍和玩家护盾成长；跨 Pawn 重生保留 |
| Pawn 生命周期 | `AApecoxPlayerCharacter`、`UApecoxHealthComponent` | 移动、姿态、输入事务、死亡状态机与表现入口 |
| 能力系统 | `UApecoxAbilitySystemComponent`、`UApecoxAbilitySet`、各 GameplayAbility | Tag 输入缓存、能力授予/撤销、开火和死亡能力生命周期 |
| 装备与库存 | `UApecoxInventoryComponent`、`UApecoxEquipmentComponent` | OwnerOnly 库存、当前装备真相、AbilitySet、公开装备摘要和 FP/TP 表现生命周期 |
| 武器数据 | `UApecoxWeaponDefinition`、`UApecoxWeaponPresentationDefinition`、`UApecoxRangedWeaponInstance` | 分离不可变玩法配置、可替换表现配置和每把武器的运行时弹药状态 |
| 射击结算 | `UApecoxProjectileFireAbility`、`FApecoxRangedShotTargetData`、`AApecoxWeaponProjectile` | 客户端提交意图，服务器校验并生成权威弹丸，命中后通过 GE 结算 |
| 动画表现 | `UApecoxCharacterAnimInstance`、`UApecoxFirstPersonAnimInstance`、两个 AnimBP | 从 Gameplay/CMC/Equipment 读取真相，分别组织 FP Arms 与 TP Manny 姿势 |
| AI | `AApecoxWanderAIController`、`AApecoxAIObjectivePoint` | 争夺点移动、局部目标发现、视线/射程判断、连发与换弹 |
| UI/反馈 | `AApecoxHUD` | 读取本地与复制状态，显示准星、命中确认、血量/护盾、弹药、比分和胜负 |

核心原则是“每类真相只有一个权威来源”。移动状态来自 CMC，能力与属性来自 ASC，装备来源于 Equipment，比赛结果来自 GameMode/GameState。AnimBP、HUD、Niagara 和音效只消费这些状态，不反向决定伤害、弹药或胜负。

## 3. 最值得讲的技术链路

### 3.1 PlayerState 持有 ASC：跨重生的 GAS 生命周期

#### 运行链路

1. `AApecoxPlayerState` 创建并持有 `UApecoxAbilitySystemComponent` 与 `UApecoxVitalAttributeSet`。
2. Character 被占有或 PlayerState 复制到达后，以 PlayerState 为 Owner、当前 Character 为 Avatar 调用 `InitAbilityActorInfo`。
3. 基础 `UApecoxAbilitySet` 在 Authority 授予死亡能力；装备步枪时由 Equipment 再授予武器 AbilitySet。
4. 卸下武器时按保存的 Granted Handles 成组撤销，只移除该武器授予的能力，不会误伤角色未来的英雄技能。
5. Character 死亡并销毁后，PlayerState 与 ASC 仍存在；新 Pawn 重新成为 Avatar。玩家的护盾等级和进化点可以跨重生保留，Pawn 级瞬时状态重新初始化。

#### 为什么这样设计

如果 ASC 在 Character 上，Pawn 销毁会同时销毁长期能力状态；放在 PlayerState 后，玩家身份、比分和成长数据天然跨重生存在。ASC 使用 Mixed 复制模式，让拥有客户端获得完整 GameplayEffect 信息，远端只接收必要的公开属性与 GameplayCue。

#### 面试价值

这条链路可回答：ASC 为什么放 PlayerState、Owner/Avatar 有何区别、何时初始化 ActorInfo、重生时如何防止重复授予、Mixed 复制模式解决什么问题。

### 3.2 数据驱动武器：Definition、Instance、Presentation 三分法

#### 三类对象

- `UApecoxWeaponDefinition`：武器类型的不可变玩法配置，包括射击配置、换弹配置、装备期间授予的 AbilitySet 和 Presentation Definition。
- `UApecoxRangedWeaponInstance`：某一把武器的运行时状态，包括弹匣、备用弹药、连发计数和射速账本。
- `UApecoxWeaponPresentationDefinition`：第一/第三人称 Mesh、Montage、VFX、声音、Socket 与挂载变换。

射击配置用受基类约束的 `TInstancedStruct<FApecoxWeaponFireConfig>` 表达不同弹道模型。当前只有正式的 Projectile 配置，但接口没有把步枪字段硬编码到 Character 或 Ability 中。

#### 为什么这比“大量散落的蓝图变量”更好

- 调平衡参数时不修改 Ability 流程。
- 替换第一人称 RAR 或第三人称 Lyra 资产时不修改伤害和弹药逻辑。
- 同类型武器共享 Definition，不同实例各自保存弹药。
- 装备组件只依赖抽象配置，减少 Character 对具体步枪资产的硬引用。

#### 简历表达重点

应写“将不可变配置、运行时状态和表现资源分层”，而不是夸大为“完成通用多武器框架”。当前项目只正式交付一把步枪，切枪、共享弹药类型和第二把武器仍是扩展边界。

### 3.3 GameplayTag 的合理使用：表达跨系统稳定语义

项目使用 Native Gameplay Tags 表达以下内容：

- 输入意图：`InputTag.Move`、`InputTag.Weapon.Fire`、`InputTag.Weapon.Reload` 等。
- 跨系统阻塞状态：`State.Input.AbilityBlocked`。
- 死亡事件与状态：`GameplayEvent.Death`、`State.Death.Dying`、`State.Death.Dead`。
- 表现通道：`GameplayCue.Weapon.Fire`、`GameplayCue.Weapon.Impact`。
- 参数语义：`SetByCaller.Damage`。

输入链是 `InputAction -> UApecoxInputConfig -> GameplayTag -> Character/ASC`。开火走 ASC 的 Pressed/Held/Released 输入缓存；移动、跳跃、蹲趴、探头、ADS、检视、换弹和镭射等事务由 Character 或 Equipment 显式处理。

这里的设计重点是没有把所有 bool 都机械地替换为 Tag：连续速度、AimAlpha、弹药数、RootYawOffset 等仍使用强类型数值；只有需要跨资产、跨系统匹配的稳定语义才使用 Tag。这样既获得配置化能力，也避免 Tag 容器变成不可追踪的全局状态仓库。

### 3.4 服务器权威射击：意图、校验、结算、表现分离

#### 客户端到服务器

1. 本地输入通过 `InputTag.Weapon.Fire` 激活 `UApecoxProjectileFireAbility`。
2. 客户端采集瞄准意图，构造 `FApecoxRangedShotTargetData`。它只携带意图标识、`ShotId`、`BurstId` 和 `BurstShotIndex`，不提交可信命中结果。
3. Authority 收到 TargetData 后重新检查当前装备来源、射速、弹药、比赛阶段和原始视线。
4. 服务器以确定性随机流计算最终椭圆锥散布，并从玩法枪口生成 `AApecoxWeaponProjectile`。
5. 视点到枪口额外做 Sweep，防止玩家贴墙时从墙后生成弹丸。

#### 命中到伤害

1. 只有 Authority Projectile 执行 Gameplay 碰撞和伤害。
2. 命中时构造 Instant GameplayEffect，通过 `SetByCaller.Damage` 注入基础伤害，并保留 `HitResult`。
3. `UApecoxVitalAttributeSet` 在统一结算点将负 Health Modifier 重分配为“护盾优先、溢出生命”。
4. 实际扣除值用于护盾进化点，过量伤害不会重复计分，自伤、友军和 AI 来源不会给玩家成长。
5. Health 归零后，HealthComponent 只在 Authority 上提交一次死亡事件；DeathAbility 取消普通能力并结束 Pawn 生命周期；GameMode 完成击杀归因、比分和重生。

#### 实体弹丸与曳光为什么分开

权威 Projectile 负责碰撞和伤害，但高速短寿命弹丸可能在首次网络位置更新前已经命中。项目没有强行让复制弹丸 Mesh 同时承担视觉反馈，而是在服务器接受该发射击后，用 `NetMulticast, Unreliable` 广播实际起点、方向、弹速和重力，各渲染端生成无碰撞、短寿命的本地 `AApecoxProjectileTracer`。

因此：

- 丢失一次曳光只影响视觉，不影响命中。
- 客户端不能用本地曳光伪造伤害。
- Dedicated Server 不创建视觉对象。
- 真实弹道与可见轨迹仍使用同一组服务器接受参数。

这是项目中“逻辑与表现分离”最完整的例子。

### 3.5 后坐力与压枪：玩家视角和模型表现分层

项目将后坐力拆为两个通道：

- Camera recoil：对 ControlRotation 写入每发增量，让准星和射击方向真实上移；停止射击后不把玩家视角拉回原方向，玩家的压枪输入会永久保留。
- Weapon model recoil：只影响第一人称武器姿势，可用弹簧平滑回中，不改变玩家最终瞄准方向。

这解决了两类常见错误：停止射击后强制大幅回正，以及玩家在射击间隔内压枪时因旧回正状态导致下一发视角突然下跳。散布则以 Burst 索引生成连续、可复现且在可控范围内变化的偏移，而不是每发完全无关的随机点。

### 3.6 第一人称动画：Pose Composition、Montage 和 IK 所有权

#### 动画数据从哪里来

`UApecoxCharacterAnimInstance` 只读取并缓存 Gameplay 状态：速度、方向、Falling、蹲伏、冲刺、趴姿、Aim、装备族等；它不选择具体动画资产。`UApecoxFirstPersonAnimInstance` 在此基础上增加 ADS、Look Sway、模型后坐力、Turning 和左手 IK 数据。

#### 姿势合成顺序

第一人称 AnimGraph 的核心不是一个无限膨胀的状态机，而是按职责合成姿势：

```text
基础持枪/移动姿势
  -> 地面/空中/蹲趴等状态层
  -> ADS、转向、探头等叠加层
  -> Equip / Reload / Inspect / Fire Montage
  -> 程序化 Weapon Recoil
  -> 左手 FABRIK 最终约束
```

顺序决定结果。程序化后坐力放在左手 FABRIK 之前，确保枪移动后左手仍能追随握点；换弹或检视期间，Montage 的动画曲线暂时降低 IK Alpha，把左手控制权交还给动画，动作结束后再恢复约束。

#### Arms 与枪械 Montage 配对

换弹、装备和检视会同时播放 Arms Montage 与 Weapon Montage。弹药提交点和事务结束时间由服务器换弹配置决定，Montage/Notify 负责表现，不直接修改弹药。不同视角动画长度不同时，通过播放倍率对齐同一个 Gameplay 事务窗口，而不是让动画时长成为服务器规则。

#### 可讲的排错案例

- 左手穿模或悬空：不是简单调整 Socket，而是检查最终 IK 的空间、链根、Effector 和求值顺序。
- 开火时左手偏移：说明 Fire Montage 或程序化后坐力在 FABRIK 后覆盖了手臂，需要重排求值或用 IK Alpha 管理所有权。
- 冷启动静止 Pose：资产迁移/保存与派生数据问题要通过全新编辑器进程冷加载验证，不能只依赖当前编辑器内重新连线后的热状态。

### 3.7 第三人称表现：共享 Gameplay 语义，使用独立资产和姿势

玩家始终使用第一人称操控，第三人称只负责让其他观察者看到正确行为。因此项目没有追求 FP 与 TP 逐帧复刻，也没有让不同骨骼强行共享同一套 Animation Sequence。

#### 共享内容

- CMC 的移动、跳跃和蹲伏状态。
- Equipment 的装备、开火、换弹与镭射摘要。
- GAS 的能力、弹药、伤害、死亡和比赛状态。
- 同一个 Gameplay 事务的开始、提交与结束语义。

#### 独立内容

- 第一人称使用 RAR Arms/Weapon 表现。
- 第三人称使用 Lyra Manny/Rifle 的移动、跳跃、蹲伏、Aim Offset、开火与换弹资产。
- 两个 AnimBP 各自组织适合自身骨骼和观看距离的 Pose。

#### 复制边界

Inventory 和精确武器实例是 OwnerOnly；远端不需要知道拥有者的全部私有库存。Equipment 复制一个轻量的公开装备/动作摘要，Simulated Proxy 据此创建第三人称 Mesh、播放 Montage 和显示镭射。客户端占有顺序可能晚于 Equipment RepNotify，因此 `PawnClientRestart` 进行幂等的本地表现重建，修复首次进入 Listen Client 时只剩第三人称表现的问题。

这条链路可以体现“同一逻辑，多套表现”和“私有状态与公开摘要分离”。

### 3.8 第三人称动画中的几个工程点

#### Locomotion 与速度域

Character 的实际速度是普通 `400 cm/s`、冲刺 `650 cm/s`、蹲伏 `230 cm/s`。AnimInstance 传入真实水平速度，不归一化。Blend Space 的 Walk 采样位于 400；600 和 900 两行复用 Jog 动画并使用不同播放倍率，使 650 的冲刺在速度域内插值得到接近真实移动速度的步频。

这说明 Blend Space 轴上限是采样域，不代表 Character 必须达到该值。面试时可用它解释“玩法速度、素材作者速度和动画播放倍率如何匹配”。

#### Aim Offset 与左手约束

站立/蹲伏 Aim Offset 负责大致表达俯仰；其后执行手部 IK，把左手重新约束到步枪基准。这样跳跃或大幅瞄准时，上半身姿势可以变化，左手仍不会脱离护木。

#### Turn in Place

角色 Actor 继续立即跟随 Controller Yaw，AnimInstance 在观察端本地维护 `RootYawOffset`，静止时抵消 Mesh 旋转。当积累角度超过阈值时进入 90 度转身资产，并通过动画中的 `RemainingTurnYaw`/权重曲线逐帧消费偏移；移动、腾空或反向输入时及时中断并用临界阻尼弹簧回零。

该值只影响视觉，不复制，也不改变胶囊、ControlRotation、弹道或镭射方向。

### 3.9 动作互斥与打断：先定义 Gameplay 规则，再选动画

项目中的动作关系不是靠 AnimBP 状态机偶然“顶掉”彼此，而是由 Gameplay 层显式规定事务优先级：

| 当前状态 | 新输入 | Gameplay 结果 | 表现结果 |
| --- | --- | --- | --- |
| 冲刺 | 开火 | 先解除冲刺，进入普通移动/静止，再允许开火 | Sprint Pose 退出，Fire Montage 播放 |
| 趴姿移动 | 开火 | 先把移动速度降到 0，保持趴姿静止后开火 | 不允许边爬边射 |
| 趴姿 | 跳跃 | 只尝试恢复站立，不执行起跳 | 顶部空间不足则继续趴姿 |
| 开火 | 换弹 | 依据弹药与当前事务结束/阻塞开火 | 进入配对 Reload Montage |
| 冲刺/换弹/检视 | 镭射 | 保留用户开关意图，动作期间暂时隐藏实际可见 Beam | 动作结束后按意图恢复 |
| 死亡 | 普通 Ability/Input | `State.Death` 统一阻止并清空输入 | 停止武器、镭射和 AI 行为 |

设计时要区分“用户意图”和“当前实际表现”。例如镭射有请求开启状态，也有因换弹临时隐藏的实际可见状态；否则动作结束后无法正确恢复，或会把临时门控误当作用户主动关闭。

### 3.10 AI、比赛与 UI：把战斗闭环做成可玩的 Demo

AI 没有复制一套假武器逻辑，而是复用玩家的 Equipment、Projectile Ability、弹药、换弹、Damage GE、死亡和重生链路。无目标时按权重选择地图争夺点，在 NavMesh 上移动并短暂停留/环视；局部范围内发现有视线的存活玩家后，按反应延迟、射程、短连发和停火间隔进入战斗，丢失目标后回到争夺点循环。

GameMode 是唯一比赛裁判，负责击杀归因、团队加分、15 分胜利和重生；GameState 只复制公共比分、阶段和胜方。达到 PostMatch 后，Ability 和 AI 都检查比赛阶段，停止生成新射击事务。HUD 读取本地 PlayerState/WeaponInstance 和复制 GameState：常驻显示血量、护盾等级/进化点、弹药与比分，实际造成伤害后才显示命中 X 和目标 AI 血条。

这部分的面试重点是权威来源与客户端反馈，而不是宣称 AI 很复杂。当前 AI 是基于 NavMesh 的轻量 Controller 状态逻辑，没有使用 Behavior Tree、EQS、AI Perception、掩体和难度系统。

## 4. 可以直接放进简历的项目描述

### 推荐标题

**Apecox 网络 FPS 垂直切片｜UE5 / C++ / GAS / Multiplayer｜个人项目**

### 推荐简介

基于 UE5 C++ 与 Gameplay Ability System 实现可玩的网络 FPS 原型，覆盖服务器权威实体弹丸、武器装备与弹药、伤害/护盾/死亡/重生、第一/第三人称双视角表现、轻量战斗 AI、Score Attack 规则及 HUD 反馈。

### 推荐核心 Bullet（四条简历版）

1. **构建 GAS 角色生命周期**：PlayerState 持有 ASC/AttributeSet，Character 作为 Avatar，通过 AbilitySet 管理能力授予与撤销，实现死亡、重生及护盾成长继承。
2. **实现服务器权威射击链路**：使用自定义 TargetData 提交射击意图，服务端校验装备、弹药与射速，生成实体弹丸并通过 GameplayEffect 结算伤害；客户端本地曳光只负责表现。
3. **设计数据驱动的双视角武器表现**：分离 Weapon Definition、运行时 Instance 与 Presentation 配置，共享 Gameplay 状态，分别驱动第一人称 Arms 和第三人称 Manny 的移动、瞄准、开火与换弹动画。
4. **完成 PvE Score Attack 闭环**：AI 复用玩家的武器、伤害、换弹和死亡系统，通过 NavMesh 在争夺点与战斗状态间切换；由 GameMode/GameState 管理计分、胜负及网络同步。

### 不建议写的表述

- “实现完整通用武器系统”：目前只有一把正式步枪。
- “实现成熟客户端预测”：项目有 GAS 输入/TargetData 流程，但没有完成高延迟下的完整弹药预测与回滚体系。
- “实现智能战术 AI”：当前是轻量争夺点/交战状态逻辑，不是 BT/EQS/感知/掩体系统。
- “完成商业级多人 FPS”：已完成 Standalone 与 Listen Server 回归，尚未做 Dedicated Server、丢包/高延迟和带宽专题。
- “完整 UMG/Lua UI 框架”：Lua 已取消，当前主要是 Canvas HUD，UMG 是后续方向。

## 5. 30 秒与 90 秒项目介绍

### 30 秒版本

我做了一个 UE5 C++ 和 GAS 的网络 FPS 垂直切片。玩家用第一人称操控，但其他客户端看到独立的第三人称 Manny 持枪表现。开火由客户端提交意图，服务器校验装备、弹药和射速后生成实体弹丸，再通过 GameplayEffect 结算护盾、生命、死亡和比分；弹丸曳光、枪口和命中反馈只消费权威结果。项目还实现了数据驱动武器配置、配对换弹动画、左手 IK、Turn in Place、战斗 AI 和 15 分 Score Attack。

### 90 秒版本

这个项目主要想证明我能把 UE Gameplay、GAS、网络和动画组织成一条完整链路。PlayerState 持有 ASC 和 AttributeSet，Pawn 重生时只更换 Avatar，所以护盾成长和玩家身份可以保留。武器拆成 Definition、Instance 和 Presentation：Definition 存射击/换弹与 AbilitySet，Instance 存弹匣和运行时连发状态，Presentation 单独配置第一/第三人称资源。

射击时客户端只提交瞄准意图与 Burst 序号，服务器重新验证视点、射速、弹药和装备来源，生成权威实体弹丸；命中后用 SetByCaller GameplayEffect 进入 AttributeSet，统一完成护盾优先、生命溢出、进化点和死亡。为了让高速弹丸可见，我把碰撞弹丸和曳光拆开：服务器接受射击后广播真实弹道参数，各端生成本地无碰撞曳光，所以视觉丢包不会影响玩法。

动画上我没有让 FP 和 TP 强行共享资产，而是共享 Gameplay 状态。第一人称用 Pose 分层、Montage、程序化后坐力和 FABRIK；第三人称用 Lyra Manny 的移动、Aim Offset、手部 IK 和 Turn in Place。开火、换弹、镭射等只同步事务摘要，远端选择自己的表现。最后把同一套装备、弹药、伤害和死亡逻辑复用给 AI，再由 GameMode/GameState 串成 Score Attack。

## 6. 高频追问与回答入口

### 为什么开火使用 GAS，但移动/蹲伏不全做成 Ability？

开火需要 Ability 的激活策略、SourceObject、网络 TargetData、阻塞标签与生命周期；基础移动和姿态已经由 CharacterMovement 提供预测、校正和复制。强行全部 Ability 化会重复 CMC 的职责并增加状态同步成本。项目按事务复杂度选择承载层，而不是为了“用了 GAS”把所有输入都塞进 GAS。

### 为什么 TargetData 不直接传 HitResult？

FPS 中客户端 HitResult 不可信。TargetData 只描述输入意图和连续射击序号，服务器重建最终方向并生成 Projectile，命中完全由 Authority 判定。这样减少作弊面，也确保伤害、弹药和比分来源一致。

### 为什么 Inventory 在 PlayerController，Equipment 在 Character？

库存是拥有者私有数据，适合 OwnerOnly 复制并跨 Pawn 管理；当前装备需要驱动 Character 上的 Mesh、Montage 和公开第三人称表现。二者分开后，远端只接收必要的装备摘要，不获得整个私有库存。

### GameplayTag 是否越多越好？

不是。Tag 适合稳定、可组合、跨系统的语义，如输入、死亡状态、GameplayCue 和 SetByCaller 参数。速度、权重、弹药和局部动画偏移使用强类型变量更清楚。Tag 不能替代所有状态建模。

### 第一/第三人称动画为何不共用同一套状态机和序列？

两个视角的骨骼、镜头距离和动作目标不同。共享的是速度、姿态、装备、换弹等 Gameplay 语义；各自 AnimBP 选择符合骨骼与观看需求的资产。这样可以快速使用 RAR 第一人称和 Lyra 第三人称配套资产，同时避免第一人称动作被第三人称可读性约束。

### Montage 和服务器事务时间不一致怎么办？

服务器配置决定换弹提交点与总时长，动画只是表现。不同视角可用播放倍率对齐同一事务窗口；Notify 可发出表现事件，但不能成为扣弹的唯一权威来源。

### 为什么左手 IK 必须放在后面？

Aim Offset、Montage 和程序化后坐力都会修改上肢或枪械姿势。左手约束在它们之后求值，才能以最终枪械位置为目标。换弹期间再用曲线降低 IK Alpha，让 Montage 暂时拥有左手控制权。

### GameMode、GameState、PlayerState 分别放什么？

GameMode 只存在于服务器，负责规则裁定；GameState 向所有端复制公共比赛状态；PlayerState 表达单个玩家的持久身份、比分和跨 Pawn 属性。Pawn 保存一次生命期内的移动和表现状态。

## 7. 与现有 Aura 项目的简历组合方式

当前简历只有 Aura，容易给面试官留下“跟随 GAS 教程完成 ARPG 链路”的印象。Apecox 应放在第一位，Aura 放第二位：

1. Apecox 展示独立工程判断、网络 FPS、动画系统、双视角和完整可玩闭环。
2. Aura 展示对 GAS 复杂伤害计算、Execution Calculation、自定义 EffectContext 和 WidgetController 的系统学习。
3. 两个项目不要重复写同样的 ASC 初始化、InputTag 和 Projectile 术语。Apecox 强调工程取舍和多人表现，Aura 强调 GAS 深度和复杂数值计算。

### 专业技能建议改写

现有技能栏术语较多，但证据关系不够明显。建议压缩通用描述，增加能被项目证明的能力：

- 熟悉 UE Gameplay Framework 与 GAS，能够围绕 PlayerState 持有 ASC、AbilitySet、GameplayTag、GameplayEffect、TargetData 和属性复制组织跨重生角色能力链路。
- 熟悉服务器权威 Gameplay 的基本边界，能够区分客户端输入意图、Authority 校验/结算、OwnerOnly 私有状态、公共复制摘要及本地表现通知。
- 掌握 UE 动画蓝图常用组织方式，能够使用 Blend Space、状态机、Montage、Aim Offset、Anim Curve、FABRIK/Two Bone IK 和程序化姿势完成第一/第三人称分层表现。
- 能够使用 DataAsset、`TInstancedStruct`、GameplayTag 与 Definition/Instance 分层组织可配置玩法，并用 C++ 自动化、冷加载和多人 PIE 回归验证关键链路。

“熟悉/掌握”的程度应以自己能否脱离文档讲清源码为准。简历中的每一条都要准备一个问题、一个设计选择、一个踩坑和一个验证证据。

## 8. 项目的诚实边界

| 已完成 | 当前边界 |
| --- | --- |
| 单步枪的装备、弹药、实体弹丸、伤害、换弹和双视角表现 | 尚未交付切枪、第二把武器、共享弹药类型和通用配件 |
| Standalone 与 Listen Server 双端回归 | 未做 Dedicated Server、高延迟、丢包和带宽专项 |
| 第一人称持枪全流程与第三人称主要行为 | 第三人称趴姿、检视同步未进入当前范围 |
| NavMesh 争夺点 + 局部交战 AI | 未使用 Behavior Tree、EQS、AI Perception、掩体和难度系统 |
| Canvas HUD 的战斗与比赛反馈 | UMG 正式界面尚未制作，Lua 方案已取消 |
| 单人/多人核心自动化与冷启动资产验证 | 动画观感、Socket、声音等仍需要人工 PIE 画面验收 |

主动说明边界不会削弱项目，反而证明你能区分原型、框架和商业化系统。

## 9. 面试复习的源码阅读顺序

### 第一遍：十分钟建立全景

1. `Agent/00_Coordination/Current_Code_Design.md`
2. `Source/Apecox/Public/Player/ApecoxPlayerState.h`
3. `Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h`
4. `Source/Apecox/Public/Weapons/ApecoxWeaponDefinition.h`
5. `Source/Apecox/Public/AbilitySystem/Abilities/Weapons/ApecoxRangedFireAbility.h`
6. `Source/Apecox/Public/AbilitySystem/TargetData/ApecoxRangedShotTargetData.h`
7. `Source/Apecox/Public/Weapons/ApecoxWeaponProjectile.h`

### 第二遍：按运行链路追代码

1. 输入：`ApecoxInputConfig` -> `ApecoxPlayerCharacter` -> `ApecoxAbilitySystemComponent`
2. 装备：`ApecoxInventoryComponent` -> `ApecoxEquipmentComponent` -> `ApecoxAbilitySet`
3. 开火：`ApecoxRangedFireAbility` -> `ApecoxProjectileFireAbility` -> `ApecoxWeaponProjectile`
4. 伤害：`ApecoxWeaponDamageEffect` -> `ApecoxVitalAttributeSet` -> `ApecoxHealthComponent`
5. 死亡比赛：`ApecoxDeathAbility` -> `ApecoxGameMode` -> `ApecoxGameState`
6. 表现：`ApecoxEquipmentComponent` -> `ApecoxWeaponPresentationActor` / `ApecoxProjectileTracer` / `ApecoxHUD`
7. 动画：`ApecoxCharacterAnimInstance` -> `ApecoxFirstPersonAnimInstance` -> FP/TP AnimBP
8. AI：`ApecoxWanderAIController` -> 同一 Equipment / Fire Ability / Damage / Death 链路

### 第三遍：每条简历 Bullet 准备四句话

1. 当时遇到的具体问题。
2. 最终链路和关键类型。
3. 为什么没有采用另一个看似直接的方案。
4. 如何通过自动化、冷启动或双端 PIE 证明结果。

## 10. 与现有八股笔记的关联

| Apecox 入口 | 应复习的笔记主题 | 可能追问 |
| --- | --- | --- |
| UObject、组件和 DataAsset 持有关系 | UE Reflection、UE GC、UE SmartPointer | `UPROPERTY` 为什么影响 GC；`TObjectPtr`、`TWeakObjectPtr` 的选择 |
| PlayerState/Character/Controller/GameMode/GameState | UE Gameplay | 各类在哪些端存在；重生时谁保留 |
| TargetData、RPC、RepNotify、FastArray | C++ 面经中的 UE 网络、UE Serialization | 状态同步与帧同步；可靠/不可靠 RPC；序列化成本 |
| AbilitySet 与委托 | GAS Learning、C++ 委托 | 如何避免重复授予；动态/原生委托差异 |
| Projectile 与碰撞 Sweep | UE Gameplay、数据结构/空间查询 | LineTrace、Sweep、碰撞通道；高速物体穿透 |
| AI NavMesh 与争夺点 | A*、NavMesh、EQS 基础 | 为什么 NavMesh 能走但策略仍不智能；EQS 可怎样扩展 |
| 动画分层与 IK | 当前动画系统面试讲解 | Local/Component/Bone Space；Additive；Montage Slot；IK 顺序 |
| 数据驱动武器 | OOP、组合、DataAsset | Definition/Instance 区别；为什么组合优于 Character 硬编码 |

## 11. 最值得反复练习的六个结论

1. **GAS 的价值不只是技能类，而是把能力生命周期、标签、网络 TargetData、效果与属性结算组织到同一套约束中。**
2. **数据驱动不是把所有字段丢进 DataAsset，而是区分不可变定义、运行时实例和可替换表现。**
3. **第一/第三人称应共享 Gameplay 语义，不必共享骨骼和逐帧 Pose。**
4. **服务器权威不等于所有对象都复制；Gameplay 真相权威，视觉可以在客户端按权威参数重建。**
5. **动画系统不能成为玩法真相；Montage、Notify、IK 和 VFX 应服从服务器事务及明确的动作所有权。**
6. **一项功能的完成标准包括源码链路、冷启动/自动化证据和人工画面验收，当前编辑器里“看起来好了”并不足够。**

## 12. 公开仓库说明建议

本项目使用的 RAR、Lyra 迁移资产、SciFi Props 等第三方内容不应直接提交到公开仓库。仓库应保留 C++、项目自建蓝图/配置、协作文档与资产依赖说明，并在 README 中说明本地恢复步骤。面试展示可以使用本机完整工程和录屏；GitHub 用于展示工程结构和核心源码。
