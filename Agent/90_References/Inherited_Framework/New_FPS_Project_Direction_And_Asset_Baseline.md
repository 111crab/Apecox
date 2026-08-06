# 新 FPS 项目方向与资产基线总结

日期：2026-08-05

> 本文是新 UE 项目创建前的迁移摘要，用于保留本轮讨论。它不是最终架构 RFC，也不代表现有 Apex 代码会被整体迁移。新项目建立后，应先重新确认产品边界，再选择性继承代码、设计与资产。

## 一、项目方向调整

旧 Apex 项目验证了第三人称角色、GAS 基础、配置驱动技能、投射物、GameplayCue 和多人同步，但以 Paragon 动作设计第三人称英雄技能时，持续受到角色动画与技能表现资产限制。为了做出更适合 UE 游戏客户端求职展示的作品，后续计划新建一个多人英雄射击项目。

当前目标不是复刻商业级《Apex Legends》《三角洲行动》或完整大逃杀，而是在可控范围内完成高质量多人垂直切片，重点展示：

- 本地第一人称战斗表现与远端第三人称世界表现。
- 多种武器、弹药、换弹、拾取、槽位与基础配件。
- 护盾、生命、伤害、死亡与重生。
- 少量英雄技能与枪械战斗的组合。
- GAS、GameplayTag、GameplayEffect、AbilityTask、GameplayCue 的深入使用。
- 服务端权威、客户端响应、预测、同步、延迟与丢包验证。
- 配置驱动、模板复用、特殊机制扩展和清晰的系统职责。
- 面向面试讲解的架构、调试、测试与性能证据。

首版更适合小型多人竞技场，而不是大地图大逃杀。具体游戏模式、队伍人数、胜负循环和观战方式尚未最终确认。

## 二、从 Aura / Apex 保留的经验

- 不追求一个 DataAsset 或一个万能 GA 配出所有武器与技能。
- `SkillDefinition` 或同类根配置是组装入口，不是所有逻辑字段化的容器。
- 公共生命周期可由 GA 模板复用；异步行为使用 AbilityTask；持续世界对象使用 CombatEntity；特殊生命周期允许专用 GA。
- AbilitySet 是向 ASC 授予能力的清单，不是技能定义或运行时状态容器。
- InputTag 属于装配和输入槽位，不属于技能固有身份。
- GameplayTag 只表达稳定、可查询、可组合的战斗语义，不为临时流程结果无限增加 Tag。
- 玩家 ASC 由 PlayerState 持有、Character 作为 Avatar 的方案已经通过多人验证，可以作为新项目候选。
- 第一/第三人称表现应共享权威战斗事实，但分离 Mesh、动画、Montage、可见性和局部表现资源。
- Lyra 用于定向学习成熟边界，不直接换皮，也不复制与项目规模不相称的完整 Experience / GameFeature 体系。
- 代码实施仍采用先讨论设计、审阅关键命名、子代理实施、Codex 审查、用户 UE 配置与单/多人验证的协作方式。

## 三、现有资产总览

`D:\UE_Resource` 当前约 3.59 GB、4232 个文件，其中包括 4172 个 `.uasset`、34 张 `.umap` 和 4 个源文件压缩包。

### 1. Modern Guns Pack

路径：`D:\UE_Resource\Modern Guns Pack 现代枪械动画`

- 本地包标注版本：UE 5.4。
- 约 758 MB、2270 个 `.uasset`、31 张演示地图。
- 包含 8 个武器族：突击步枪 AG14W、轻机枪 HVG7、冲锋枪 SP60、手枪 X13、狙击枪 MR22、左轮 RC425、霰弹枪 MAK12、火箭筒 LRAF9。
- 每种武器包含骨骼网格、模块化部件和逐武器第一人称动作。
- 动作覆盖开火、瞄准、换弹、空仓换弹、检视、移动、冲刺、跳跃、收枪、手雷和近战等。
- 包含两套第一人称手臂、139 个 WAV、146 个 Sound Cue、基础枪口特效和演示蓝图。
- 官方说明其动画和手臂基于 UE4 Mannequin 骨骼，不直接兼容 UE5 Manny。

建议职责：作为第一人称武器与逐武器动作主库；远端第三人称动作不要求与它逐把一一对应。

### 2. Stephen_FPS

路径：`D:\UE_Resource\Stephen_FPS\FPS`

- `EngineAssociation` 为 UE 5.7。
- 约 1.48 GB、985 个 `.uasset`、2 张地图。
- 是纯内容项目，没有 C++ `Source`。
- 提供步枪和手枪的第一人称手臂动作与第三人称 Manny/Quinn 动作。
- 第三人称包含开火、换弹、装备、蹲伏、跳跃、转身、Aim Offset 和 Blend Space。
- 提供步枪/手枪骨骼与静态网格、枪口火焰、曳光、弹壳、环境命中特效、音频和 HUD 素材。
- HUD 素材覆盖准星、弹药、生命、武器卡片和击杀信息。

建议职责：作为新项目第一把步枪纵向切片的首选资产，也是第一/第三人称动画结构的参考样本。

### 3. Rifle Pro MoCap Pack

路径：`D:\UE_Resource\UE-FPS-人物动作\Rifle Pro MoCap Pack`

