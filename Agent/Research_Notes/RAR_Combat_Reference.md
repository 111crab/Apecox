# RAR步枪射击实现调查

日期：2026-09-10。范围：原工程步枪发射、弹道、散布、后坐、伤害和命中反馈。

来源工程：`D:/UE_Resource/Realistic Assault Rifle Template/UE5/RAR/RAR.uproject`。通过UE 5.8 Python Commandlet加载原始资产并导出蓝图节点、连线、数据表、曲线及继承组件属性；没有保存资产，没有进行本轮RAR运行画面或联网实测。下文数值是已确认的配置，不等于所有演示关卡实例都没有覆盖。

## 结论

| 问题 | 原工程已确认的做法 |
| --- | --- |
| 有没有实体子弹 | 有。弹匣配置引用BP_RAR_AssaultRifle_PROJ_Bullet，继承BP_IG_Projectile；武器实际调用SpawnActor与On Launch。 |
| 是否纯Hitscan | 不是。发射前用射线找目标方向，实际命中由飞行子弹的碰撞触发。 |
| 是否有弹速、下坠 | 有初速度、ProjectileMovement和重力。继承组件GravityScale=1、Sweep Collision=true、Should Bounce=false。 |
| 随机散布 | UE的RandomUnitVectorInEllipticalConeInDegrees，Yaw/Pitch范围由武器配置乘动态散布倍率。 |
| 连发散布 | Shot Count除以弹匣容量，采样FC_WEP_SPREAD_Auto；再处理瞄准和移动因素。 |
| 后坐 | 按Shot Count采样预制向量曲线，分别产生枪械位置、枪械旋转、镜头旋转；弹簧平滑后应用。 |
| 命中反馈 | 碰撞→点伤害/冲量/表面效果；受伤端或靶子→射击者Hitmarker Widget和提示音。 |

## 1. 发射与弹道

主要链路：

```text
BP_IG_Character：开火输入、射速与动作判断
  → CGraph Fire Animation Play / On Try Fire Projectile
  → BP_IG_Weapon.Fire Projectile
  → Event All Fire Projectile及生成事件
  → CGraph Spawn Projectile
  → SpawnActor（弹匣配置的Projectile Type）
  → BP_IG_Projectile.On Launch
  → ProjectileMovement.SetVelocityInLocalSpace
  → Collision Box命中事件
```

`CGraph Spawn Projectile`从角色视点沿Facing Rotation做10000厘米射线。命中时用目标点减Fire Point Location计算朝向；没有命中时使用Facing Rotation的前向。对这个方向施加散布，然后在Fire Point Location生成子弹。射线的10000厘米是找准目标的距离，不是已证明的子弹最大飞行距离。

已读弹匣表`DT_RAR_WEP_AssaultRifle_Settings_Magazines`：

| 行 | 容量 | Projectile Velocity Range | 每发弹丸数 |
| --- | --- | --- | --- |
| Magazine_Default | 30 | 15000～20000 cm/s，即150～200 m/s | 1 |
| Magazine_Transparent | 30 | 25000～30000 cm/s，即250～300 m/s | 1 |

发射时通过Random Float In Vector Range选取速度，送入子弹局部X速度。两个弹匣都引用`BP_RAR_AssaultRifle_PROJ_Bullet`。

子弹继承的ProjectileMovement实读值：GravityScale=1、InitialSpeed=0、MaxSpeed=0、Sweep Collision=true、Should Bounce=false。InitialSpeed=0不表示它不飞：On Launch随后明确写入速度。子类碰撞盒半尺寸为1厘米，使用`SM_IG_Projectile_Bullet`可见模型，另有缩放Timeline。

所以RAR具备有限飞行时间和重力下坠的机制；本轮没有测量演示关卡中的落点，也没有证据把它称为包含空气阻力、穿透、跳弹的完整真实弹道模拟。该步枪子类Bounces Range=(0,0)，没有启用运动组件反弹。

## 2. 随机散布与连发曲线

入口：`BP_IG_Weapon → Graph Interfaces → CGraph Update Spread`。

自动射击分支可归纳为：

```text
t = Clamp(ShotCount / MagazineCapacity, 0, 1)
连发项 = FC_WEP_SPREAD_Auto(t) × (瞄准 ? SpreadAimingMultiplier : 1)
移动项 = 瞄准 ? 0 : MovementSpread
SpreadMultiplier = 连发项 + 移动项

Yaw范围 = SpreadYaw × SpreadMultiplier × 玩家/AI倍率
Pitch范围 = SpreadPitch × SpreadMultiplier × 玩家/AI倍率
实际方向 = RandomUnitVectorInEllipticalConeInDegrees(目标方向, Yaw范围, Pitch范围)
```

