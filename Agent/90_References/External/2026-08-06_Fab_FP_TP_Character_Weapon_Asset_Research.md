# Fab 第一/第三人称角色、枪械与动画资调研报告

日期：2026-08-06  
执行者：ClaudeCode（子代理）  
调研方法：Fab 公开搜索 + 外部引擎页面交叉验证（Fab 商品页直接访问受限）  
查询日期：2026-08-06（所有价格与版本信息仅在该日有效）

> **重要声明**：由于 Fab 商品页面在本会话中无法直接访问（网络限制），以下信息通过搜索引擎结果、官方文档和第三方页面交叉验证获得。凡标注"价格未确认""版本未确认"的字段，请以 Fab 实际页面为准。本报告不构成购买建议，不提供法律保证。

---

## 1. 执行摘要

### 1.1 现有资产是否足以启动第一把步枪

**基本可以。** Stephen_FPS（UE 5.7）已经覆盖了第一把步枪纵向切片的核心需求：

- FP Arms + 步枪/手枪模型 + FP 动画（开火/换弹/装备）
- TP Manny/Quinn 动画（开火/换弹/装备/蹲伏/跳跃/转身/Aim Offset/Blend Space）
- 基础 VFX（枪口火焰/曳光/弹壳/命中）+ SFX + HUD

Modern Guns Pack 可作为第二武器家族（霰弹枪/狙击枪）的 FP 动画补充。

### 1.2 当前真正阻塞 Phase 2 的美术缺口

1. **霰弹枪管式逐发装填动画（P0）**：Stephen_FPS 和 Modern Guns 均无管式霰弹枪逐发装填的 FP/TP 专属动画。这是最明确、最急迫的单一缺口。
2. **狙击枪拉栓动画（P0）**：Modern Guns 有 MR22 狙击枪 FP 动画，但拉栓语义与常规步枪换弹不同，值得寻找更精确的 bolt-action 动画包。
3. **TP 通用 Locomotion（P1）**：Stephen_FPS 已有基础 TP 动作，但缺少完整的 Rifle/Pistol/Unarmed 姿态族（四向 Walk/Jog/Run/Sprint/Crouch/Turn In Place）。GASP 免费包可覆盖大部分。
4. **可复用角色外观（P1）**：当前没有任何可展示的第三人称角色 Mesh。Manny/Quinn 灰色人体不能用于作品展示。
5. **VFX 升级（P2）**：Stephen_FPS 的 Cascade 特效和 Modern Guns 的旧 VFX 可以在 Niagara 升级后使用；但如果需要一个现代 Niagara 表现包，也是值得考虑的。

### 1.3 是否有必要现在购买资产

**不必须。** 免费选项（GASP + MoCap Online Free + Paragon 社区包）可以覆盖大部分 TP 动作需求。但如果你想在 Phase 2 启动前就补齐霰弹枪和角色外观，建议购买 **1-2 个付费资产**（见第六节拼装方案）。

---

## 2. 现有资产覆盖与缺口矩阵

| 类别 | 子项 | Stephen_FPS | Modern Guns | Rifle Pro | 缺口程度 |
| --- | --- | --- | --- | --- | --- |
| **FP Arms** | 手臂 Mesh | ✓ 有 | ✓ 有（2 套） | 无 | **已覆盖** |
| **FP Anim** | Rifle Fire | ✓ | ✓ | 无 | **已覆盖** |
| | Rifle Reload/Empty | ✓ | ✓ | 无 | **已覆盖** |
| | Pistol Fire/Reload | ✓ | ✓ | 无 | **已覆盖** |
| | Shotgun Fire/Reload | 无 | ✓（弹匣式） | 无 | **缺管式逐发装填** |
| | Sniper Bolt-Action | 无 | ✓（MR22） | 无 | **可改进** |
| | ADS/Equip/Sprint/Jump | ✓ | ✓ | 无 | **已覆盖** |
| **TP Body** | Mesh | Manny/Quinn | 无（UE4 骨骼） | UE4 Mannequin | **缺角色外观** |
| **TP Anim** | Rifle Fire/Reload/Equip | ✓ | 无 | ✓ | **已覆盖** |
| | Pistol Locomotion | ✓ | 无 | 无 | **已覆盖（基础）** |
| | Locomotion（Idle/Walk/Jog/Run） | 基础 | 无 | 无 | **需 GASP 补充** |
| | Crouch/Jump/Fall/Land | ✓ | 无 | 无 | **可接受** |
| | Sprint/Slide | 无 | 无 | 无 | **需 GASP** |
| | Aim Offset | ✓ | 无 | ✓ | **已覆盖** |
| | Shotgun/Sniper TP | 无 | 无 | 无 | **缺** |
| **Weapon Mesh** | Rifle | ✓ | ✓（AG14W） | M4 | **已覆盖** |
| | Pistol | ✓ | ✓（X13） | 1911 | **已覆盖** |
| | Shotgun | 无 | ✓（MAK12 弹匣式） | 无 | **缺管式** |
| | Sniper | 无 | ✓（MR22） | 无 | **可接受** |
| | Modular/可动画 | 部分 | ✓（模块化部件） | 无 | **可接受** |
| **VFX** | Muzzle Flash | Cascade | Cascade | 无 | **建议 Niagara** |
| | Tracer/Impact/Casing | ✓ | 基础 | 无 | **可接受** |
| **SFX** | 枪声/换弹 | ✓ | ✓（146 Cue） | 无 | **已覆盖** |
| **Character** | 可展示角色 Mesh | Manny 灰模 | 无 | 无 | **严重缺口** |