- `.uasset` 内部版本来源可追溯到 UE4.10，使用 UE4 Mannequin Skeleton。
- 约 1.36 GB、917 个 `.uasset`，其中 886 个动画。
- 包含 448 个 In-Place、348 个 Root Motion、48 个 Aim Offset 和 42 个开火/换弹/收枪/切换动作。
- 绝大多数动画属于步枪 `W2` 体系；1911 主要作为切枪道具，不是完整手枪动作库。
- 包含 M4、1911、A-Pose、T-Pose、FBX、Maya 和 MotionBuilder 源文件。

建议职责：作为第三人称步枪动作补充库。只迁入明确需要的动作，不全量复制。

## 四、第三人称动画复用决定

第三人称不需要为每一把枪制作完全独立的动作。当前采用武器姿态家族思想：

| 动画家族 | 可复用武器 |
| --- | --- |
| `Rifle` | 突击步枪、冲锋枪、轻机枪、狙击枪、弹匣式霰弹枪 |
| `Pistol` | 自动手枪、左轮手枪 |
| `Heavy` | 火箭筒等肩扛重武器 |

第一人称优先使用 Modern Guns 的逐武器精确动作。第三人称优先保证姿态、朝向、握持、出手时机和网络状态正确，通过 Socket、左手 IK、Aim Offset、播放速率和少量 Montage 覆盖改善视觉。

只有管式霰弹枪逐发装填、左轮弹巢换弹、拉栓狙击、火箭筒装填、弓箭拉弦等明显改变动作语义的武器，才值得后续增加专属第三人称动作。

## 五、UE 版本与兼容性风险

| 资产 | 引擎版本风险 | 骨骼/动画风险 | 综合判断 |
| --- | --- | --- | --- |
| `Stephen_FPS` | UE 5.7 升级到 UE 5.8，通常风险较低 | 已包含 UE5 Manny/Quinn 与独立第一人称手臂 | **低风险，优先使用** |
| `Modern Guns Pack` | 本地包为 UE 5.4，升级到 UE 5.8 通常可行 | 官方明确为 UE4 Mannequin，不直接兼容 UE5 Manny；枪口粒子以旧 Cascade 为主 | **中风险，主要是骨骼和旧表现技术，不是引擎打不开** |
| `Rifle Pro MoCap Pack` | 资产源自 UE4.10，直接读取旧 `.uasset` 的风险最高 | UE4 Skeleton、旧 Retarget 工作流、Root Motion/Additive/Notify 都需要重新验证 | **高风险，优先使用 FBX 重导入与 UE5 IK Retarget** |

需要区分两个问题：

- “引擎版本兼容”决定旧 `.uasset` 能否被新引擎稳定加载、升级和保存。
- “骨骼兼容”决定动画能否直接播放在目标角色上。资产能在 UE 5.8 打开，不等于能直接播放在 Manny 上。

对于计划中的 UE 5.8 新项目：

1. `Stephen_FPS` 最接近目标版本，适合作为第一轮迁入资产。
2. `Modern Guns Pack` 可以保留其独立第一人称手臂和原骨骼，不必强行先转到 Manny；武器 Mesh 可以用于第一和第三人称。
3. `Rifle Pro` 先在隔离测试项目尝试打开；如有兼容问题，直接从附带 FBX 重新导入，并通过 IK Rig / IK Retargeter 转到 UE5 Skeleton。
4. 从旧版本升级后的资产应另存到项目自有目录，不修改原始资产包。
5. 不要尝试把 UE 5.8 保存过的资产再放回 5.7、5.4 或 UE4 项目。

## 六、新项目创建后的资产迁入顺序

1. 创建 UE 5.8 C++ 项目并先验证空项目编译、PIE、Git 和插件基线。
2. 只迁入 Stephen_FPS 的步枪、第一人称手臂、对应第三人称动作和最小 VFX/UI 依赖。
3. 先完成一把步枪的第一/第三人称表现验证，不立即实现完整武器逻辑。
4. 再从 Modern Guns 选择一把霰弹枪或狙击枪，验证第二武器家族。
5. Rifle Pro 只在 Stephen 动作确实不足时按需重定向。
6. 第三方原始资产不进入公开 Git；公开仓库只保存项目代码、自建资产和必要的依赖说明。

## 七、新项目建立后优先讨论

1. 新项目名称、目录、UE 模板和最小插件。
2. 本地第一人称、远端第三人称、死亡与观战视角边界。
3. 小型多人游戏模式与一局游戏的最小循环。
4. 第一把步枪和第二把差异化武器。
5. Character、Pawn、PlayerState、ASC、Equipment、Inventory、Weapon 与 GA 的职责。
6. 第一/第三人称 Mesh、AnimInstance、AnimLayer、Montage 和武器动画 Profile。
7. Lyra 下一轮需要定向阅读的源码，以及明确不采用的重型部分。
8. 旧 Apex 代码哪些选择性迁移、哪些只保留为经验。

## 八、当前未定事项

- 是否继续使用项目名 `Apex`。
- 首版具体游戏模式和队伍人数。
- 本地第三人称是否仅出现在死亡/观战阶段。
- 首把步枪的射击模型是 Hitscan、Projectile 还是二者组合。
- 第二把验证武器选择霰弹枪还是狙击枪。
- 是否使用 Manny/Quinn 作为统一世界角色。
- 武器开火、换弹、装备等行为中哪些进入 GAS，哪些由 Equipment/Weapon Runtime 承担。

官方参考：

- Modern Guns Fab：https://www.fab.com/listings/6914a185-4b14-475c-8f4f-cc0a6a3c589f
- Modern Guns 文档：https://docs.infimagames.com/product/low-poly-animated-modern-guns-pack/getting-started/introduction
- Fab 标准许可证：https://www.fab.com/eula?lang=en