生成图中玩家/AI倍率分别为1和5；对应Is Player Controlled选择。这里不是按三个坐标独立随机一个命中点，也不是查询动画枪口抖动来推导散布。

`DT_RAR_AssaultRifle_Settings.Default`实际配置：Fire Rate=800 RPM、Spread Yaw=12、Spread Pitch=12、Spread Aiming Multiplier=0.3，Spread Curve引用`FC_WEP_SPREAD_Auto`。12是需要乘动态倍率的角度配置，不能理解成每一发固定偏12度。旧CDO中的500 RPM、2/2度和空曲线不是这张配置表的值。

曲线关键点：t=0时0、t=0.05时0、t=1时1，中间为三次曲线。起始连发段保留精度，后续扩大。采样参数是归一化连发计数，而不是经过的秒数。

角色每帧向武器传递Movement Spread。其来源是CMC当前加速度除最大加速度，取长度、向上取整，再乘0.3；更接近“是否在主动移动”的散布附加量，不能直接称为按实际移动速度线性变化。瞄准分支不叠加这个移动项。

半自动另走专门分支：基础项由瞄准选择0或1，再与上述移动项相加。它不等同自动模式的曲线采样。

Shot Count在开火流程递增；松键的自动模式路径、无法继续射击的路径会重置，半自动还有0.1秒RetriggerableDelay重置路径。原图没有采用本项目此前提议的独立“每发增加Bloom、停火后按速度衰减”状态。若Apecox选用平滑恢复，需要明确这是我们的设计调整。

随机方向节点不是RandomStream版本；所检查的步枪路径没有逐发Seed/ShotId驱动的确定性随机。Apecox若要兼容既有预测和权威重查，需要自行设计同一发的随机一致性，不能说这是直接照搬RAR。

## 3. 后坐：动画、枪械曲线与镜头曲线分开

已有开火动作仍参与表现；程序后坐并不只靠反复播放一段Montage。

枪械层位于`ABP_IG_Character.Update Recoil Values`：

- 取得当前站姿/瞄准的Recoil State Weapon。
- 将Shot Count转换成浮点，作为GetVectorValue的输入，分别采样位置和旋转曲线；乘配置倍率后形成Target Recoil Location/Rotation。
- 分别通过Custom Vector Spring Interp，得到Current Recoil Location/Rotation。
- 主AnimGraph的`AnimGraphNode_ModifyBone_4`以Additive方式修改`ik_hand_gun`的位置与旋转；输入通过Get Current Recoil Location/Rotation绑定。

实际曲线：

```text
VC_WEP_Recoil_Standing_Location
VC_WEP_Recoil_Standing_Rotation
VC_WEP_Recoil_Aiming_Location
VC_WEP_Recoil_Aiming_Rotation
VC_WEP_Recoil_Camera_Rotation
```

这些曲线横轴按发数使用，包含预制的横向/纵向变化；当前链路的后坐轨迹不是每枪重新抽一个随机Pitch/Yaw。部分曲线轴还配置了循环或往复外推。

镜头层位于`BP_IG_Character.Interpolate Camera Recoil`：同样按Shot Count采样相机旋转曲线、应用倍率和弹簧，另处理Return Recoil Pitch与Reset Recoil。Get Added Rotations组合镜头后坐和Lean；Tick把它加入角色缓存的Control Rotation，Facing Rotation会读取该缓存。因而它参与观察/发射朝向，不能只作为纯装饰晃枪理解。

数据表中非瞄准枪械位置/旋转倍率分别1.2/0.6，瞄准为1/0.15；非瞄准镜头旋转倍率1.3。不同通道还有独立Stiffness、Critical Damping Factor、Mass。迁移时应先确认骨骼坐标轴与叠加职责，不能把这些值直接加到Apecox现有Look Sway上。

### 3.1 Apecox后坐首轮验收后的复核

2026-09-10验收结果：镜头后坐规律、玩家主动压枪以及既有射击规则均通过；连续开火时左手掌会相对护木偏移；散布主观上仍然过大、随机感过强。2026-09-11只把左手FABRIK的旋转来源改为`Copy From Target`后，用户确认偏移修复且组合验证通过。

