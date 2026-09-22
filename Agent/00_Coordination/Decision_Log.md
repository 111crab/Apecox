# Apecox 决策记录

## 2026-09-21 - 高速实体弹丸与多人曳光分层

- `AApecoxWeaponProjectile` 继续是服务器唯一弹道、碰撞和伤害真相；可视曳光是由已接受射击派生的纯表现，不能反向决定命中。
- 不继续用高速复制 Actor 的逐帧位置表现拖尾。Authority 只为每发广播一次实际起点、散布后方向、弹速、重力和距离，各客户端本地模拟短寿命发光段。
- 高频曳光使用 `NetMulticast, Unreliable`；Dedicated Server 不生成，客户端 Actor 不复制、无碰撞、最长 0.75 秒。允许偶发丢失一条纯表现，禁止用 Reliable 为自动步枪持续堆积 RPC。
- 第三人称检视同步由用户取消，不再作为下一阶段或可选扩展安排。
- 单人以及 Listen Server Host/Client 双向人工验证已经通过；该项正式收口。

## 2026-09-14 - 人机改为静止靶标，先补齐步枪命中表现

- 当前机器人不再承担自由移动演示职责；`AApecoxWanderAIController`默认关闭寻路，只保留AIController、PlayerState、ASC、受伤、死亡和重生闭环。
- 当前一倍镜固定装入`BP_Apecox_RiflePresentation_FP`并使用RAR的`SOCKET_Scope`与Scope_01参数，不建设通用配件库存、兼容表或运行时换镜。
- 移动准星由HUD读取真实水平速度并平滑改变线段间距；它只表现已有移动散布趋势，不创建第二份弹道散布真相。
- 实体子弹继续通过`GameplayCue.Weapon.Impact`触发表面表现；粒子、声音和短寿命弹孔属于GameplayCue，伤害仍由Projectile与ASC结算。
- 镭射拆为下一批持续表现：固定挂件、开关输入、持续Trace、光束和命中光点；遵守RAR的ADS允许、冲刺关闭规则。

## 2026-09-11 - 换弹弹匣采用 RAR 双对象视觉交接

- `MagazineDefault` 继续代表枪槽中的固定弹匣；换弹手中弹匣由表现 Actor 的独立 `MagazineReserve` 表示。
- `MagazineReserve` 附着武器骨架 `SOCKET_Magazine_Reserve`，由 Weapon Reload Animation 驱动轨迹；不附着 `hand_l`，也不参与弹药或网络真相。
- 备用弹匣运行时复制 `MagazineDefault` 的 Mesh 与材质，避免 Blueprint 重复配置同一外观。
- 固定/备用显隐采用已核对的 RAR Character Montage Notify 时间；Apecox 不迁移供应商弹药 Notify，服务器换弹提交仍由自身事务负责。
- First Person FOV 120 只作为常态构图基线，不再用来解释换弹中缺少手持弹匣的问题。

## 2026-09-01 - 第一人称成为唯一表现质量主线

- Apecox 不再开发玩家可操控的第三人称视角，也不再把第三人称动画、Aim Offset、换弹或技能表现作为独立质量里程碑。
- 本地玩家的 FP Arms、FP Weapon、动画衔接、镜头、VFX/SFX 和操作反馈是唯一需要持续打磨的表现通道。
- 多人 FPS 仍保留远端 Character、Capsule、世界 Mesh、武器摘要和必要复制状态，使其他玩家可见、可命中并能参与服务器权威战斗；这一通道只承担最低技术验证职责。
- 不删除已经建立的 TP Mesh/Weapon 表现入口，但后续仅在它阻碍多人正确性时修复，不为其单独采购、重定向或调试动画资产。
- 当前优先收口第一人称空手与持枪的基础移动、跑动、跳跃和蹲伏，再继续换弹、ADS、命中反馈与 GAS 战斗闭环。

## 2026-08-24 - 武器外观采用固定完整枪械

- 当前不实现通过拾取配件动态改变枪械外观或功能的通用模块化武器系统。
- RAR 枪体、默认弹匣、护木和机械瞄具虽然来自拆分资产，但只在项目自建表现 Blueprint 中固定组装一次；运行时把它视为一把完整步枪。
- Weapon Presentation Actor 的目的只是提供可预览的完整武器 Prefab、枪械动画目标和枪口锚点，不拥有配件库存、数值 Modifier 或复制状态。
- 瞄准镜替换是后续可选项；真实进入 ADS 阶段时再用一个受控 Optic 入口设计，不提前建设通用配件框架。