---

## 3. 免费候选优先清单

按优先级排序，最多 8 个。这些资产可以立即领取/加入库，并在原型阶段提供实际价值。

### 候选 1：GASP Animations（Game Animation Sample Project）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | GASP Animations（Game Animation Sample Project） |
| 作者 | Epic Games |
| Fab 链接 | https://www.fab.com/listings/259f8545-f820-47b3-8fc1-e8ec5458214d |
| 价格 | **免费** |
| 许可证 | Epic Fab Standard License（Epic 自有内容） |
| 支持 UE 版本 | UE 5.4 - 5.7（向下兼容），已适配 Manny |
| 骨骼 | **UE5 Manny**（原生录制/重定向） |
| 动画数量 | **约 1,378 个** |
| FP Arms | 无 |
| TP Body | 有（Manny 全身） |
| Weapon Mesh | 无 |
| 动画语义 | Idle/Walk/Jog/Run/Sprint/Crouch/Jump/Fall/Land/Turn In Place/Slide/Dodge/Aim Offset/Blend Space |
| 特殊内容 | **滑铲（Slide）** 动作、Motion Matching、Pose Search、Smart Objects |
| VFX/SFX | 无 |
| AnimBP | 有（完整 AnimBP + Blend Space + Motion Matching 示例） |
| 接入方式 | **直接使用**（原生 Manny 骨骼） |
| 推荐理由 | Epic 官方出品，最权威的 UE5 Manny 动作参考。包含滑铲、Motion Matching 等高级特性。约 1,378 个动画覆盖了绝大部分 TP 第三人称移动需求。 |
| 风险 | UE 5.7 更新版本，需在 UE 5.8 中测试兼容性（通常向下兼容）。 |

### 候选 2：MoCap Online Free Animation Pack

| 字段 | 内容 |
| --- | --- |
| 资产名称 | MoCap Online Free Animation Pack |
| 作者 | Crispin / MoCap Online |
| Fab 链接 | https://www.fab.com/listings/64c53af0-dcb7-4483-9d65-5cbc84bd9a93 |
| 价格 | **免费** |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x（需 IK Retargeter 转 Manny，约 2 分钟） |
| 骨骼 | 自定义（附 IK Retargeter 转 UE5 Manny） |
| 动画数量 | 约 20+ |
| FP Arms | 无 |
| TP Body | 有 |
| Weapon Mesh | 无 |
| 动画语义 | Idle / Walk / Jog / Run / Crouch Walk / Crouch Idle / Transitions / Death |
| VFX/SFX | 无 |
| 接入方式 | UE5 IK Retarget（作者提供修改版 IK Retargeter，声称 2 分钟完成） |
| 推荐理由 | 专业 MoCap 质量的基础 locomotion，免费且轻量。适合快速验证 TP 角色移动。 |
| 风险 | 动画数量有限，仅覆盖基础移动，无武器姿态。 |