已经查实、下次无需重复调查的结论：

- RAR的散布方向仍是每发独立调用`RandomUnitVectorInEllipticalConeInDegrees`，没有读取上一发散布位置，也没有逐发后坐模板决定命中点。玩家看到的连续规律主要来自枪械和镜头后坐曲线不断移动散布锥的中心；随机散布在该中心周围独立取样。
- Apecox以30发弹匣归一化连发序号。原配置下静止腰射长连发可达到12度半角，移动腰射最大15.6度；2026-09-11将基础Yaw/Pitch半角降至3度并保留移动附加0.3后，用户确认体验验证通过。
- Apecox为同一发的预测与权威重查增加确定性`ShotId`种子。这是Apecox自己的联网一致性职责，不是RAR原实现。
- RAR将曲线向量映射为旋转时使用`X→Roll、Y→Pitch、Z→Yaw`；枪械后坐通过UE的`VectorSpringInterp`恢复，`Target Velocity Amount=0`，不启用Clamp，也不从Target初始化。
- RAR对`ik_hand_gun`使用Additive `Modify Bone`。该节点未显式覆盖坐标空间时，UE构造函数默认位置与旋转均为Component Space。
- Apecox的`SK_IG_FP_Mannequin`和`SK_IG_FP_Arms`骨架中，`ik_hand_gun`位于`ik_hand_root`下；`ik_hand_l`、`ik_hand_r`以及`SOCKET_Weapon`都直接隶属于`ik_hand_gun`。因此“左手目标完全没有随枪械骨移动”已被骨架层级证据排除。
- RAR左手FABRIK的准确设置为：`Tip Bone=hand_l`、`Root Bone=clavicle_l`、`Effector Target=IK_hand_l`、`Effector Transform Space=Bone Space`、`Effector Rotation Source=Copy From Target`、`Precision=0.01`。Apecox验收前基线为：`Tip Bone=hand_l`、`Root Bone=upperarm_l`、`Effector Target=ik_hand_l`、`Effector Transform Space=Bone Space`、`Effector Rotation Source=Keep Local Space Rotation`、`Precision=0.1`。
- Apecox最终左手FABRIK位于外层`ABP_Apecox_FirstPersonArms`。`ABP_Apecox_Rifle_FP_Arms`负责步枪姿势与移动分支，不含该最终FABRIK节点。

UE 5.8的`FAnimNode_Fabrik::EvaluateSkeletalControl_AnyThread`确认两种旋转来源的准确行为：`Keep Local Space Rotation`用末端骨原局部旋转乘求解后的父骨Component Transform重建手掌方向；`Copy From Target`直接把末端骨Component Space旋转设置为Effector目标旋转。2026-09-11单变量PIE验证通过，因此本项目连续后坐时的左手偏移根因已经确认是掌面没有同步枪械目标旋转；`Root Bone=upperarm_l`与`Precision=0.1`无需改动。

## 3.2 ADS机械瞄具

2026-09-11对RAR瞄准资产与`DT_RAR_AssaultRifle_Settings_Ironsights.Default`只读复核：默认相机FOV倍率为`0.75`，Yaw/Pitch灵敏度倍率均为`0.5`，Aimed Viewmodel FOV为`100`。其瞄准动作不是把一条Aimed Loop直接当完整姿势播放，而是以下结构：

```text
A_RAR_FP_PCH_AssaultRifle_Aim_Pose（完整非Additive基础姿势）
  + A_RAR_FP_PCH_AssaultRifle_Idle_Loop_Aimed（Local Space Additive）
  或 A_RAR_FP_PCH_AssaultRifle_Walk_F_Loop_Aimed（Local Space Additive）
```

RAR还提供Mesh Space Additive的`A_RAR_FP_PCH_AssaultRifle_Transition_Aim_In`和`Aim_Out`，长度约`0.7s/1.3s`。Apecox首轮不直接叠加这两条长过渡，先以`0.20s` Pose Blend建立可调基线，避免重现Sprint In/Out曾出现的额外摆枪与拖沓。ADS分支插在Apecox步枪地面/空中基础选择之后、现有Turning与Jump/Lean之前，因此下游Fire Slot、程序后坐和左手FABRIK不被绕开；ADS时普通Turning Additive归零，机械瞄具仍保留缩小后的Look Sway。

## 4. 命中、伤害与提示