## 2026-08-24 - 固定第一人称操控与路线图颗粒度重整

- Apecox 继续定位为小规模多人英雄 FPS 战斗垂直切片，不转为单机 FPS。
- 玩家只使用第一人称操控，不提供运行时第一/第三人称视角切换。
- Owning Player 使用 FP Arms/Weapon；其他客户端使用 TP Manny/Weapon。两套表现共享玩法真相，但动画资产、AnimBP、挂点和渲染组件可以独立。
- 第三人称不作为玩家操控模式开发，只保证远端玩家的持枪、瞄准、开火、换弹、技能和死亡等世界表现可读。
- 旧 `Phase 2B-2C` Prompt 暂停执行；Presentation Actor 思想保留为候选，需按新视角边界重新审阅。
- 项目计划改为“产品方向（L0）→ 进化里程碑（L1）→ 能力包（L2）→ 当前迭代（L3）”。不再用无限细分的 Phase 编号承载路线图。
- 当前唯一主线是完成第一把步枪可玩闭环；先独立收口 FP 完整武器表现，再处理远端 TP 最低可读表现。

## 2026-08-05 - 新项目方向与上下文迁移

- 放弃继续在 Aura/Apex 旧业务代码上演进，Apecox 从干净 UE 5.8 C++ 项目重新建立运行时。
- 新项目面向 UE 游戏客户端求职作品，方向为小规模多人英雄射击垂直切片；不以完整大逃杀内容量为目标。
- Lyra 用于学习职责边界、GAS、武器、输入、预测与复制，不整体照搬 Experience、GameFeature、ModularGameplay。
- 旧项目代码不自动迁移；旧技术思想通过“综合基线 + 精选参考 + 去重原文归档”保存。
- Apecox 继续使用 Codex 规划/审查、ClaudeCode 实施、用户审核和 UE 手工验证的协作方式，但减少重复文档。
- `Current_Phase.md` 是当前状态、下一步和待确认事项的唯一入口。

## 继承但仍需 Apecox RFC 落地的原则

- 不追求万能 GA 或万能配置；追求可复用流程、受控扩展和快速开发。
- GA 扩展采用：公共基类、稳定流程模板、AbilityTask、虚函数钩子、必要的专用 GA。
- SkillDefinition 是组装入口；AbilitySet 是授予层；InputConfig 是 IA 到 InputTag 的映射层。
- 目标选择拆分为施法目标、衍生物生成规则和衍生物检测规则。
- GameplayTag 只表达稳定语义；封闭选项优先枚举或类型化配置，避免特殊情况驱动 Tag 膨胀。
- Effect/State/Buff、CombatEntity、Presentation、Timeline 都按真实需求分阶段引入。
- 单人完成不等于技能完成；涉及玩法的阶段必须给出 Listen Server + Client 验证口径。

## 2026-08-05 - 战斗框架首轮审阅与实施启动

- 架构 RFC 作为后续专题设计的框架基线，但不代表其中所有玩法分叉已经一次性冻结。
- 用户列举的武器、护盾、配件、特殊技能、双视角和滑铲行为用于压力测试架构；具体实现仍按局部阶段逐项讨论。
- 当前先做可玩的战斗原型，不实现胜利条件、积分结算或完整比赛模式；最小 GameMode/GameState 仍承担多人出生、死亡与复活职责。
- V1 输入范围只覆盖键盘和鼠标，不规划手柄映射、辅助瞄准和手柄 UI。
- 普通武器槽固定为两个；技能临时武器或特殊装备不默认占用普通槽，具体规则随技能确认。
- 第一把步枪首版只实现腰射；Hitscan/Projectile、双视角具体资产方案和相机/枪口弹道口径在武器垂直切片前讨论。
- “FP Arms/Weapon + TP World Body/Weapon”同时描述人物姿态和武器表示。运行时有两个表现通道，但不强制制作两份独立源 Mesh；FP/TP 动画可以不同，玩法时序和结算语义必须只有一个来源。
- 死亡后武器、配件、弹药和护盾电池形成世界掉落并在死亡位置附近散落；EvolutionProgress 保留。安装配件是否拆分、掉落散布和并发拾取稍后确定。
- 护盾电池允许移动使用，受伤不取消，切枪或技能会取消；持续时间、消耗时点和取消结果稍后确定。
- 对敌人造成的有效伤害转化为 EvolutionProgress，且进化进度在死亡/复活后保留。
- V1 配件规则由武器及武器类型决定，不允许英雄被动直接改变单把武器的配件兼容和聚合规则。
- 实施按 `Project_Roadmap.md` 的纵向阶段推进；日常只维护 `Current_Phase.md`，不新增重复的实施计划文件。
- Phase 0 关闭 Hardware Ray Tracing 与 Substrate，保留 DX12、SM6、Lumen 和 VSM；禁用实验性 `GASToolsets`，显式启用官方 Gameplay Ability System。
- 项目自有 Gameplay 资产统一进入 `/Game/Blueprints/...`。这里把 `Blueprints` 作为项目内容命名空间，不限制只能存放 Blueprint 类资产；第三方商业资产仍放在其他独立顶层目录且不进入公开仓库。
- GitHub 远端使用 `https://github.com/111crab/Apecox`；项目自建 `.uasset/.umap/.ubulk/.uexp` 使用 Git LFS。
- 初期验证地图新建为非 World Partition 的轻量 `L_Apecox_DevGym`。Lyra `L_ShooterGym` 仅作为布局参考；不迁移其 GameFeature/Experience 依赖，也不迁移收益有限的旧 Apex 模板地图。
- Phase 0 已完成 `GameplayAbilities` 显式启用、Hardware Ray Tracing/Substrate 关闭、轻量 DevGym、单人/2 人 Listen Server 和 `ApecoxEditor Win64 Development` 构建验证；旧地图 Redirector 已清理。

