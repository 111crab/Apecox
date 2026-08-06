# 第三方资产复盘：技能原型验证候选池

更新时间：2026-07-13

## 范围和方法

本次只读取 `Content/ThreePartAsset` 的目录、文件名和数量，没有移动、重命名或修改任何 `.uasset`。因此本文是第一轮“候选池判断”，不是精确资产注册表报告。后续如果要确认每个资源的真实 UE 类型、父类、Skeleton、是否 Niagara/Cascade、是否可直接软引用，需要通过 UE 编辑器或 Asset Registry 再查一轮。

磁盘路径：`D:\UnrealProject\Aura\Content\ThreePartAsset`

UE 内容路径前缀：`/Game/ThreePartAsset`

## 总览

| 资产包 | 文件量 | 初步定位 | 对技能系统的价值 |
| --- | ---: | --- | --- |
| `ParagonSparrow` | 1723 | 弓箭、远程普攻、蓄力、箭雨 | 远程投射物、蓄力射击、范围箭雨、Buff 表现 |
| `ParagonYin` | 1655 | 近战连击、鞭子、拉扯、风场 | 近战连击、命中窗口、牵引/反弹/风墙类技能 |
| `ParagonPhase` | 1434 | 法术、光束、连线、投射物 | 法师/支援型标准验证角色，适合火球、光束、连线、目标技能 |
| `ParagonSevarog` | 1071 | 重武器、灵魂吸取、重击、压制 | 生命汲取、持续引导、重击、范围震荡、被动层数表现 |
| `SlashTrail_SoftTofu` | 415 | 斩击拖尾、元素斩波、命中特效 | 近战拖尾、飞行斩波、武器附着特效，且有 Niagara |
| `FXVarietyPack` | 232 | 火球、治疗、魔法阵等旧粒子蓝图 | 快速临时表现，长期需确认是否是 Cascade |
| `Free_Spells` | 198 | 治疗、Buff、Aura、元素光环、爆炸 | 治疗术、Buff、光环、领域类技能的优先候选 |
| `Free_Magic` | 192 | 投射物、命中、范围、法阵、斩击 | 火球术、范围法术、通用魔法表现的优先候选 |
| `Maps` | 1 | 地图入口 | 暂不作为技能系统重点 |

## 第一结论

1. `Free_Magic` 和 `Free_Spells` 是当前最适合直接服务技能原型的 VFX 包，因为它们有明确的 `VFX_Niagara/NS_*` 命名，和我们长期想走 Niagara 参数化、软引用配置的方向一致。
2. 四个 Paragon 包更像“完整角色技能样板库”：动作、音效、特效、命中资源比较完整，但大量特效文件命名为 `P_*`，大概率是 Cascade 粒子。它们依然有价值，但要先确认是否要临时兼容 `UParticleSystem`，还是只把它们当美术参考。
3. 如果想立刻提升技能验证质量，最稳路线不是一次性替换所有角色，而是先挑一个“标准验证角色”或“标准预览角色”，再只迁移 2-4 个关键蒙太奇。
4. 下载资产不要直接混入我们技能系统配置目录。建议保留原包在 `/Game/ThreePartAsset`，后续把精选后的 DA、Montage、Cue、SkillDefinition 放到 Aura 自己的目录中，并通过软引用指向第三方资源。

## 已确认决策

法术类技能的标准验证角色采用 `ParagonPhase`。原因是 Phase 的资源语义更贴近法术、投射物、光束、连线和支援技能，适合验证我们当前优先推进的 `ProjectileCast`、`ChannelCast`，以及后续的 Beam/Drain、Link、AreaField 等技能模板。

这个决策不等于以后所有玩家角色都必须使用 Phase。Phase 的职责是“标准法术验证角色”和未来技能编辑器预览角色候选：先用它把技能链路、蒙太奇事件、发射点、VFX、命中、多人显示验证稳定，再决定是否把同一套技能配置迁移到 Aura 当前主角或其他角色。

## 各资产包候选用途

### Free_Magic

重要候选：

- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Projectile2`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Area1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Area2`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Circle1`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Attack_Line`
- `/Game/ThreePartAsset/Free_Magic/VFX_Niagara/NS_Free_Magic_Slash`

建议用途：

- 火球术：投射物飞行、命中爆点、施法手部光效。
- 范围法术：地面法阵、持续区域、结束爆发。
- 早期技能表现：优先用这些 Niagara 系统，不急着引入复杂角色特效。