子弹碰撞：`BP_IG_Projectile`的Collision Box命中事件保存Result，进入CGraph On Hit / On Explode。On Explode是通用命名，不代表步枪子弹一定造成范围爆炸。

步枪继承的`BPSC_IG_Explosion`组件实读Damage Method=Point、Damage Amount=(10,20)；Get Damage使用RandomFloatInRange，Apply Point Damage最终调用UE的ApplyPointDamage。子类Radius=(0,0)，另配置物理冲量范围10500～24500；命中可推动物理物体。伤害、物理冲量和表面视听效果是不同调用。

表面效果链：

```text
Projectile.Result + Impact Surfaces Handle
  → BPFL_IG_Surfaces.Spawn Surface Details
  → 读取DT_Surfaces的Bullet-Impact行
  → GetSurfaceType选Details
  → 生成粒子、弹孔贴花、碰撞音效
```

该行当前只有SurfaceType_Default配置，引用：

- `NS_IG_Impact_Bullet`；框架同时有Cascade与Niagara分支。
- `MI_IG_Decal_Impact_Wood_01/02/03`三个贴花备选，随机旋转与3～7尺寸；延迟5～9秒后用3秒淡出。
- `SC_IG_Impact_Bullet_Random`；声音先于表面细分检查生成，并将表面Type作为参数传入音频组件。

因此应区分“框架支持按物理表面配置”与“当前步枪已完整配置多种表面”。本轮只确认默认行，未逐项听测Sound Cue内部的材质差异。

受伤与命中标记：`BPAC_IG_Health.Receive Damage Point`处理扣血并向Damage Causer调用Spawn Hitmarker；`BP_IG_Target`也有独立受伤→Spawn Hitmarker入口。角色通过Event Client Spawn Hitmarker创建`WBP_IG_Hitmarker`。Widget播放Up动画与2D提示音，带小幅随机位置/角度，延迟后RemoveFromParent。

所以命中墙面特效不等于命中敌人提示。Hitmarker需要受伤端或靶子确认，不能每次开火或射线碰到任意墙壁都显示。

Health组件还有Event On Kill Point、On Death及Ragdoll处理。此次未找到独立于普通Hitmarker的击杀确认UI链，不能把目标死亡逻辑等同于已经实现击杀图标/击杀音效。

### 4.1 Scope_01、镭射与枪口硝烟补充核对

2026-09-14继续只读核对 RAR 资产和数据表：