## 2026-08-06 - Phase 1A 玩家与 ASC 生命周期设计

- Phase 0 已以提交 `e115f8f chore: establish Apecox project baseline` 推送到 `origin/main`，开始 Phase 1。
- Phase 1 拆分为三个可独立编译验证的小闭环：1A Gameplay Framework/ASC 所有权，1B AbilitySet/InputTag/最小 Ability，1C Health/Death/Respawn。
- Phase 1A 批准创建 `AApecoxGameMode`、`AApecoxGameState`、`AApecoxPlayerController`、`AApecoxPlayerState`、`AApecoxPlayerCharacter`、`UApecoxAbilitySystemComponent`、`UApecoxVitalAttributeSet`。
- `AApecoxGameMode` 继承 `AGameModeBase`，当前不引入完整比赛状态机。
- 玩家 ASC 由 `AApecoxPlayerState` 真正拥有并使用 Mixed 复制；`AApecoxPlayerCharacter` 只作为当前 Avatar 和 ASC 访问桥梁。
- `UApecoxVitalAttributeSet` 本批只建立 `Health/MaxHealth`，不提前加入 Shield、EvolutionProgress 或死亡行为。
- 新 ClaudeCode 窗口必须先阅读 Apecox 当前协作规范和设计文件；子代理继续只负责按批准 Prompt 实施并提交中文报告，Codex 负责后续审查。

## 2026-08-06 - Git 采用主动暂存、阶段提交、低频推送

- 已验证的小目标完成后，Codex 可以主动暂存该目标的明确改动。
- commit 以可说明的功能闭环或小阶段为单位，不为零散改动频繁提交。
- push 由用户负责最终收口；Codex 仅在形成足够稳定的里程碑时建议，并在获得确认后执行。
- Git 服务于恢复和协作，不应拖慢架构讨论、实现和验证。

## 2026-08-06 - Phase 1B AbilitySet 与输入生命周期

- `UApecoxGameplayAbility` 作为项目抽象 GA 基类；激活策略采用 `OnInputTriggered / WhileInputActive / OnAvatarSet`。
- Ability 并发采用 `Independent / ExclusiveReplaceable / ExclusiveBlocking` 封闭枚举；本阶段不引入 Tag Relationship Mapping。
- `UApecoxAbilitySet` 是 Authority 授予、可由 Handles 撤销的 `UPrimaryDataAsset`，不是技能完整定义。
- 输入链采用 IA -> InputConfig -> InputTag -> AbilitySpec；ASC 在 PlayerController `PostProcessInput` 阶段统一处理 Pressed/Held/Released。
- 首批 Native Tag 仅为 `InputTag.Ability.Tactical` 与 `State.Input.AbilityBlocked`；Tactical 绑定键盘 Q。
- Pawn AbilitySet 由 Character 保存授予 Handles 并随 Avatar 生命周期撤销；装备和英雄玩家级授予以后由各自所有者管理。

## 2026-08-10 - Phase 1C 死亡与重生职责分层