### 候选 3：Paragon 动画重定向到 Manny（社区包）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Paragon Animations Retargeted to Manny |
| 作者 | Kingboars（社区）/ Epic Games（原始动画） |
| Fab 链接 | 不在 Fab（社区发布，Epic 论坛: https://forums.unrealengine.com/t/2656824） |
| 价格 | **免费** |
| 许可证 | Epic 拥有原始动画版权，社区重定向为便利分发 |
| 支持 UE 版本 | UE 5.x（Manny 骨骼，直接使用） |
| 骨骼 | **UE5 Manny**（已重定向） |
| 动画数量 | **约 5,300 个** |
| FP Arms | 无 |
| TP Body | 有（Manny 全身） |
| Weapon Mesh | 无 |
| 动画语义 | 来自 Paragon 英雄：全面的战斗、移动、技能、受击、死亡等动画。覆盖 Idle/Walk/Jog/Run/Sprint 多种风格。 |
| 接入方式 | **直接使用**（已重定向到 Manny） |
| 推荐理由 | 庞大的免费动画库，出自 Epic 原版 Paragon 资产。5,300 个动画几乎可以覆盖任何 TP 动作原型需求。 |
| 风险 | 不在 Fab 官方渠道，需从社区下载；动画风格偏向 MOBA 英雄，与写实军事射击有风格差异；部分 Root Motion/Additive 需验证。 |

### 候选 4：Free FPS Template & Tutorial

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Free FPS Template & Tutorial |
| Fab 链接 | https://www.fab.com/listings/6a0af880-2b74-480c-a82c-8e597918dffe |
| 价格 | **免费** |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x |
| 骨骼 | UE5 Mannequin Rig（FP 动画用）+ 自定义 FP Arms Rig |
| FP Arms | 有（含 Blender 源文件） |
| TP Body | 无 |
| Weapon Mesh | 有（1 把已绑定枪模，含 Blender 源文件） |
| 动画语义（FP） | Idle / Aim / Walk / Run / Holster / Equip / Fire / Reload |
| 动画语义（TP） | 无 |
| VFX/SFX | 无（纯动画模板） |
| 接入方式 | 可参考其 FP 动画结构和 Blender 源文件；FP Arms 可能需要适配项目风格 |
| 推荐理由 | 免费且含 Blender 源文件，可作为 FP 动画制作/修改的参考基线和学习资源。 |
| 风险 | 仅为教学模板，动画数量和武器种类有限；不可直接用于成品。 |

### 候选 5：Basic Survival Animation Pack

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Basic Survival Animation Pack |
| Fab 链接 | https://www.fab.com/listings/1be7d473-b2e0-4d46-b149-2f51ef0d7225 |
| 价格 | **未确认**（搜索结果未显示价格；可能是免费或低价） |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x（2025-02-10 更新重定向到 Manny） |
| 骨骼 | UE5 Manny（已重定向） |
| 动画数量 | **583 个**（含 2025-02-10 新增 228 个） |
| FP Arms | 无 |
| TP Body | 有 |
| Weapon Mesh | 无 |
| 动画语义 | Idle(6) / Jump(18) / Locomotion(240) / Combat / Climbing / Crouching(126) / Crawling / Dodging / Swimming / Slide(6) / Roll(18) / Climb Rope(22) / Climb Stair(14) |
| 接入方式 | **直接使用**（已重定向到 Manny） |
| 推荐理由 | 数量可观（583），覆盖生存/战斗类动作，含滑铲、匍匐、攀爬等特殊动作。 |
| 风险 | **价格未确认**——可能不是免费；需在 Fab 页面核实。 |

### 候选 6：HEAT Beta Plugin

| 字段 | 内容 |
| --- | --- |
| 资产名称 | HEAT Beta plug-in |
| Fab 链接 | https://www.fab.com/listings/fc33e19e-9d7c-49eb-8d84-2e859f0bb41c |
| 价格 | **免费**（Beta 插件） |
| 许可证 | Fab Standard License（Beta） |
| 支持 UE 版本 | UE 5.x |
| 骨骼 | Manny / MetaHuman / 自定义 |
| 动画数量 | **1,000+ AAA 动画** |
| FP Arms | 无 |
| TP Body | 有 |
| 接入方式 | 插件拖放式集成；直接在编辑器内浏览和添加动画 |
| 推荐理由 | 1,000+ AAA 质量动画免费使用，拖放式工作流，支持 Manny 和 MetaHuman。 |
| 风险 | **Beta 插件**，稳定性未经验证；动画语义清单不明；需实际测试与 UE 5.8 的兼容性。 |