- 固定瞄具资产为`SM_RAR_ATT_AssaultRifle_Scope_01`，材质为`MI_RAR_ATT_AssaultRifle_Scope_01`，步枪骨架提供`SOCKET_Scope`。
- `DT_RAR_AssaultRifle_Settings_Scopes.Scope_01`中`Render Target Required=False`、相机FOV倍率`0.7`、Yaw/Pitch灵敏度倍率`0.5`、Aimed Viewmodel FOV`100`。因此该瞄具不需要实时渲染目标；Apecox可以在固定步枪Prefab中装配，不需要先建设通用配件系统。
- 镭射挂件资产为`SM_RAR_ATT_AssaultRifle_Laser`。RAR 的表现由`BP_IG_WEP_Laser_Beam`、`SM_IG_Laser_Beam`和`BP_IG_WEP_Laser_Dot`共同组成，不是只在枪上放一个红色发光 Mesh。
- `DT_RAR_AssaultRifle_Settings_Lasers.Lasersight`配置为`Off While Aiming=False`、`Off While Running=True`、`Off While Lowered=True`，并使用`SC_IG_WEP_ToggleLaser`。Apecox接入时应保留“ADS允许、冲刺关闭”的动作关系。
- RAR 默认枪口系统是`NS_IG_MuzzleFlash`，内部同时包含`NE_IG_MuzzleFlash_Flame`和`NE_IG_MuzzleFlash_Smoke`。Apecox已经迁入并在`DA_WeaponPresentation_Rifle.MuzzleFlashSystem`使用该系统，因此硝烟属于现有一次性枪口效果，先做可见性验收，不重复生成另一套烟。
- RAR `BP_IG_Weapon`的`Try Spawn Fire Particles`实际调用`SpawnSystemAttached`：Attach Point为`SOCKET_Emitter`、Location为零、Rotation为`Pitch 0/Yaw 90/Roll 0`、SnapToTarget、AutoDestroy和AutoActivate为True。Apecox首轮传零旋转会让该系统按错误局部轴发射；应由Presentation数据传入相同Yaw 90，而不是旋转枪口锚点或复制第二套烟。
- Apecox已有实体子弹命中后执行`GameplayCue.Weapon.Impact`并携带完整`HitResult`，但此前没有对应Notify资产，因而不会显示RAR的粒子、声音或弹孔。本轮以Apecox原生GameplayCue父类消费该入口，继续保持伤害与表现分离。
- 2026-09-14首轮人工验收时，`GCN_Weapon_Impact.uasset`二进制没有可搜索的完整`GameplayCue.Weapon.Impact`字符串，而已工作的`GCN_Weapon_Fire`明确保存`GameplayCue.Weapon.Fire`。原因是专用原生父类CDO预填标签后，派生Blueprint没有把相同默认值序列化成自身Asset Registry字段。修复合同是原生表现基类保留None、派生GCN明确选择并保存Impact标签，同时在DefaultGame.ini限定GameplayCue扫描目录。
- 当日复验确认Impact粒子、弹孔和命中声均已恢复；该定位成立。`NS_IG_MuzzleFlash`补齐RAR的Yaw 90度后仍看不到硝烟，说明剩余问题不再是系统引用或生成方向。后续应直接检查Smoke Emitter在UE 5.8中的运行状态、Spawn/Lifetime、Bounds和Scalability。
- 2026-09-15只读检查当前`NS_IG_MuzzleFlash`，确认`Flame`、`Smoke_Main`、`Smoke_Sides`三个Emitter均存在并启用；`Smoke_Main`在Time 0以Count 1生成Burst，Sprite Renderer引用`MI_IG_MuzzleFlash_Smoke_Main`。三个枪口材质实例和贴图的导出设置与RAR副本一致，没有发现缺失Emitter或空材质引用。
- 临时取消 Niagara 的`FirstPerson`图元类型后，硝烟仍不明显且原有枪口火光也消失，证明Apecox的独立第一人称FOV/深度路径要求枪械和枪口Niagara使用相同图元类型。该试验已撤销，恢复此前稳定路径。用户决定不再把硝烟清晰可见作为近期完成门槛，后续如需强化，应新做可控的Apecox烟雾表现，而不是继续改变已工作的枪口火光引用链。
- 2026-09-17用户重新要求火光与烟雾同时清晰可见。进一步导出`M_IG_VFX_MuzzleFlash`确认`Density Smoke`直接乘以Flipbook采样，并共同进入Opacity与Emissive；Apecox与RAR两个烟雾材质实例此前均为1.0。本轮保持同一个`NS_IG_MuzzleFlash`、Yaw 90、FirstPerson图元类型和创建后激活路径，只把`MI_IG_MuzzleFlash_Smoke_Main`与`MI_IG_MuzzleFlash_Smoke_Side`的该参数提高到2.5，避免再次破坏已经工作的Flame分支。腰射与ADS的火光、烟雾均已由用户验证通过，2.5成为Apecox当前基线。

2026-09-15继续以Python Commandlet只读导出RAR 5.8镭射资产，补齐运行参数与空间关系：

- RAR枪械Skeletal Mesh不含`SOCKET_Laser`。`Event Update Forestock Attachments`先把武器的`Socket Laser`场景锚点以SnapToTarget挂到当前Forestock Mesh的`SOCKET_Laser`，镭射挂件再使用该锚点。挂件`SM_RAR_ATT_AssaultRifle_Laser`自身另有一个`SOCKET_Laser`，相对位置`(-1.238064, 3.922933, 1.767690)`、Yaw 90度，作为光束和光点的实际发射锚点。此前把第一个挂点误记为枪械骨架Socket，已由2026-09-17 Apecox实际资产检查纠正。
- 光束使用`SM_IG_Laser_Beam`，其局部X轴长度约`20.024246`厘米。RAR每帧沿前向Trace 10000厘米，命中后把X缩放设为`HitDistance / 20`，Y/Z使用Thickness；当前步枪数据把Thickness设为5。
- 光点使用`MI_IG_Laser_Dot`。只有Blocking Hit时显示，世界位置取`Hit.Location`，以`MakeRotFromX(Hit.Normal)`贴合法线；均匀缩放为`5 + HitDistance * 0.002`。RAR还附带红色Point Light，Intensity 1.5、Attenuation Radius 75、关闭阴影。
- `DT_RAR_AssaultRifle_Settings_Lasers.Lasersight`确认`Off While Aiming=False`、`Off While Running=True`、`Off While Lowered=True`。RAR分开保存“用户想开启”和“当前是否可见”，所以冲刺只暂时关闭，结束后自动恢复，不再次播放`SC_IG_WEP_ToggleLaser`。
- Apecox按上述可观察规则原生复刻，没有迁入`BP_IG_WEP_Laser*`供应商Gameplay蓝图。只读报告位于`Saved/Diagnostics/M1D/Laser/rar_laser_report.json`，脚本为同目录`inspect_rar_laser.py`；执行过程中没有保存RAR资产。
- RAR的Beam Trace始终沿侧挂发射器前向，因此瞄具中心与表面镭射点存在真实视差。Apecox根据本项目的一倍镜可读性需求做有意调整：腰射保留RAR的实体方向；ADS按`AimAlpha`平滑指向相机中心Visibility目标，使完全开镜后两个红点视觉合并，但不改变真实射击TargetData、散布或伤害。