- `Health/MaxHealth` 继续由 PlayerState 上的 `UApecoxVitalAttributeSet` 持有；AttributeSet 只检测变化并广播 OutOfHealth，不控制 Pawn。
- 新建 Pawn 生命周期组件 `UApecoxHealthComponent : UActorComponent`，负责绑定 ASC、复制严格死亡状态和向 GAS 暴露死亡 Tag；组件不重复存储生命值。
- 死亡状态采用 `NotDead -> DeathStarted -> DeathFinished`；`State.Death.Dying` 表示死亡流程执行中，`State.Death.Dead` 表示旧 Pawn 可以完成销毁，两者互斥。
- `UApecoxDeathAbility` 由 `GameplayEvent.Death` 触发，负责取消普通 Ability、清空输入、进入 Blocking 组，并为未来死亡蒙太奇保留异步结束出口。
- `AApecoxPlayerCharacter` 只处理当前身体的移动、碰撞、解绑与销毁；`AApecoxGameMode` 只在服务器延时 3 秒后重生。
- PlayerState、ASC 和 VitalAttributeSet 跨死亡保留；Pawn AbilitySet 随 Pawn 撤销和重新授予；复活恢复满生命。
- Phase 1C 使用直接修改 Health 的 Debug GE 验证生命周期，不把它当正式伤害架构，也不提前加入 Shield、Damage Meta Attribute 或伤害类型。

## 2026-08-10 - Phase 1C 双视角与镜头收口

- 一个 Character、Capsule 与 CMC 继续作为移动和网络预测的唯一事实；第一/第三人称只拆分表现。
- 项目自有 `AApecoxPlayerCameraManager` 使用 UE 5.8 官方 `ViewPitchMin=-70 / ViewPitchMax=80` 基线，并随 PlayerController 跨 Pawn 重生保留。
- Motion Blur 常驻关闭，避免第一人称近景移动产生不必要的拖影干扰。
- Pitch 钳制已验证生效，但完整 Manny 空手第一人称仍存在轻微胸腔透视和跑动观感问题。该问题归入表现质量，不继续用更窄 Pitch 或缩放参数硬压。
- 用户批准结束 Phase 1C；空手第一人称专用 Arms/动画暂不阻塞，随第一把步枪的 FP/TP 表现架构统一处理。

## 2026-09-14 - 固定瞄具跳跃与武器粒子修复

- 固定一倍镜刚性附着保持不变。ADS时只把现有Jump Mesh Space Additive从100%平滑降到10%，避免红点大幅离开中心；腰射仍保留完整跳跃动作。
- 腰射准星以独立14像素腾空项提示跳跃精度下降，并与水平移动项叠加；UI不反向驱动真实散布。
- 枪口Niagara继续只使用`NS_IG_MuzzleFlash`，但按RAR真实SpawnSystemAttached调用补Yaw 90度相对旋转；不创建重复烟雾系统。
- GameplayCue专用原生表现类不再预填派生资产标签。`GCN_Weapon_Impact`必须显式序列化`GameplayCue.Weapon.Impact`，并由`/Game/Blueprints/GameplayCues`确定路径扫描。
- 当日最终PIE确认ADS跳跃、腾空准星、Impact粒子、弹孔和命中声通过。Yaw 90度修正后枪口硝烟仍不可见；该问题单独留到下一工作日检查Smoke Emitter运行数据，不阻塞今天收口，也不回退已通过项。

## 2026-09-17 - 移动人机采用轻量AI与速度驱动AnimBP

- Lyra调查确认持续移动动画由AnimBP状态机和Linked Anim Layer驱动，不使用空手Walk/Jog Montage；行为树只产生目标和MoveTo请求。
- Apecox保留现有随机漫游Controller，不迁移Lyra Experience、GameFeature、AI Perception、EQS和完整Shooter行为树。
- `BP_ApecoxBotCharacter`当前实际引用`ABP_Unarmed`，不是此前修改的`ABP_Apecox_Manny`。下一步先统一Bot Anim Class和动画变量来源，再恢复`bEnableWandering`。
- Bot地面移动状态以真实水平速度为准；`bRequestedMoveUseAcceleration=true`仅负责MoveTo起停平滑，不把瞬时加速度作为持续Walk/Jog的必要条件。
- 首轮继续使用项目已有同骨架`BS_Idle_Walk_Run`。只有基础漫游通过后，才按需重定向Lyra Idle/Walk/Jog/Start/Stop/Pivot/Jump AnimSequence；不把持续移动改成Montage。