### 候选 7：UE 5.7 刚更新 1398 个免费动画包（B站参考）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | 同候选 1（GASP）——B 站视频提供了额外的中国社区视角和使用演示 |
| 说明 | 此条本质上是候选 1（GASP）的社区分发和中文使用介绍，**不是独立的新资产** |

### 候选 8：Animation Starter Pack（Epic 官方推荐）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Animation Starter Pack |
| Fab 链接 | 未找到独立 Fab 页面（在 UE 5.6 文档中作为官方推荐被引用） |
| 价格 | **免费**（Epic 官方推荐） |
| 说明 | UE 5.6 文档中推荐的 Manny 基础 locomotion 入门动画包。具体动画数量和语义需在 Fab 中确认。 |
| 风险 | **信息不足**——未找到独立商品页。可能已合并到 GASP 或其他官方包中。 |

---

## 4. 付费高匹配候选

最多 5 个。每个必须说明比现有资产多解决了什么、为什么值得花钱。

### 付费候选 1：FPS Animation Pack（KINEMATION）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | FPS Animation Pack |
| 作者 | KINEMATION |
| Fab 链接 | https://www.fab.com/listings/a2d0dc02-0380-4fb8-84a9-d9e03636aec9 |
| 价格 | **Lite $24.99 / Pro $49.99 / Ultimate $99.99** |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.3 - 5.7（2025-08-02 更新） |
| 骨骼 | UE4 Mannequin（UE5 Manny 通过 Retargeter 支持，仅 5.4+） |
| FP Arms | 有（含程序化动画系统） |
| TP Body | 无（纯 FP 包） |
| Weapon Mesh | 有（低多边形武器模型：Lite 4 把 / Pro 10 把 / Ultimate 20 把） |
| 武器覆盖 | **Ultimate**: M1911/Kolibri/DGL-50/X18/Viper-357（手枪）、MPS5/Striker-V（SMG）、ASVal/MX16A4/AKX200/G3（步枪）、KXG12/Drake-12（霰弹枪）、Kar98k/L96X（狙击）、Mk14EBR/SVD（精确步枪）、MGX5（机枪）、RPG7（火箭筒） |
| 动画语义（FP） | Fire / Reload（Tactical/Empty/Start-Loop-End） / Equip / Unequip / Aim / Idle / Walk / Sprint / Jump / Grenade Throw / Melee / Procedural Recoil & Sway |
| 霰弹枪装填 | **有**——KXG12 使用 Start-Loop-End 三段式模拟管式逐发装填 |
| 狙击枪 | **有**——Kar98k 拉栓 + L96X |
| VFX/SFX | 有（枪声、程序化后坐力） |
| 接入方式 | FP Arms 保持独立骨骼（UE4 Mannequin），不与 TP Manny 混合；武器 Mesh 可用于 FP 和 TP |
| **为什么值得花钱** | 对比 Modern Guns：武器种类更多（20 vs 8），动画更现代（程序化系统 + 2025 更新），有明确的管式霰弹枪和拉栓狙击动画。对比 Stephen_FPS：武器覆盖远超 Stephen 的步枪+手枪。Lite 版 $24.99 即可获得 4 把代表性武器的完整 FP 动画。 |
| 与现有资产重合度 | 与 Modern Guns 高度重合（武器类型和 FP 动画）。如果 Modern Guns 的 MAK12 霰弹枪和 MR22 狙击枪动画已满足需求，则此包价值降低。 |

### 付费候选 2：Tactical Pump Shotgun（管式霰弹枪专用）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Tactical Pump Shotgun |
| Fab 链接 | https://www.fab.com/listings/095f67f7-ff21-4429-9135-eb27f8c3b5df |
| 价格 | **未确认**（需在 Fab 页面查看） |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x（2025 年 6 月更新） |
| 骨骼 | 自定义 FP Arms |
| FP Arms | 有（3 种左手握持位置变体） |
| TP Body | 无 |
| Weapon Mesh | 有（**模块化**泵动霰弹枪，可拆卸部件） |
| 动画语义（FP） | Fire / Pump Action / Reload Start（含 chamber load 变体） / **Reload Loop Single Caliber / Double Caliber** / Reload Stop Empty / Reload Stop Loaded / Load Chamber / Check Ammo / Check Chamber / Check Magazine |
| 管式逐发装填 | **有——核心特色**：逐发插入弹仓的 Start→Loop→End 完整序列 |
| VFX/SFX | 未确认 |
| **为什么值得花钱** | 这是本次调研发现的**最精准匹配管式霰弹枪逐发装填**的单一资产。动画覆盖逐发插入、打断装填后开火、检查弹仓和膛内等关键语义。模块化武器 Mesh 可直接作为 TP 武器表现的基础。 |
| 风险 | **价格未确认**；FP Arms 骨骼需评估与现有系统的兼容性；不含 TP 动作。 |