注意：

- 该包也带 Manny/Quinn、`IK_Mannequin`、`RTG_Mannequin` 等内容，适合做 UE5 标准人形骨架的重定向参考。

### Free_Spells

重要候选：

- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Healing`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Healing`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Mana`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Buff_Mana`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Aura_Fire`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Circle`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Explosion`
- `/Game/ThreePartAsset/Free_Spells/VFX_Niagara/NS_Free_Spells_Shockwave`

建议用途：

- 生命涌动：引导时用 `Aura_Healing` 或 `Buff_Healing`，完整释放时用更强的瞬时治疗表现，提前松开时用较弱的持续回复表现。
- Buff/状态验证：用于后续验证“效果模板 Effect + 状态 State + 实例 Buff”的分层。
- 魔法资源表现：`Mana` 类效果可用于耗蓝、回蓝、法力护盾等技能。

### FXVarietyPack

重要候选：

- `BP_ky_fireBall`
- `BP_ky_healAura`
- `BP_ky_magicCircle1`
- `BP_ky_fireStorm`
- `P_ky_fireBall`
- `P_ky_healAura`
- `P_ky_magicCircle1`

建议用途：

- 可以作为火球、治疗、魔法阵的临时备选。
- 如果这些是 Cascade 粒子，V1 可临时支持，但长期技能编辑器最好优先围绕 Niagara 做参数化预览。

### ParagonPhase

重要动作候选：

- `Cast`
- `Ability_Q`
- `Ability_E`
- `Ability_R_Alt`
- `Ability_R_Alt_Loop`
- `RMB_Intro`
- `RMB_Loop`
- `RMB_Pull`
- `RMB_Throw`
- `Primary_Attack_*_Montage`

重要特效候选：

- `P_PhaseBeam`
- `P_BeamImpactDamage`
- `P_BeamImpactPhase`
- `P_PhaseLinkCast`
- `P_PhaseLinkImpact`
- `P_PhaseLink_PreCast_Loop`
- `P_PhasePrimaryCastL`
- `P_PhasePrimaryCastR`
- `P_PhasePrimaryImpact`
- `P_PhaseProjectileRibbons`

建议用途：

- 最适合作为“法师/支援/远程技能验证角色”候选。
- 火球术可以借用 `Cast` 或 `RMB_Throw` 的动作语义。
- 后续很适合做“连线、光束、牵引、支援”类技能原型。

### ParagonSparrow

重要动作候选：

- `Primary_Fire_Fast`
- `Primary_Fire_Med_Montage`
- `Primary_Fire_Slow`
- `Q_Ability_Montage`
- `Q_Ability_Targeting`
- `RMB_Drawback`
- `RMB_Loop`
- `RMB_Fire`
- `R_Ability_*_Fire`

重要特效候选：

- `P_SparrowPrimaryMuzzleFlash`
- `P_Sparrow_PrimaryAttack`
- `P_Sparrow_HitHero`
- `P_Sparrow_Primary_Ballistic_HitPlayer`
- `P_RainArrowsTarget`
- `P_RainofArrows`
- `P_RainofArrowsProj`
- `P_Sparrow_UltArrow`
- `P_Sparrow_UltHit`

建议用途：

- 后续做“蓄力投射物”“普通远程连射”“范围箭雨”时优先考虑。
- 不建议作为第一批通用法术验证角色，因为弓箭动作会把机制表达带偏，但它非常适合验证投射物和多段范围落点。

### ParagonYin

重要动作候选：

- `Primary_Attack_*_Montage`
- `E_Ability_Attack_A_Montage`
- `E_Ability_Attack_B_Montage`
- `E_Ability_Intro`
- `Q_Intro`
- `Q_Pull_Kick`

重要特效候选：

- `p_Yin_Primary_Attack`
- `P_Yin_Primary_Impact`
- `p_Yin_Primary_WispTrail`
- `P_Lash_Tether`
- `P_Lash_Tether_End`
- `p_WhipCrack_Afterimages`
- `p_WhipCrack_Reflect`
- `p_WhipCrack_WindWall`
- `p_Ult_Windzone`

建议用途：

- 后续做近战连击、命中窗口、武器拖尾、牵引/拉扯类机制时优先考虑。
- 这类技能更依赖 AnimNotifyState 命中窗口，不适合作为“投射物模板”的第一验证对象。

### ParagonSevarog

重要动作候选：

- `Soul_Siphon`
- `Soul_Siphon_Targeting`
- `Soul_Siphon_Targeting_Loop`
- `Subjugation`
- `Swing1_Medium_Montage`
- `Swing2_Medium_Montage`
- `Ultimate_Targeting`
- `Ultimate_Targeting_Loop`

重要特效候选：

- `P_SiphonCasting`
- `P_SiphonImpact`
- `P_SiphonTargeting`
- `P_SoulCastTrails`
- `P_SoulSwirls`
- `P_SoulSwirlsBody`
- `P_Sevarog_Melee_SucessfulImpact`
- `P_Sub_Cast`
- `P_Sevarog_Subjugate_Blast`
- `P_UltimateLooping`

建议用途：

- 很适合做“持续引导 + 目标锁定 + 吸血/汲取”的技能验证。
- 也适合做重武器近战、范围震荡和被动层数表现。

### SlashTrail_SoftTofu

重要候选：

- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Niagara/Fire/NS_AuraFX_Fire`
- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Niagara/Fire/NS_Hit_Fire`
- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Niagara/Basic/NS_SlashTrail_Basic`
- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Niagara/Basic/NS_SlashTrail_Basic_Loop`
- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Niagara/Basic/NS_Hit_Basic`
- `/Game/ThreePartAsset/SlashTrail_SoftTofu/Blueprint/fire/BP_Slash_Projectile_Start_Fire`