## 2026-09-08 - RAR 第一人称步枪垂直切片优先

- 当前产品验收只要求第一人称单人 PIE；Listen Server、Client 和远端第三人称不再作为日常阶段验收项。
- 已有服务器权威射击、复制和远端世界表示代码继续保留，但不为当前动画阶段增加自定义 CMC、FSavedMove 或第三人称适配。
- 暂停 LPSP 空手重定向，优先使用 RAR 已有同骨架资产完成持枪移动、冲刺、跳跃、蹲伏、趴下和左右探头。
- RAR Demo 只作为动画图、资产选择和参数参考；Gameplay、Equipment、GAS 与 Hitscan 流程继续由 Apecox 自己实现。
- 输入口径调整为：Left Shift 冲刺、Left Ctrl 蹲伏、Z 趴下、Q/E 左右探头、F 交互、X Tactical、鼠标左键开火。
- 第一批直接在现有 Character 上实现单人移动状态，不新建自定义 CMC；未来重新要求多人移动预测时再升级到 FSavedMove 方案。

## 2026-09-18 - 项目收口清理由用户口令触发

- 全项目废弃设计、试验分支和冗余代码清查暂不执行，避免在功能仍调整时误删仍被资产引用的入口。
- 只有当用户明确说“项目正式收口时”，才启动代码、配置、资产引用和协作文档的全量清查、验证与删除。
- 当前功能修复仍可删除直接导致错误且已被新实现取代的局部逻辑；这不等同于提前进行项目级清理。

## 2026-09-18 - 镜头后坐停止后不自动改写玩家视角

- 本发镜头Kick会完整执行，包括把弹匣打空的最后一发；Kick结束后的弹簧回零只清理内部状态，不把负差值写回真实`ControlRotation`。
- 枪械模型继续自然回稳。玩家是否向下压枪、压到什么方向，停火或弹空后都由当前真实视角保留。
- 生命周期重置只清空后坐状态，不再通过反向旋转“撤销”程序后坐，避免与玩家输入重复计算。

## 2026-09-18 - 第一人称单步枪垂直切片正式收口

- 用户发出“项目正式收口”口令，启动此前延后的全项目废弃设计、试验分支、配置、资产引用和当前态文档清查。
- 步枪运行时统一为 Projectile 模型；删除 Hitscan Ability、专用 TargetData 和配置入口。Ranged Fire 基类继续提供共享输入、预测、校验与提交框架。
- 删除 `X` 战术输入、调试自伤 Ability/Effect 和旧输入生命周期演示资产；基础 Pawn AbilitySet 只保留死亡 Ability。
- 移动 AI 暂缓。`AApecoxWanderAIController` 删除寻路实现，只因 `BP_ApecoxBotCharacter` 的序列化类路径保留名称，并继续请求 PlayerState 以复用 GAS 生命周期。
- 删除未使用旧 AnimBP、Retargeter、NavigationSystem 依赖、重复 DX12 配置、未用库存堆叠字段和无资产引用的 IK 布尔字段。
- `AApecoxWanderAIController`、`DA_Phase1B_AbilitySet`、`LeftHandIKEffectorTransform` 与 `GE_Weapon_Rifle_Damage_Debug` 保留现有名称或字段，原因是 Blueprint/资产序列化兼容收益高于纯命名清理收益。
- 空仓后坐最终语义冻结：本发镜头 Kick 完整执行；停火或弹空后内部弹簧静默归零，不再反向改写玩家真实视角。
- 正式交付边界是第一人称单步枪体验与静止可重生靶标。移动 AI、积分胜利/HUD 提示、独立击杀确认、多人专项回归、切枪/配件/第二武器、护盾/进化/英雄技能转为未来重新立项内容。
- 最终验证：ApecoxEditor 完整构建无警告；Apecox Blueprint 0 错误、0 警告、0 加载失败；资产断言通过；`Apecox.*` 64/64 成功、0 失败。

## 2026-09-18 - 第三人称单步枪动画采用 Lyra 素材、Apecox 运行时