### 付费候选 3：Kar 98 Animation Pack（拉栓狙击枪专用）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | Kar 98 Animation Pack |
| Fab 链接 | https://www.fab.com/listings/57eaacef-eff0-4438-a4f1-b362b927f50a |
| 价格 | **未确认**（需在 Fab 页面查看） |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x |
| 骨骼 | 自定义 FP Arms |
| FP Arms | 有 |
| TP Body | 无 |
| Weapon Mesh | 有（Kar98k） |
| 动画语义（FP） | Hip Fire / ADS Fire / **Bolt Action** / Reload / Melee / Inspect / Idle / Walk / Run / Jump |
| 特殊内容 | Niagara Muzzle Flash 粒子系统 / 射击音效 / 相机调整 |
| **为什么值得花钱** | 专注拉栓狙击动画，比 FPS Animation Pack 的 Kar98k 动画更深入（含 ADS、拉栓细节、Niagara 闪口）。如果狙击枪是 Phase 2 的第二把验证武器，这是最佳 FP 动画来源。 |
| 风险 | **价格未确认**；不含 TP 动画；需要额外匹配 TP 拉栓表现。 |

### 付费候选 4：US Soldier | Modular Low-Poly Military Character

| 字段 | 内容 |
| --- | --- |
| 资产名称 | US Soldier | Modular Low-Poly Military Character |
| Fab 链接 | https://www.fab.com/listings/fad00839-f2ce-4f36-8724-8823fcaf0565 |
| 价格 | **未确认**（需在 Fab 页面查看） |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x |
| 骨骼 | **UE5 Manny 原生绑定**（含 IK 骨骼），兼容 UE5 Control Rig |
| 模块化 | **14 个可拆卸模块**（头盔、背心、护甲、装备等） |
| 风格 | Low-Poly 军事写实，适合战术射击/俯视策略/移动端 |
| 动画 | 不包含独立动画；直接使用所有 Epic Skeleton 兼容动画（无需重定向） |
| LOD | 有（优化低多边形，适合多角色同屏） |
| **为什么值得花钱** | 这是最直接的"即插即用"Manny 兼容角色。原生 Manny 骨骼意味着项目所有 TP 动画都可以直接在此角色上播放，无需任何重定向。模块化设计支持为不同英雄/职业创建外观变体。Low-poly 风格适合原型阶段性能。 |
| 风险 | **价格未确认**；Low-poly 风格可能不适合追求高质量作品展示的最终效果；需要额外材质工作提升近景质量。 |

### 付费候选 5：MC Core Motion Animation Pack（TP Locomotion 专业方案）

| 字段 | 内容 |
| --- | --- |
| 资产名称 | MC Core Motion Animation Pack |
| 作者 | MoCap Central |
| Fab 链接 | https://www.fab.com/listings/9657e6b9-a480-43a8-a0f0-55abd806ce85 |
| 价格 | **未确认**（需在 Fab 页面查看） |
| 许可证 | Fab Standard License |
| 支持 UE 版本 | UE 5.x |
| 骨骼 | **UE5 Manny**（原生）+ 可重定向到 MetaHuman |
| 动画数量 | **599 个 MoCap 动画**（30+ 组，男女双版本） |
| FP Arms | 无 |
| TP Body | 有（Manny 全身） |
| Weapon Mesh | 无 |
| 动画语义 | 两套 Locomotion 系统——V1: 8 向 Strafe（Walk/Jog/Crouch）；V2: 带 Start/Stop/Pivot/Diagonal 的方向移动（Walk/Jog/Run/RunFast/Crouch）；Standing/Crouch Idles + Fidgets；45/90/135/180 度转身；Jump；Pickup；Conversation/Interaction |
| 特色 | 专业 MoCap 工作室录制，手工清理；NoRM（无 Root Motion）版本；与 MoCap Central 全系列兼容（共享进入/退出姿态） |
| **为什么值得花钱** | 如果 GASP 的动作风格不够写实，MC Core Motion 提供了专业 MoCap 质量的替代方案。男女双版本对英雄射击的多样化角色很重要。599 个动画 + 两套 Locomotion 系统提供了比 GASP 更精细的移动表现。 |
| 风险 | **价格未确认**；不含任何武器姿态（是纯 body locomotion）；需额外购买武器姿态包才能在手持武器时使用。 |