建议用途：

- 近战武器拖尾。
- 飞行斩波类投射物。
- 元素斩击命中特效。

注意：

- 这个包包含很多 Blueprint 演示资产。长期最好把“表现资源”抽成我们自己的 GameplayCue/Presentation 配置，不直接依赖演示蓝图的游戏逻辑。

## Retarget 是什么，能解决什么

Retarget 的目的，是把“源骨架上的动画”转换到“目标骨架上的角色”上。比如 ParagonPhase 的 `Cast` 动作原本绑定在 Phase 自己的 Skeleton 上，当前 Aura 角色如果不是同一个 Skeleton，就不能直接播放。UE5 通常通过 IK Rig 和 IK Retargeter 建立“源骨架 -> 目标骨架”的映射，再批量生成目标骨架可播放的 AnimSequence。

它能做：

- 把别的角色的普通动作序列转到当前角色身上。
- 让我们少做很多手工动画。
- 帮我们把第三方包里的 Cast、Attack、Channel 动作变成 Aura 技能可用素材。

它不能完全自动保证：

- 手部武器位置一定对。
- 脚底不滑。
- Root Motion 位移一定符合网络同步需求。
- Montage 里的 Notify/Event 帧一定仍然准确。
- 武器 Socket、发射 Socket、特效附着点一定存在。

所以长期最佳实践是：优先 Retarget AnimSequence，然后在目标 Skeleton 上重新创建 Montage，并重新检查 GameplayEvent Notify 的关键帧。

## 推荐重定向流程

1. 先确定标准验证角色。
   - 最保守：继续用当前 Aura 角色，只把少量施法/攻击动作 retarget 过来。
   - 更适合技能预览：选 Phase 作为法术测试角色，后续再让 Aura 角色接入同一套技能配置。
2. 打开源角色 Skeleton，确认它是否已经有 IK Rig。
3. 打开目标角色 Skeleton，创建或复用 IK Rig。
4. 创建 IK Retargeter，配置 Root、Spine、Head、Arm、Leg 等链。
5. 在 Retargeter 里预览动作，调整 Retarget Pose，先保证站姿、手臂和脚底合理。
6. 批量 retarget 少量候选 AnimSequence，不要一开始全量迁移。
7. 在目标 Skeleton 上创建 Montage。
8. 在 Montage 关键帧添加 `GameplayEvent` Notify，例如释放火球、开始命中窗口、结束引导。
9. 用技能 DA 软引用这个 Montage 和 VFX，进入 PIE 验证单人、多端显示和服务器权威效果。

## 当前技能原型的资产建议

### 火球术 / ProjectileCast

优先候选：

- 动作：Phase `Cast` 或 `RMB_Throw`；短期也可继续用已有 Aura 蒙太奇。
- 飞行：`NS_Free_Magic_Projectile1` 或 `NS_Free_Magic_Projectile2`
- 命中：`NS_Free_Magic_Hit1` / `NS_Free_Magic_Hit2` / `NS_Free_Spells_Explosion`
- 发射点：优先使用手部 socket；如果当前角色 socket 不统一，先在技能配置里暴露 `SpawnSocketName`。