- 第一人称和第三人称共享同一套 Character、CMC、GAS、Equipment、Projectile、弹药与动作事务，分别产生 FP Arms/Weapon 和 TP Manny/Weapon 表现；不复制 Gameplay，也不要求两套动画逐帧一致。
- Lyra 作为第三人称 Manny 步枪动画、Blend Space、Aim Offset、Montage 与骨架素材来源；不迁移 `ABP_Mannequin_Base`、`ABP_RifleAnimLayers`、Experience、GameFeature、自定义 CMC 或 Lyra 武器运行时。
- 新建 Apecox TP AnimBP，继承 `UApecoxCharacterAnimInstance`。当前只有一把步枪，继续扩展 `UApecoxWeaponPresentationDefinition`，暂不引入 GameplayTag 查询式通用 Animation Set 或 Lyra Linked Anim Layer 框架。
- 首选直接迁移 Lyra 的 `SKM_Manny`、`SK_Mannequin` 与精选 Rifle 动画，并以 Lyra Skeleton 创建新 TP AnimBP；不假设它与 Apecox 模板路径下的同名 Skeleton 是同一资产。
- CMC/Character 继续提供 Velocity、Falling、Crouch，装备摘要提供 AnimationFamily，Fire 继续使用现有 GameplayCue；只为 Sprint、Prone、ADS、Lean、Reload/Inspect 等缺少远端真相的状态补精简复制。
- 玩家始终使用第一人称操控，不增加本地第三人称相机。第三人称按远端 Mesh/武器可见性、基础 Locomotion、Aim、UpperBody Action、复制状态、IK/Turn In Place、多端回归的顺序实施。移动 AI 继续暂缓，但完成后的 TP AnimBP 可供静止 Bot 和未来 AI 复用。

## 2026-09-18 - 双视角以动作语义一致为同步标准

- 第一人称 RAR Arms 与第三人称 Lyra Manny 可以使用不同 Skeleton、Sequence、Montage 长度和姿势细节；不做逐帧 Pose 复制，也不为视觉一致性引入 Retarget 依赖。
- Apecox 的 Character/CMC、GAS、Equipment 和 GameplayCue 只发布移动、装备族、瞄准、开火、换弹等唯一动作语义；FP/TP 各自选择适合其骨架的动画表现。
- 第一批直接使用 Lyra `SKM_Manny`、Lyra `SK_Mannequin` 和配套 Rifle 序列，并以 `UApecoxCharacterAnimInstance` 为新 TP AnimBP 父类。这是当前单步枪范围内开发速度最快、状态边界最清楚的方案。
- 首轮只接 Idle/Walk/Jog。连续移动通过后再逐批加入 Jump、Aim 和 UpperBody Montage，避免同时排查骨架、Locomotion、复制与动作时序。

## 2026-09-18 - 第三人称武器外观允许独立于第一人称

- 当前目标是让远端正确理解角色行为，不要求 FP RAR 步枪和 TP 武器使用同一 Mesh。第一人称继续使用完整 RAR Presentation，第三人称优先使用与 Lyra Manny Rifle 动画原生配套的 `SK_Rifle`。
- Lyra 实际配置为 `weapon_r` Socket、Location 0、Rotation Yaw -90、Scale 1。Apecox 不再用 `hand_r + 单位变换` 强行套 RAR TP Mesh。
- TP Mesh 差异只属于表现层；GAS、Equipment、弹药、Projectile、命中和动作语义仍由 Apecox 的同一套玩法事务驱动。

## 2026-09-18 - 单步枪版本取消空手主流程

- 玩家出生与重生时由服务器自动向 Primary 库存槽放入并装备 `DA_Weapon_Rifle`；不再要求先以空手状态寻找并按 `F` 拾枪。
- 默认配装必须走既有 `Inventory -> Equipment` 原子事务，使弹药、GAS、复制和双视角 Presentation 继续共享同一个武器实例；禁止在角色 Mesh 上单独生成无玩法状态的装饰枪。
- 当前第三人称 AnimBP 直接按 Rifle 家族构建。静止和移动基础 Pose 均采用 Lyra Rifle 序列；单独替换枪 Mesh/Socket 不能让普通空手动画进入持枪姿势。

## 2026-09-18 - 第三人称地面移动直接使用完整 Rifle Locomotion