---

## 5. 不推荐或暂缓候选

| 候选 | 原因 | 判断 |
| --- | --- | --- |
| **Pistol Mega MocapAnimPack**（RIB Studio） | 300+ TP 手枪动画，质量高但价格约 $705，远超原型阶段预算 | 暂缓——手枪在 Phase 3+ 才需要 |
| **Tactical Shooter Kit** | 声称 FP+TP 支持、193 动画，但搜索未找到精确 Fab 页；信息不足以核实 | 无法核验 |
| **FPS Multiplayer Controller** | 声称 400+ FP/TP 动画 + 整套蓝图系统，但更偏向卖蓝图系统而非美术资产；C++ 项目可能大量不兼容 | 暂缓——蓝图逻辑不是购买重点 |
| **Rifle ShootReload MocapAnimPack** | 含霰弹枪和双管霰弹枪动画，但仅 42 个 FBX 动画、UE 4.27-5.4、质量未知；与已有 Stephen/Rifle Pro 重合度高 | 重复度高 |
| **Cover Rifle MocapAnimPack** | 288 个 FBX 动画专注掩体射击，偏向特定玩法且与已有 TP 步枪动作重合 | 暂缓——掩体射击不是当前原型需求 |
| **Modular Military Character**（Quantum Assets） | 400+ 模块、10.6 GB，功能最全但体积过大；价格未知；UE 5.0-5.4 非最新 | 暂缓——体积和版本风险 |
| **Asian Girl Fantasy Character** | 高质量但风格偏向东方奇幻，与军事/近未来射击设定不匹配 | 风格不匹配 |
| **PROCEDURAL/BODYCAM FPS KIT** | 程序化动画系统有趣，但主要是蓝图框架；素材不可拆分；强依赖其封闭玩法框架 | 素材不可拆分 |
| **100 Muzzle Flashes (Niagara)** | 100 个闪口效果很全面，但 Stephen_FPS 已有基础 VFX；在原型阶段属于锦上添花 | 暂缓——Phase 2 不阻塞 |
| **Muzzle-flashes vfx pack** | 15 个效果、UE 5.2-5.4 版本偏旧 | 暂缓 |
| **Stylized Shooting VFX Remaster** | 45+ Niagara FX 覆盖全面，但风格化（Overwatch/Valorant 风格）可能与写实武器不协调 | 风格需评估 |

---

## 6. 推荐的拼装方案

### 方案 A：零新增付费（完全依靠现有资产 + 免费补充）

| 层 | 资产来源 | 覆盖内容 |
| --- | --- | --- |
| **FP Arms + FP 动画** | Stephen_FPS（步枪/手枪）+ Modern Guns Pack（霰弹枪/狙击枪/更多武器） | 步枪、手枪、弹匣式霰弹枪、狙击枪的完整 FP 动画 |
| **TP Body 动画** | Stephen_FPS（基础 TP）+ **GASP Animations（免费）** + **MoCap Online Free（免费）** + **Paragon 社区包（免费）** | 完整的 Idle/Walk/Jog/Run/Sprint/Crouch/Jump/Slide locomotion + 基础武器姿态 |
| **TP 角色 Mesh** | **UE5 Manny/Quinn 灰模**（引擎自带） | 原型验证用，不具备展示质量 |
| **Weapon Mesh** | Stephen_FPS + Modern Guns Pack | 步枪、手枪、霰弹枪、狙击枪的 FP/TP 武器模型 |
| **VFX/SFX** | Stephen_FPS（Cascade VFX + SFX）+ Modern Guns（SFX） | 基础枪口、曳光、弹壳、命中 + 枪声 |

**主要工作**：
1. 从 GASP 迁入 TP 移动动画（原生 Manny，直接使用）。
2. 将 Stephen_FPS 的 TP 步枪动画与 GASP locomotion 集成（AnimLayer/BlandSpace）。
3. 霰弹枪管式逐发装填暂时使用 Modern Guns MAK12 的弹匣式装填替代，或从 FPS Animation Pack Lite 购买。
4. 角色展示暂用 Manny 灰模；后续可以购买付费角色替换。

### 方案 B：最小购买（只买一个最能补关键缺口的资产）