验证重点：

- Montage Notify 是否在发射帧触发。
- 投射物是否只由服务器生成，表现是否在其他端正确显示。
- 命中 Cue 是否在目标和旁观客户端可见。

### 生命涌动 / ChannelCast

优先候选：

- 引导循环：`NS_Free_Spells_Aura_Healing` 或 `NS_Free_Spells_Buff_Healing`
- 完整释放：`NS_Free_Spells_LevelUp` 或更强的 Healing Aura 变体
- 提前释放：较弱的 Healing Buff 或持续恢复表现
- 动作：先用简单站立/占位动作；后续可考虑 Phase/Sevarog 的持续引导动作。

验证重点：

- 按住、松开、移动取消三个结果是否分清。
- Full / Partial / Cancelled 结果不应靠临时字符串判断，应由模板内部 outcome 枚举或稳定标签承接。
- Full 是瞬时治疗；Partial 是较低数值且持续回复。

### 范围领域 / AreaField

优先候选：

- 地面区域：`NS_Free_Magic_Area1` / `NS_Free_Magic_Area2`
- 法阵：`NS_Free_Magic_Circle1` / `NS_Free_Spells_Circle`
- 结束爆发：`NS_Free_Spells_Shockwave` / `NS_Free_Spells_Explosion`

验证重点：

- 技能目标是“生成位置”；衍生物目标是“领域内被检测到的目标”，二者要分开配置。
- 领域生命周期、周期检测、周期应用 GE 是后续 CombatEntity 的重点。

### 近战连击 / MeleeCombo

优先候选：

- 动作：Yin `Primary_Attack_*_Montage`，或 Sevarog `Swing*_Medium_Montage`
- 拖尾：`SlashTrail_SoftTofu/Niagara/.../NS_SlashTrail_*`
- 命中：`NS_Hit_*` 或 Yin/Sevarog 自带 impact 粒子

验证重点：

- 命中窗口用 AnimNotifyState 比单点 Notify 更合适。
- 近战检测应是模板/Task/Fragment 组合，不要写死在某个具体技能里。

### 光束/汲取 / BeamOrDrain

优先候选：

- Phase：`P_PhaseBeam`、`P_PhaseLinkCast`、`P_PhaseLinkImpact`
- Sevarog：`P_SiphonCasting`、`P_SiphonImpact`、`P_SiphonTargeting`

验证重点：

- 目标锁定和持续检测要与技能释放目标区分开。
- 这类技能适合验证“持续任务 + 周期 GE + 持续表现 + 中断清理”。

## 资产管理建议

1. 第三方原始包保留在 `/Game/ThreePartAsset`，不要为了短期验证直接重命名或移动。
2. 我们自己的技能配置、运行时资产和文档放在 Aura 体系目录，例如后续可以建立：
   - `/Game/Aura/Skills`
   - `/Game/Aura/CombatEntities`
   - `/Game/Aura/GameplayCues`
   - `/Game/Aura/SkillPrototype`
3. `UAuraSkillDefinition`、CombatEntity、Presentation/Cue 配置里使用软引用指向第三方资源。
4. 代码里不要硬编码 `/Game/ThreePartAsset/...` 路径。
5. 每次设计具体技能时，先写“资产需求清单”：动作、发射点、飞行表现、命中表现、持续表现、结束表现、音效、UI 图标。

## 后续建议

下一步不建议立刻重构资产目录。更好的顺序是：

1. 在 UE 编辑器里打开 `Free_Magic` 和 `Free_Spells` 的 Niagara 预览图，挑出 3-5 个看起来最顺眼、比例最合适的 VFX。
2. 以 `ParagonPhase` 作为法术标准验证角色，先确认它的 Skeleton、Mesh、AnimBP、Socket 和候选动作能否正常预览。
3. 只迁移一个动作：例如 Phase `Cast` 或 `RMB_Throw`，走完整 retarget + montage + GameplayEvent Notify 流程。如果直接以 Phase 本体做验证角色，则优先在 Phase 自身 Skeleton 上创建 Montage，不急着 retarget 到 Aura 当前主角。
4. 用这个动作和 VFX 重新验证火球术。
5. 再为生命涌动选一个持续表现，验证 ChannelCast。

等这两条链路稳定之后，再进入近战连击、领域、召唤/陷阱等机制。这样资产会自然服务于技能架构，而不是反过来牵着架构跑。