- 当前只实现 Rifle Idle、四向移动、四向冲刺和移动中的持续持枪，不提前接入开火、换弹、跳跃、蹲趴、Aim Offset 或左手 IK。
- `BS_Apecox_Rifle_TP_Locomotion` 中的 Idle、Walk、Jog 均为完整 Lyra Rifle 动画，已经包含基础持枪姿势。用户验证 `MM_Rifle_Hipfire_OverridePose` 权重归零后腿部恢复，说明常驻静态覆盖是重复且有害的；基础图改为 Blend Space 直接输出。
- 离线根骨轨迹显示 Walk 素材约 `300 cm/s`、Jog 素材约 `600 cm/s`。后续冷加载确认 `BP_ApecoxPlayerCharacter` 的真实玩法速度一直是普通 `400 cm/s`、冲刺 `650 cm/s`，而非当时按 C++ 旧默认值假设的 600/900。Blend Space 的 Walk 行最终移到 `400`；Jog 保留 `600@1.0` 与 `900@1.5` 两行，使 `650` 冲刺在同一 Jog 动画的两个倍率间插值得到约 `1.083` 倍播放率。轴上限保留 900，不引入 Stride Warping。
- 后续 Aim Offset、开火和换弹以明确的表现层或 UpperBody Slot 加在完整 Rifle Locomotion 之后，不再用静态覆盖 Pose 代替动作状态。

## 2026-09-19 - 复杂动画资产必须经过冷加载验证

- `BS_Apecox_Rifle_TP_Locomotion` 首版由 Python 直接写入 `sample_data` 并保存，遗漏 Blend Space 编辑器负责构建的三角剖分与网格采样数据，导致冷启动时停在静止 Pose；手工打开资产会触发构建，因此问题会暂时消失。
- 修复通过原生 Blend Space 编辑器初始化路径重建并保存派生数据；资产从 `9,521` 字节增长到 `41,826` 字节。新增 `Apecox.Animation.ThirdPersonBlendSpaceColdLoad`，在不打开资产编辑器的全新 UE 进程中直接验证运行时采样。
- 用户已完成最终冷启动双端 PIE 验证。以后少量复杂动画资产优先由用户手工迁移或配置；需要自动化时，必须先确认原生构建入口，并通过“全新进程冷加载 + 运行时求值 + 最小 PIE”三层验证。

## 2026-09-21 - M3 采用原生最小 PvE Score Attack，不接入 Lua

- 用户取消 Lua/UnLua。后续基础界面使用 UMG，计划显示玩家生命、武器名、弹匣/备用弹药、玩家队/AI 队比分、目标分和胜负；准星与命中标记继续由当前 Canvas HUD 承担。
- 不整体迁移 Lyra AI。Lyra 只用于职责和行为拆分参考；Apecox 直接复用现有 Character、CMC、GAS、Equipment、Projectile、Health、Death、Respawn 和第三人称动画。
- 首版服务器战斗 AI 只做最近玩家目标、NavMesh 追击、视线/射程判断、现有 GAS 开火与换弹。Behavior Tree、EQS、感知、掩体、巡逻和武器拾取暂缓。
- GameMode 是唯一计分与胜负裁判；GameState 只复制公共比赛状态；PlayerState 保存个人 K/D 和阵营；HealthComponent 每次死亡只上报一次 Victim/Killer。
- 默认规则为玩家队对 AI 队、先到 10 分。PostMatch 后停止 AI 决策、新射击与后续重生。当前 DevGym 用于功能验证，闭环通过后再制作轻量 Arena。
- 命中标记只消费 Authority 对实体 Projectile 的最终伤害确认；成功发射、Miss 和环境碰撞不触发。腰射保留中心点，红色 X 在腰射与 ADS 共用。

## 2026-09-21 - 首轮 AI PIE 异常按视点、骨架和距离行为分别修复

- AI 开火输入实际已经触发，但服务器日志显示射击视点与 Pawn 相差约 13 至 14 米并被反作弊校验拒绝。原因是本地 TargetData 只从 `APlayerController` 获取 ViewPoint，而服务器 AI 没有 PlayerController。玩家继续使用 PlayerController ViewPoint；AI 改用 Pawn 的 `GetPawnViewLocation()` 与 `GetBaseAimRotation()`，两者仍进入同一 Authority 校验、弹药和射速事务。
- Bot 原先使用 FirstPerson 模板的 `SKM_Manny_Simple`，其 Skeleton 与 Lyra 持枪动画不是同一资产，也没有 `weapon_r` Socket，因此第三人称枪械只能落到错误位置，Montage 也会因 Skeleton 不一致被跳过。Bot 改用与玩家相同的 Lyra `SKM_Manny`，并用冷加载测试强制检查 `weapon_r`。
- DevGym 初始玩家与 Bot 约相距 11 米，已经进入 18 米开火射程。旧决策在射程内主动 `StopMovement()`，所以即使 NavMesh 正常也只会原地转向。首版近战行为改为保持面向玩家并每约 1.25 秒在 NavMesh 上选择一次 350 cm 横向换位；超出 18 米后继续追击。
- 命中标记线宽从 `2.5` 调为 `1.5`。腰射命中时红色四段 X 与原白色中心点和动态准星同时绘制，不再用命中状态替换普通准星；ADS 仍只显示红色 X。