| 层 | 资产来源 | 覆盖内容 |
| --- | --- | --- |
| **FP 霰弹枪装填** | **购买：Tactical Pump Shotgun**（价格未确认） 或 **FPS Animation Pack Lite（$24.99）** | 管式霰弹枪逐发装填完整 FP 动画 |
| **其他 FP 动画** | 同方案 A（Stephen_FPS + Modern Guns） | 步枪、手枪、狙击枪 FP |
| **TP Body 动画** | 同方案 A（Stephen + GASP 免费 + MoCap Free + Paragon） | 完整 locomotion + 武器姿态 |
| **TP 角色 Mesh** | UE5 Manny/Quinn 灰模 | 原型验证 |
| **Weapon Mesh** | 同方案 A | 全部武器模型 |
| **VFX/SFX** | 同方案 A | 基础表现 |

**主要工作**：同方案 A + 将购买的霰弹枪 FP 动画集成到项目的武器 Ability 系统。

**推荐购买**：**FPS Animation Pack Lite（$24.99）**——最便宜的选项即可获得 M1911 手枪、MX16A4 步枪、Drake-12 霰弹枪、Kar98k 狙击枪的完整 FP 动画（含管式霰弹枪 Start-Loop-End 装填和拉栓狙击动画）。

### 方案 C：更完整方案（允许少量付费，覆盖双视角和第二武器家族）

| 层 | 资产来源 | 覆盖内容 |
| --- | --- | --- |
| **FP Arms + 动画** | Stephen_FPS（步枪/手枪基础）+ **FPS Animation Pack Pro（$49.99）** | 步枪、手枪、霰弹枪（管式+弹匣式）、狙击枪（拉栓）、SMG、精确步枪的完整 FP 动画（10 武器 + 程序化系统） |
| **TP Body 动画** | Stephen_FPS（基础 TP 武器动画）+ **GASP（免费）** + MoCap Online Free（免费）+ 可选 **MC Core Motion（付费）** | 完整的 locomotion 两套系统 + 武器姿态族 |
| **TP 角色 Mesh** | **购买：US Soldier Modular（价格未确认）** | Manny 原生绑定、模块化、可直接播放所有 TP 动画的军事角色 |
| **Weapon Mesh** | Stephen_FPS + Modern Guns + FPS Animation Pack 自带低模 | 完整的武器模型库（15+ 武器） |
| **VFX/SFX** | Stephen_FPS + Modern Guns + 可选 **100 Muzzle Flashes Niagara** | 全面 VFX 覆盖 |
| **狙击专项** | 可选 **Kar 98 Animation Pack** | 专用拉栓狙击动画 |

**总预算估算**：约 $75 - $150 + 角色价格（未确认）

**主要工作**：
1. 将 FPS Animation Pack 的 FP 动画与 GASP 的 TP locomotion 集成。
2. 将 US Soldier 角色 Mesh 替换 Manny 灰模，验证所有 TP 动画在此角色上的表现。
3. 建立 Rifle/Pistol/Shotgun/Sniper 四个武器家族的 FP+TP 动画对应关系。
4. 整合 Niagara VFX 替换旧的 Cascade 特效。

---

## 7. 最终行动建议

### 现在可免费领取/加入库

| 资产 | 链接 | 用途 |
| --- | --- | --- |
| **GASP Animations** | https://www.fab.com/listings/259f8545-f820-47b3-8fc1-e8ec5458214d | TP Manny locomotion 主库（~1,378 动画，含滑铲） |
| **MoCap Online Free Animation Pack** | https://www.fab.com/listings/64c53af0-dcb7-4483-9d65-5cbc84bd9a93 | 补充基础 locomotion（MoCap 质量） |
| **Free FPS Template & Tutorial** | https://www.fab.com/listings/6a0af880-2b74-480c-a82c-8e597918dffe | FP 动画参考和 Blender 源文件学习 |
| **HEAT Beta Plugin** | https://www.fab.com/listings/fc33e19e-9d7c-49eb-8d84-2e859f0bb41c | 拖放式 AAA 动画库（Beta，需测试兼容性） |
| **Paragon 动画重定向** | Epic 论坛社区 | 5,300 免费 TP 动画备用库 |

### Phase 2 前建议购买