## 5. 对Apecox第3天计划的修正

本轮先调查，未实施新的业务代码或改变现有Hitscan。

- 将散布基础机制改为以RAR的椭圆锥、连发曲线、瞄准/移动修正作为参考；确定性种子是Apecox既有射击事务需要补的职责。
- 后坐按“开火动作＋枪械曲线＋镜头曲线＋弹簧恢复”设计，分别验证，避免与Look Sway重复驱动同一效果。
- 命中反馈拆为物体命中视听、目标受伤提示、目标死亡确认；先核对已有Impact Cue与确认事件，再接具体表现。
- **实体Projectile还是保留Hitscan是尚未定案的玩法选择。** 保留Hitscan可以参考RAR的散布、后坐和反馈，但不会获得其飞行时间/下坠。若要复现那部分，需要单独设计Projectile射击模型，不能给Hitscan加一道曳光效果后宣称已复制弹道。
- 本轮没有证据要求直接迁入RAR的整套复制、伤害或蓝图框架；继续尊重Apecox自己的GAS、Equipment与射击事务边界。

## 证据索引

本机只读导出位于`Saved/Diagnostics/Day3/rar_combat/`；报告`report.json`、`components.json`，命令日志`Saved/Diagnostics/Day3/inspect_final.log`。检查脚本为同目录上级`inspect_rar_combat.py`，只调用加载、属性读取、文本导出，没有Save调用。

| 结论 | 复核资产/函数 |
| --- | --- |
| 实体子弹及随机方向 | BP_IG_Weapon：Graph Replication / CGraph Spawn Projectile，SpawnActorFromClass_0、CallFunction_52、CallFunction_34 |
| 默认弹速与数量 | DT_RAR_WEP_AssaultRifle_Settings_Magazines.json |
| 重力、扫掠、点伤害 | components.json中的ProjectileMovement模板与步枪Explosion模板属性 |
| 散布公式 | BP_IG_Weapon：CGraph Update Spread；FC_WEP_SPREAD_Auto；DT_RAR_AssaultRifle_Settings.Default |
| 移动散布与计数重置 | BP_IG_Character：CGraph Weapon Update、CGraph Input Action Fire、CGraph Fire Ticking |
| 枪械后坐 | ABP_IG_Character：Update Recoil Values、AnimGraphNode_ModifyBone_4及PropertyBindings |
| 镜头后坐 | BP_IG_Character：Interpolate Camera Recoil、Get Added Rotations、CGraph Event Tick |
| Apecox IK骨架父子关系 | `Saved/Diagnostics/Day3/Recoil/ik_hierarchy.json` |
| RAR左右手FABRIK参数 | ABP_IG_Character：Retarget Hands To IK Bones中的左右FABRIK节点 |
| Modify Bone默认坐标空间 | `E:/UE_5.8/Engine/Source/Runtime/AnimGraphRuntime/Private/BoneControllers/AnimNode_ModifyBone.cpp`构造函数 |
| 表面反馈 | BPFL_IG_Surfaces、DT_Surfaces.Bullet-Impact |
| 伤害与命中标记 | BPSC_IG_Explosion.Apply Point Damage、BPAC_IG_Health.Receive Damage Point、BP_IG_Target、WBP_IG_Hitmarker |

角色与AnimBP的大型图复用此前原工程只读导出`Saved/Diagnostics/Day2/rar/`。所有本机诊断文件默认不纳入Git；本篇保存可复核的原始资产名、图名、配置和结论边界。