## 2026-09-21 - AI 移动验收先使用可逆的 Bot 无伤模式

- 首轮 PIE 已经证明 AI 持枪、瞄准、实体 Projectile、伤害、死亡停火链路有效，但控制器持续保持开火输入，相当于零反应时间、零停火间隔的满射速炮台，玩家会在进入 PIE 后迅速死亡，无法观察寻路和横向换位。
- 不给玩家添加永久无敌状态，也不修改共享步枪的 Damage GE。Projectile 在识别到 Instigator 是 `AApecoxBotCharacter` 时读取 `apecox.AI.DamageEnabled`；默认 `0` 只跳过 Damage GE，仍保留弹道、碰撞、声音和 Impact。运行时设为 `1` 即可恢复真实伤害。
- Lyra 的 C++ BotController 主要提供 PlayerState、队伍、生命周期和 Perception 接口，具体战斗节奏位于上层行为资产。Apecox 当前不迁移整套 Lyra Experience/AI 资产；移动通过后，按反应时间、短点射、停火间隔、瞄准误差四个独立参数完善最小 AI。

## 2026-09-21 - AI 换弹走 TP 分流，ADS 改为点击切换，战斗节奏独立随机

- `IsLocallyControlled()` 不能单独作为第一人称判据：Listen Server 上的 AIController 也是 LocalController。Equipment 的视角分流统一改为“`IsPlayerControlled() && IsLocallyControlled()` 才是 FP”，因此 AI 的开火、换弹和镭射均走第三人称可见路径。
- ADS 取消 `Completed/Canceled` 松键退出绑定，只消费 `Started`。`bAimInputRequested` 保存点击切换意图；冲刺、换弹、死亡和卸枪继续清除该意图，避免高优先级动作结束后自动重新开镜。
- 每个 AIController 使用独立随机流：新目标反应 `0.45～0.9s`，点射 `0.2～0.45s`，停火 `0.65～1.35s`，每轮瞄准偏差水平 `±85cm`、垂直 `±45cm`；换位间隔 `0.8～1.8s`，横向距离 `250～600cm`，并依据当前距离做轻微前后修正。随机只生成服务器意图，射击仍走同一 GAS、Projectile 和 Authority 校验。

## 2026-09-21 - PostMatch 保留规则并补充最低可见反馈

- “仍有弹药但不能射击、所有 AI 同时静止”由默认 10 分目标触发，不是输入、弹药或 AI 随机故障。`PostMatch` 后双端拒绝新射击、AI 停止决策和停止安排重生均保持不变。
- 缺陷是玩家无法看见比分和比赛阶段。正式 UMG 尚未开始前，`AApecoxHUD` 直接读取复制的 `AApecoxGameState`，常驻显示双方比分，并在 `PostMatch` 显示 `VICTORY / DEFEAT` 和重启 PIE 提示；结果绘制不依赖 Pawn，因此玩家死亡后仍可见。
- 后续扩展优先级登记为伤害成长护盾、多人可预测滑铲、低墙 Mantle。自由贴墙攀爬所需自定义 Movement Mode 与完整动画集超出当前面试版范围。

## 2026-09-22 - 客户端占有完成后重新收敛 FP/TP 表现

- `IsPlayerControlled() && IsLocallyControlled()` 仍是正确的最终视角分流：它排除 Listen Server 上同样 Local 的 AIController。但客户端首次收到 `EquippedWeaponState` 时 Controller 可能尚未复制完成，不能假设 RepNotify 当帧已经得到最终分流关系。
- 早到的装备摘要会先隐藏 FP Arms、跳过 FP Weapon，并按观察者路径创建 TP 镭射宿主；过去没有 Controller 就绪后的重试，因此客户端会永久只剩物理镭射挂件。
- `AApecoxPlayerCharacter::PawnClientRestart` 是 Owning Client 本地占有完成入口。只在 `NM_Client && IsLocallyControlled()` 时调用 Equipment 的幂等重建：先销毁错误的本地表现，再依据现有装备摘要重新生成 FP Arms/Weapon。若 Controller 先到，后续正常 RepNotify 仍可生成；两种网络到达顺序结果一致。Listen Server/Standalone 不执行，避免重复装备动画。