| 资产 | 预估价格 | 理由 |
| --- | --- | --- |
| **FPS Animation Pack Lite** | **$24.99** | 最小代价补全霰弹枪管式装填 + 拉栓狙击 FP 动画。4 把代表性武器覆盖了首批核心武器类型。若确认 Modern Guns 的 MAK12/MR22 动画已满足需求，可跳过。 |

### 等实际缺口出现再购买

| 资产 | 触发条件 |
| --- | --- |
| **US Soldier Modular Character**（或同类 Manny 角色） | Phase 2 中需要向他人展示 TP 角色表现，Manny 灰模不再够用时 |
| **MC Core Motion Animation Pack** | GASP 动画的风格/质量经实际验证不能满足展示需求时 |
| **Kar 98 Animation Pack** | 狙击枪被选为第二把验证武器，且 FPS Animation Pack 的 Kar98k 动画精细度不足时 |
| **Tactical Pump Shotgun** | 需要更精细的管式霰弹枪模块化 Mesh 和专用动画时 |
| **100 Muzzle Flashes (Niagara)** | Stephen_FPS 的 Cascade VFX 在 UE 5.8 中表现不佳或需要升级时 |

### 无需购买，现有资产可解决

- **步枪 FP/TP 动画**：Stephen_FPS 已完整覆盖
- **手枪 FP/TP 动画**：Stephen_FPS + Modern Guns 已覆盖
- **通用 SFX**：Stephen_FPS + Modern Guns 的 WAV/Cue 已覆盖
- **TP 基础 Locomotion**：GASP（免费）已覆盖
- **Aim Offset/Blend Space**：Stephen_FPS 已覆盖
- **HUD 素材**：Stephen_FPS 已覆盖

---

## 8. 调研统计

| 指标 | 数值 |
| --- | --- |
| 调研候选总数 | **23 个**（含 Fab 直接候选 + 社区/参考候选） |
| 免费推荐数 | **6 个**（GASP、MoCap Online Free、Paragon 社区包、Free FPS Template、HEAT Beta、Animation Starter Pack） |
| 付费推荐数 | **5 个**（FPS Animation Pack、Tactical Pump Shotgun、Kar 98 Pack、US Soldier、MC Core Motion） |
| 不推荐/暂缓数 | **12 个** |
| 拼装方案 | **3 套**（零付费 / 最小购买 $24.99 / 更完整 ~$75-150） |

### 最重要的三个结论

1. **Phase 2 不阻塞**：现有 Stephen_FPS + Modern Guns 已经可以启动第一把步枪的 FP/TP 纵向切片。关键缺口（霰弹枪管式装填、TP locomotion 补充）都有明确、廉价的付费或免费解决方案。

2. **最值得花的 $24.99**：FPS Animation Pack Lite 是性价比最高的单一购买——同时补全霰弹枪管式装填（Drake-12 Start-Loop-End）、拉栓狙击（Kar98k）和手枪（M1911）的现代程序化 FP 动画，直接解锁第二武器家族的动画底座。

3. **角色外观是隐形最大缺口**：Manny 灰模不能用于作品展示。US Soldier Modular 提供了最直接的 Manny 原生绑定解决方案，但价格未确认。建议在 Phase 2 首次内部演示前确定角色资产策略。

---

## 9. 未确认事项与风险汇总

| 事项 | 影响 | 建议 |
| --- | --- | --- |
| **多个付费资产价格未确认** | 无法精确预算 | 用户需在 Fab 页面逐一核验实际价格 |
| **Fab 页面直接访问受限** | 本报告所有信息来自搜索引擎结果和第三方页面 | 用户在最终购买决策前应自行查看 Fab 商品页确认内容 |
| **UE 5.8 兼容性未验证** | 多数资产标注 5.3-5.7，未测试 5.8 | 迁入前在隔离测试项目中验证 |
| **FP Arms 骨骼兼容性** | KINEMATION 和 Tactical Pump Shotgun 使用独立 FP 骨骼 | 需评估与项目 FP Arms 系统的集成方式 |
| **GASP UE 5.7 → 5.8 兼容** | GASP 包含 Motion Matching 和 Pose Search，依赖特定引擎版本 | 在 5.8 中测试；若 Motion Matching API 变化，动画数据本身仍可用 |
| **Paragon 社区包合法性** | 社区重定向分发，非 Epic 官方渠道 | 确认 Epic 的原始资产 EULA 允许此类使用 |
| **价格时效性** | Fab 价格和促销活动频繁变动 | 本报告价格仅反映 2026-08-06 搜索结果 |
