# Infima Games 全产品调研

调研日期：2026-08-12  
调研方：Apecox 外部资产调研代理  
状态：**完成。等待用户/Codex 审查，未执行购买。**

## 执行摘要

Infima Games 当前在 Fab/UE Marketplace 共有 **约 6 个可识别的产品/产品线**（含 Bundle 和单品）。对 Apecox 最有价值的资产是 **Tactical FPS Animation Packs 系列**——这是唯一确认 UE5 Manny 骨架兼容、同时提供 FP 和 TP 动画、以美术素材为主的独立武器包。其他产品（Low Poly Shooter Pack、LPAMG、Modern Guns Bundle）各有价值，但骨架兼容性、框架耦合度或内容聚焦度上存在显著折中。

**最高优先级推荐**：Tactical FPS Shooter Pack（7 武器 Bundle）或单个 Tactical FPS Animation Pack - Assault Rifle，直接补足 Apecox 当前最缺的第一人称步枪开火/换弹表现。

**仍需人工确认**：各产品具体价格、Tactical FPS Animation Packs 除 Assault Rifle 和 Pistol 外其他 5 把武器的确切列表、Modern Guns Bundle 与 LPAMG 武器模型的互换性、各产品当前 Fab 页面实际折扣。

---

## 调研范围与来源

### 已阅读的项目上下文

| 文件 | 状态 |
| --- | --- |
| `Agent/README.md` | 已读 |
| `.agents/ue-project-context.md` | 已读 |
| `Agent/Research_Notes/2026-08-12_Phase2B2_Presentation_HUD_Deferred_Proposal.md` | 已读 |
| `Agent/90_References/External/2026-08-10_Rifle_Pro_MoCap_Asset_Inventory.md` | 已读 |
| `Agent/01_Project/Apecox_Project_Charter.md` | 不存在于当前工作区 |

### 信息来源

| 优先级 | 来源 | 访问状态 |
| --- | --- | --- |
| 1 | Infima Games 官方文档站 `docs.infimagames.com` | ✅ 完整访问 |
| 2 | Infima Games Gumroad 商店 `infimagames.gumroad.com` | ❌ 连接被拒（ECONNREFUSED） |
| 3 | Fab.com 商品页 | ❌ HTTP 403（需客户端 JS 渲染） |
| 4 | 网页搜索结果（Google/Bing 索引） | ✅ 用于补充和交叉验证 |
| 5 | 第三方站点（PSDly、GameAssetDeals 等） | ⚠️ 仅作参考，非官方来源 |

**重要声明**：Fab.com 商品页无法通过 WebFetch 直接获取内容（动态 JS 渲染 + 403）。以下标注"未从官方来源核实"的字段来源于搜索结果摘要、文档交叉引用和第三方站点，可能在用户打开官方页面时与实际状态不同。

---

## 完整产品目录

盘点范围：Infima Games 在 Fab/UE Marketplace 和 Gumroad 上的全部可识别产品（含已迁移、更名、Bundle 和免费样品）。以下产品均从 Infima Games 官方文档站 `docs.infimagames.com` 的产品结构确认存在，Fab URL 来自搜索结果中的直接链接。

文档站列出的产品线为 4 个：Low Poly Shooter Pack、Low Poly Animated - Modern Guns Pack、Rigged Gun Model Packs、Tactical FPS Animation Packs。加上独立的 Modern Revolver 和两个 Bundle，共 **6 个可识别的产品/产品线条目**。

### 产品 1：Low Poly Shooter Pack (LPSP)

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Low Poly Shooter Pack（v6.0 当前最新） |
| 官方 URL | Fab: `https://www.fab.com/listings/90ba076a-dc9a-4782-9ac8-dc2ed4f06405`（未从官方来源核实）；Gumroad: `https://infimagames.gumroad.com/l/low-poly-shooter-pack`；旧 Marketplace: `https://www.unrealengine.com/marketplace/en-US/product/low-poly-fps-pack` |
| 当前状态 | 在售。最新更新 2025 年 11 月（v6.0, UE 5.4+）。另有免费样品版 `Low Poly Shooter Pack - Free Sample` |
| 产品类别 | 框架/模板（完整 FPS 游戏框架） |
| 官方支持的 UE 版本 | UE 4.26 – 5.4+。v6.0 明确支持 5.4+。**未明确声明 UE 5.8 支持** |
| 主要内容 | 完整低多边形 FPS 模板：18+ 武器模型、240+ 武器材质、170+ 角色材质、AI 敌人（行为树）、多人复制、武器拾取/库存/切换/丢弃、生命/伤害/死亡/重生、第一/第三人称一键切换、VFX、UI/HUD。文档覆盖 AI、动画、角色、伤害系统、游戏模式、生命、库存、交互、存档、表面材质、武器、UI 组件 |
| 依赖关系 | 无外部依赖。内部文件夹 `AnimatedLowPolyWeapons` 是子内容，非独立产品 |
| 重复关系 | 其武器模型和动画与 LPAMG 有重叠（同一作者），但 LPSP 是完整框架，LPAMG 是纯美术包。功能层面与 Apecox 自身的 GAS/武器框架严重冲突 |
| 许可证信息 | Fab 标准许可（Professional 层级）。未从官方来源核实具体层级。Gumroad 另售 |
| Apecox 相关性 | **低**。是完整框架而非素材包；大量 BP/C++ 系统（武器、库存、AI、多人）与 Apecox 已有 GAS 实现冲突。唯一可能价值为其低多边形武器模型和动画，但 LPAMG 和 Tactical FPS Packs 提供了更专注的美术素材 |

### 产品 2：Low Poly Animated - Modern Guns Pack (LPAMG)

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Low Poly Animated - Modern Guns Pack |
| 官方 URL | Fab: `https://www.fab.com/listings/6914a185-4b14-475c-8f4f-cc0a6a3c589f`（未从官方来源核实） |
| 当前状态 | 在售。持续更新（changelog 从 v0.2.0 到 v1.0.0+） |
| 产品类别 | 武器 Mesh + FP 动画美术包 |
| 官方支持的 UE 版本 | 未从官方来源核实具体版本。兼容 UE4 Mannequin 骨架 |
| 主要内容 | 8 把低多边形武器（Skeletal Mesh，可互换部件）：SP60 (SMG)、X13 (Pistol)、MR22 (Sniper)、RC425 (Revolver)、MAK12 (Shotgun)、LRAF9 (Rocket Launcher)、AG14W (Assault Rifle)、HVG7 (LMG)。每把武器含 FP 动画（换弹/空仓换弹/瞄准换弹/开火/瞄准开火/检视/收枪/冲刺/行走/刀攻击/手雷投掷）。2 套第一人称手臂 Mesh（基础 + 手套）。模块化配件（握把、红点、消音器、激光、双脚架）。16+ 可定制武器材质。Demo Blueprint |
| 依赖关系 | 可选与 LPSP 集成（通过 Discord 获取集成方案） |
| 重复关系 | 与 LPSP 内 `AnimatedLowPolyWeapons` 内容高度重叠。与 Tactical FPS Animation Packs 无重叠（低多边形 vs 写实风格） |
| 许可证信息 | Fab 标准许可。未从官方来源核实 |
| Apecox 相关性 | **中**。提供完整的 FP 武器动画素材，但存在两个关键问题：(1) 骨架为 UE4 Mannequin，需手动重定向到 UE5 Manny；(2) 无第三人称动画。适合作为低多边形风格选项或 FP 动画参考 |

### 产品 3：Modern Guns Bundle（Rigged Gun Model Packs 的 Bundle）

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Modern Guns Bundle - Rigged & Game Ready Models |
| 官方 URL | Fab: `https://www.fab.com/listings/eb4258b3-cec4-4995-85fe-ace8c9b5d45f` |
| 当前状态 | 在售。最新更新 2026 年 6 月 27 日（5.3 GB） |
| 产品类别 | 武器 Mesh（写实高质量模型，非低多边形） |
| 官方支持的 UE 版本 | **UE 5.0 – 5.8（明确标注）** |
| 主要内容 | 8 把高质量写实武器模型组合 Bundle（从独立单枪包合并）。每把武器含干净拓扑、4K PBR 纹理、Blender 源文件（完整 Rig + 预设 Idle 姿态）。含第一/第三人称示例设置、Demo 地图和示例 Blueprint。**注意：不含动画**——仅为 Rigged 模型 |
| 依赖关系 | 单枪可独立购买（作为 Rigged Gun Model Packs 系列的子产品）。文档含 Blender 导出和 UE5 FPS 动画教程 |
| 重复关系 | 武器代码名与 LPAMG 相同（AG14W, HVG7 等），但这是写实版模型，LPAMG 是低多边形版。两者是不同的产品线 |
| 许可证信息 | Fab 标准许可。未从官方来源核实 |
| Apecox 相关性 | **中**。高质量写实武器 Mesh 适合替换当前占位步枪模型。UE 5.8 明确兼容。但**不含动画**——需配合 Tactical FPS Animation Packs 或自行制作动画。Blender 源文件有利于自定义 Socket 和配件 |

### 产品 4：Tactical FPS Animation Packs（独立武器动画包系列）

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Tactical FPS Animation Packs（系列名）。独立包如 "Tactical FPS Animations Pack - Assault Rifle"、"Tactical FPS Animations Pack - Pistol" |
| 官方 URL | 未从官方来源核实各包独立 Fab URL。已知 Bundle URL: `https://www.fab.com/listings/76c5f2b0-ea55-461c-8b3c-811a9deb3279` |
| 当前状态 | 在售。按武器独立销售 + 7 武器 Bundle |
| 产品类别 | FP+TP 武器动画美术包（写实风格） |
| 官方支持的 UE 版本 | **UE 5.4+（官文明确要求）**。未明确声明 UE 5.8 支持，但 UE 5.4+ 通常可向上兼容 |
| 主要内容 | 每武器包含：约 123 个动画资产（FP 角色 61 + TP 角色 34 + 武器 24 + 环境 4）。覆盖：开火、换弹（标准/瞄准/快速/空仓）、检视、近战、手雷、卡壳排除、弹匣检查、射击模式切换、持枪 Idle/行走/跑动/冲刺/跳跃/下蹲（仅 FP Locomotion）、收枪/出枪、瞄准偏移。含武器 Mesh、材质、纹理、音频、VFX。Demo 地图和 Blueprint。Blender 武器 Rig 源文件 |
| 依赖关系 | 无外部依赖。各武器包独立，共享 `InfimaGames/TacticalFPSAnimations/Common/` 核心系统。Demo Blueprint 不依赖外部框架 |
| 重复关系 | 与 LPAMG 和 LPSP 内容不重复（不同风格/骨架）。Bundle（Tactical FPS Shooter Pack）是 7 个单品的集合 |
| 许可证信息 | Fab 标准许可。未从官方来源核实 |
| Apecox 相关性 | **高**。唯一确认 UE5 Manny 骨架兼容（使用 SKEL_TFA_Mannequin，基于 Manny 层级）的 Infima 产品线。同时提供 FP 和 TP 动画。以美术素材为主，Demo BP 仅作参考。动画为原地动画（无 Root Motion），适合与 Apecox CMC 驱动的角色移动配合。可直接分配 Manny Mesh 到 TFA Skeleton |

### 产品 5：Tactical FPS Shooter Pack（TFA Bundle）

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Tactical FPS Shooter Pack |
| 官方 URL | Fab: `https://www.fab.com/listings/76c5f2b0-ea55-461c-8b3c-811a9deb3279` |
| 当前状态 | 在售 |
| 产品类别 | Bundle（集合 7 个独立 Tactical FPS Animation Packs） |
| 官方支持的 UE 版本 | UE 5.4+（继承 TFA 各包要求） |
| 主要内容 | 7 个独立武器动画包的全部内容：模型、声音、VFX、程序化动画、全身角色支持。官方描述为"true FPS animations"、FPS+TPS 双视角 |
| 依赖关系 | 是 7 个 TFA 单品的 Bundle。购买后不再需要单独购买单品 |
| 重复关系 | 与 TFA 单品 100% 重叠——**不要同时购买 Bundle 和单品** |
| 许可证信息 | Fab 标准许可。未从官方来源核实 |
| Apecox 相关性 | **高**。如果 7 武器中覆盖了步枪、手枪、狙击枪和霰弹枪，是性价比最高的入口 |

### 产品 6：Modern Revolver - Rigged & Game Ready Model

| 字段 | 内容 |
| --- | --- |
| 产品名称 | Modern Revolver - Rigged & Game Ready Model for Unreal Engine 5 |
| 官方 URL | Gumroad: `https://infimagames.gumroad.com/l/modern-revolver-rigged-ue5`；同时声明 Fab 有售 |
| 当前状态 | 在售 |
| 产品类别 | 单个写实武器 Mesh |
| 官方支持的 UE 版本 | UE 5.0 – 5.5（Gumroad 标注）。未明确 UE 5.8 |
| 主要内容 | 单把高质量左轮手枪模型：模块化配件（瞄准镜、握把、激光、消音器、快速装弹器）、3 种枪管长度、示例 Blueprint、Blender Rig 源文件。**不含动画** |
| 依赖关系 | 无 |
| 重复关系 | 可能已含在 Modern Guns Bundle 的 8 武器中（RC425 Revolver）。需人工确认是否同一模型 |
| 许可证信息 | Gumroad/Fab 各自许可。未从官方来源核实 |
| Apecox 相关性 | **低**。单把武器模型无动画。如已在 Modern Guns Bundle 中则重复购买 |

### 目录覆盖说明

| 来源 | 显示数量 | 本报告覆盖 | 说明 |
| --- | --- | --- | --- |
| `docs.infimagames.com` 产品线 | 4 条产品线 | 4 条全覆盖 | LPSP、LPAMG、Rigged Gun Models、TFA Packs |
| Fab 搜索结果中的独立 URL | 6 个独立 Listing | 6 个全覆盖 | 含 Bundle 和单品 |
| 已下架/旧 Marketplace 商品 | 至少 1 个 | 1 个已记录 | "Animated Low Poly Weapons v1.0"（2022，未迁移到 Fab）可能已并入 LPAMG 或 LPSP |

**未覆盖的缺口**：
- Tactical FPS Animation Packs 除 Assault Rifle 和 Pistol 外，其余 5 个武器包的确切名称和 URL 未从官方来源核实（Fab 页面需登录渲染，搜索引擎未索引完整列表）
- Rigged Gun Model Packs 中各独立单枪的 Fab URL 未核实
- 所有产品的当前价格均未从官方来源核实（Fab 页面 403）

---

## 射击相关产品深挖

以下对 Apecox 相关性为"高"或"中"的产品逐一深挖。

### Tactical FPS Animation Packs（相关性：高）

#### 角色与骨架

- **视角**：同时包含第一人称手臂和第三人称全身动画。第一人称使用独立 Arms Mesh；第三人称使用全身 Mannequin。
- **骨架**：**UE5 Manny 兼容**。使用自定义 `SKEL_TFA_Mannequin` 骨架，基于 UE5 Manny 骨骼层级。官方文档明确支持 Manny 兼容角色，并提供"如何分配自定义角色模型"教程。
- **IK Rig/Retargeter**：未核实是否提供。但骨架与 Manny 层级一致，无需额外 Retargeter 即可使用 Manny Skeletal Mesh。
- **动画独立性**：可完全脱离 Demo Blueprint 和演示框架使用。Demo BP 仅作展示用途（官方 FAQ 明确：Demo 逻辑非多人、Demo 角色静态无移动动画）。

#### 动画覆盖

| 动画类型 | FP | TP | 说明 |
| --- | --- | --- | --- |
| Idle | ✅ | ✅ | 含瞄准/放松姿态变体、握持变体、扳机纪律姿态 |
| Walk / Jog / Sprint | ✅ (仅 FP) | ❌ | FP 含移动、扫射、后退。**TP 不含 Locomotion**——官方建议与 MoCap 下身动画混合 |
| Jump / Fall / Land | ✅ (仅 FP) | ❌ | FP 含跳跃和下落。TP 不含 |
| Crouch | ✅ (仅 FP) | ❌ | FP 含蹲伏姿态和过渡。TP 不含 |
| Equip / Unequip / Weapon Switch | ✅ | ✅ | 含快速出/收枪 |
| Fire | ✅ | ✅ | 含瞄准和非瞄准版本 |
| Dry Fire | 未核实 | 未核实 | 未在官方动画列表中明示 |
| Reload / Empty Reload | ✅ | ✅ | 含标准、空仓、快速、瞄准换弹 |
| Tactical Reload | ✅ | 未核实 | 含"Reload Aimed"变体，可视为战术换弹 |
| ADS / Aim Offset | ✅ | ✅ | 含瞄准偏移、瞄准过渡 |
| Hit React / Death | 未核实 | 未核实 | 未在动画列表中提及 |
| Melee | ✅ | ✅ | 含近战攻击 |
| Inspect | ✅ | ✅ | 含武器检视 |
| Jam Clear | ✅ | ✅ | 含卡壳排除动画 |
| Mag Check | ✅ | ✅ | 含弹匣检查 |
| Grenade Throw | ✅ | ✅ | |
| Fire Mode Switch | ✅ | 未核实 | |
| 左手 IK / 武器握持 | 未核实 | 未核实 | 动画含多种握持变体；IK 校正未明确说明 |

- **Animation Sequence / Montage / BlendSpace / Aim Offset / AnimBP**：确认提供 Animation Sequence 和 Aim Offset。Montage 和 BlendSpace 未在文档中明确提及。Demo AnimBP 提供作为展示。
- **FP/TP 区分**：明确分为 FP 角色动画（61 个）、TP 角色动画（34 个）、武器动画（24 个，FP+TP）。
- **按武器家族差异**：每武器独立动画包，所有动画均针对该武器制作（非通用重定向）。不同武器间的动画不共享。
- **Root Motion / Anim Notify / Motion Warping**：动画为 In-Place（无 Root Motion）。Anim Notify 状态和 Notify 在文档中有专门参考页。Motion Warping 未明确提及。
- **展示但不提供的内容**：Demo 角色移动动画、Demo 多人逻辑。

#### 武器与表现

- **武器 Mesh**：每包包含该武器的 Skeletal Mesh（带可动部件）。同时包含 Static Mesh 版本（未完全核实）。
- **独立骨骼/Socket**：含武器 Rig（Blender 源文件提供）。Socket（枪口、弹匣、抛壳口）未逐一核实，但武器结构含可拆卸弹匣、枪机等部件。
- **枪口 VFX/曳光/Impact/弹壳/声音/镜头后坐力**：音频（开火、操作、枪机、弹匣、撞击声）和 VFX 均包含。镜头震动烘焙在头部骨骼上（可通过 Layered blend per bone 减弱）。曳光和弹壳未单独核实。
- **FP/TP 武器 Mesh**：同一武器 Mesh 用于 FP 和 TP（通过不同摄像机视角渲染）。未确认是否有独立 LOD 或比例差异。
- **替换难易度**：官方提供"How to Import a Custom Weapon Model"教程。但自定义武器需匹配 TFA Weapon Rig 层级才能使用其动画。替换为其他商城武器需 Blender 重定向武器 Rig。

#### 技术与可拆用性

- **素材 vs 框架**：**以素材为主**。动画、Mesh、材质、音频、VFX 是核心交付物。Demo Blueprint 明确标注为展示/参考用途，非游戏就绪框架。
- **可迁移性**：动画可直接使用（赋值到 Manny 兼容骨架）。武器 Mesh 需匹配 SKEL_TFA_Mannequin 层级。Demo BP 不建议接入。音频和 VFX 可独立提取。
- **是否可只取素材**：**可以**。动画资产直接兼容 Manny；武器模型通过 Skeleton Assignment 可使用。不依赖 Infima 的输入、库存或网络系统。
- **源码**：无 C++ 源码。所有逻辑为 Blueprint（Demo 级别）。无预编译插件。
- **多人/GAS 宣传**：Demo 逻辑**非多人**（官方 FAQ 确认）。不涉及 GAS。Apecox 的 GAS、预测和服务器权威架构完全不受影响。

---

### Low Poly Animated - Modern Guns Pack（相关性：中）

#### 角色与骨架

- **视角**：**仅第一人称**。提供两套 FP 手臂 Mesh（基础 + 手套）。
- **骨架**：**UE4 Mannequin 骨架**。官方文档明确警告："与 UE5 Manny 骨架不兼容，除非手动重定向"。
- **IK Rig/Retargeter**：未提供。需用户自行创建 UE5 IK Retargeter 将 UE4 Mannequin 动画重定向到 Manny。
- **动画独立性**：可脱离 Demo BP 使用。动画为纯素材。

#### 动画覆盖

- 每武器含 FP 动画：换弹/空仓换弹/瞄准换弹/开火/瞄准开火/检视/收枪/冲刺/行走定向移动/刀攻击/手雷投掷
- **无第三人称动画**
- **无 Locomotion 完整集**（仅行走/冲刺，无跳跃/蹲伏）
- 动画为 In-Place，Animation Sequence 格式

#### 武器与表现

- 8 把低多边形武器（Skeletal Mesh，可互换部件：枪管、弹匣、滑套、握把等）
- 模块化配件：垂直/倾斜握把、红点瞄准镜、消音器、激光指示器、双脚架、手枪导轨
- 16+ 可定制材质（迷彩、碳纤维等）
- **不含音频和 VFX 系统**（不同于 TFA Packs）

#### 技术与可拆用性

- **以素材为主**。不含完整武器/库存框架
- 与 LPSP 集成需额外方案（通过 Discord 获取）
- **关键风险**：UE4 Mannequin → Manny 重定向工作量大；手指和武器握持位置可能不精确；无 TP 动画

---

### Modern Guns Bundle（相关性：中）

#### 角色与骨架

- **不含角色或动画**。仅为 Rigged 武器模型。

#### 武器与表现

- 8 把写实高质量武器（Static Mesh / Skeletal Mesh，4K PBR 纹理）
- Blender 源文件含完整 Rig 和预设 Idle 姿态
- 含 FP/TP 示例设置
- 独立单枪可拆买

#### 技术与可拆用性

- **纯模型素材**。不含动画、音频、VFX 或游戏逻辑
- **完美适配 Apecox**：可作为第一人称/第三人称武器 Mesh 替换占位资源
- UE 5.8 明确兼容
- **关键缺口**：必须搭配独立动画包（如 Tactical FPS Animation Packs）才能形成可用武器

---

### Low Poly Shooter Pack（相关性：低）

快速总结：完整 FPS 框架（武器/库存/生命/AI/多人/UI），UE4 Mannequin 骨架，UE 4.26-5.4。与 Apecox 已有 GAS 架构大面积冲突。美术资产价值有限（低多边形风格、无 Manny 兼容动画）。**不建议购买**作为素材来源；作为学习参考可考虑免费样品版。

---

## 与 Apecox 现有资产的缺口矩阵

| 能力维度 | Epic UE5.8 FP Template | Rifle Pro MoCap Pack | Infima TFA Packs | Infima LPAMG | Infima Modern Guns Bundle |
| --- | --- | --- | --- | --- | --- |
| FP 空手 Idle/Run | ❌ 模板有基础动画 | ❌ 不含 FP | ❌ 不含空手 FP Locomotion | ❌ 仅武器动画 | ❌ 仅模型 |
| FP 步枪 Idle/Aim | ❌ | ❌ | ✅ 多姿态 | ✅ 基本姿态 | ❌ |
| FP 步枪开火 | ❌ | ❌ | ✅ 含瞄准/非瞄准 | ✅ 含瞄准/非瞄准 | ❌ |
| FP 步枪换弹 | ❌ | ❌ | ✅ 多类型 | ✅ 多类型 | ❌ |
| FP 步枪切枪 | ❌ | ❌ | ✅ | ✅ | ❌ |
| FP 步枪检视/卡壳 | ❌ | ❌ | ✅ | ✅ | ❌ |
| FP Locomotion（持枪） | ❌ | ❌ | ✅ Walk/Run/Sprint/Strafe | ⚠️ 仅 Walk/Sprint | ❌ |
| TP 步枪 Idle | ❌ 模板不含 | ✅ Stand Aim/Relaxed | ✅ | ❌ | ❌ |
| TP 步枪开火 | ❌ | ✅ Single/Continuous | ✅ | ❌ | ❌ |
| TP 步枪换弹 | ❌ | ✅ | ✅ (Reload 变体) | ❌ | ❌ |
| TP 步枪切枪 | ❌ | ✅ Holster/Unholster | ✅ | ❌ | ❌ |
| TP 步枪移动 | ❌ | ✅ Walk/Jog/Run 四向 | ❌ 仅上身 | ❌ | ❌ |
| TP 跳跃/落地 | ❌ | ✅ | ❌ | ❌ | ❌ |
| TP 蹲伏 | ❌ | ✅ Crouch | ❌ | ❌ | ❌ |
| TP 受击/死亡 | ❌ | ✅ 持枪死亡 | ❌ | ❌ | ❌ |
| 武器 Mesh（写实） | ⚠️ 模板 AR 占位 | ❌ 不含 | ✅ 武器自带 | ⚠️ 低多边形 | ✅ 写实高质量 |
| 武器 Socket/VFX/音频 | ❌ | ❌ | ✅ 完整 | ⚠️ 部分 | ❌ |
| Manny 骨架兼容 | ✅ | ⚠️ 需 Retarget | ✅ 原生兼容 | ❌ UE4 Mannequin | ✅ 无动画需求 |
| 脱离框架可拆用 | N/A | ✅ | ✅ | ✅ | ✅ |
| UE 5.8 声明支持 | ✅ | ❌ 旧包 | ⚠️ 5.4+（待确认 5.8） | ⚠️ 待确认 | ✅ 明确标注 |

### 关键回答

1. **哪个产品能最直接补足第一人称空手/持枪表现**：**Tactical FPS Animation Packs**。唯一 Manny 兼容 + FP 动画完整 + 写实风格。但**不含空手 FP Locomotion**——Apecox 仍需从其他来源（Epic 模板或 Stephen_FPS）获取第一人称空手移动动画。

2. **哪个产品能补足第三人称远端玩家的持枪、开火、换弹和移动**：**混合方案**。TP 上身武器动画（开火/换弹/切枪/检视）用 TFA Packs；TP 下身 Locomotion（行走/跑动/跳跃/蹲伏）用 **Rifle Pro MoCap Pack**。两者骨架不同但通过 IK Retargeter 可统一到 Manny。

3. **是否存在同时覆盖 FP/TP 且能使用 Manny 的产品**：**Tactical FPS Animation Packs** 最接近——FP 动画完整、TP 动画含上身武器动作。但 TP 不含 Locomotion，需搭配补充。

4. **哪些产品只是框架功能丰富，但对美术素材帮助有限**：**Low Poly Shooter Pack**。大量 BP/C++ 系统与 Apecox 冲突，动画为 UE4 骨架 + 低多边形风格，素材价值低。

5. **哪些产品和现有 Rifle Pro MoCap 大量重复**：无直接重复。Rifle Pro MoCap 优势在 TP Locomotion 全覆盖；TFA Packs 优势在 FP+TP 武器操作动画和 Manny 兼容。两者互补。

6. **哪些产品适合未来狙击枪、手枪或英雄特殊武器**：TFA Packs 系列覆盖多武器类型（如其武器列表含 Assault Rifle、Pistol、SMG、Shotgun、Sniper、Revolver、LMG、Rocket Launcher）。LPAMG 同样覆盖 8 种武器类型。

7. **如果购买某个 Bundle，会不会重复购买其单品**：Tactical FPS Shooter Pack 是 7 个 TFA 单品的 Bundle，Modern Guns Bundle 是 8 个 Rigged Gun Model 单品的 Bundle。**不要同时购买 Bundle 和其涵盖的单品**。

---

## 推荐分级与采购组合

### A 级：当前第一把步枪表现最值得采用

| 产品 | 解决缺口 | 可脱离框架 | Manny 适配 | 预计人工工作 | 关键风险 |
| --- | --- | --- | --- | --- | --- |
| **Tactical FPS Shooter Pack**（7 武器 Bundle） | FP+TP 开火/换弹/切枪/检视/Idle/Aim；多武器类型为后续预留 | ✅ 纯素材。动画直接赋值 Manny 兼容骨架；武器 Mesh 通过 Skeleton Assignment 使用 | ✅ 原生 Manny 兼容（SKEL_TFA_Mannequin） | **少量配置+动画重定向**。将 Manny Skeletal Mesh 分配到 SKEL_TFA_Mannequin → 创建 AnimBP 或 Anim Layer → 配置 Montage 和 Notify → 接入 Apecox 的 Fire/Impact GameplayCue | (1) 需用户确认 Bundle 中 7 武器的确切列表和价格；(2) UE 5.8 兼容性需实测验证；(3) 不含空手 FP Locomotion 和 TP Locomotion |
| **Tactical FPS Animation Pack - Assault Rifle**（单品） | 同上但仅步枪 | ✅ 同上 | ✅ 同上 | **少量配置**。如只需步枪，性价比可能优于 Bundle。但后续扩展需逐把购买 | 同上。且单买可能不如 Bundle 总价划算 |

### B 级：后续武器家族可能需要

| 产品 | 解决缺口 | 可脱离框架 | Manny 适配 | 预计人工工作 | 关键风险 |
| --- | --- | --- | --- | --- | --- |
| **Modern Guns Bundle** | 写实高质量武器 Mesh 替换当前占位模型 | ✅ 纯模型素材 | N/A（无动画） | **少量配置**：导入 → 设置 Socket（枪口/弹匣/抛壳） → Apecox WeaponPresentationDefinition 配置 | (1) 需搭配 TFA Packs 才有动画；(2) 武器 Rig 与 TFA 武器 Rig 可能不一致，需 Blender 调整；(3) 不含武器动画 |
| **LPAMG**（如接受低多边形风格） | FP 武器动画 + 武器模型一体方案 | ✅ | ⚠️ 需 UE4→UE5 IK Retargeter | **动画重定向**：完整 UE4 Mannequin → Manny IK Retarget 流程 | (1) 无 TP 动画；(2) 低多边形风格需团队接受；(3) 手指/握持重定向精度风险 |

### C 级：主要适合作为实现参考，不建议接入项目

| 产品 | 原因 |
| --- | --- |
| **Low Poly Shooter Pack** | 完整 FPS 框架与 Apecox GAS 架构冲突。动画为 UE4 Mannequin 低多边形风格。作为蓝图逻辑参考有价值（武器切换、UI、AI 行为树），但不建议作为项目依赖接入。可考虑免费样品版用于学习 |

### D 级：与当前目标无关

| 产品 | 原因 |
| --- | --- |
| **Modern Revolver**（单品） | 单把武器模型无动画。如已在 Modern Guns Bundle 中则重复。Apecox 当前阶段不需要独立左轮手枪 |

### 最小采购组合

覆盖当前步枪 FP/TP 表现的最低购买：

```text
Tactical FPS Animation Pack - Assault Rifle（单品）
```

预估人工：将 Manny Mesh 分配到 SKEL_TFA_Mannequin → 配置 Apecox Fire/Impact GameplayCue → 接入 AnimBP。工作日约 2-4 天（含 Anim Notify 和 Montage 配置）。

局限性：后续添加手枪/狙击枪需再次购买其他 TFA 单品；武器 Mesh 为 TFA 自带风格（写实但不一定是最终选择）。

### 扩展采购组合

为当前步枪 + 后续手枪、狙击枪和英雄特殊武器预留：

```text
Tactical FPS Shooter Pack（7 武器 Bundle）
+ Modern Guns Bundle（写实武器 Mesh，如需替换 TFA 自带武器外观）
```

优势：7 种武器类型覆盖 AR/Pistol/SMG/Shotgun/Sniper/Revolver/LMG/Launcher 中的大部分；写实武器 Mesh 可配合或替换 TFA 自带模型；Manny 兼容减少 Retarget 工作量。

预估人工：按武器逐个接入，每武器约 1-3 天（动画赋值 + Montage + Cue 配置）。首把步枪约 2-4 天。

### 不建议购买清单

| 产品 | 不适合的原因 |
| --- | --- |
| Low Poly Shooter Pack（付费完整版） | 框架冲突；素材与 LPAMG/TFA 相比无独特价值；价格不作为唯一理由 |
| LPAMG + Tactical FPS Animation Packs 同时购买 | 功能大量重叠（均为 FP 武器动画包）；风格不同（低多边形 vs 写实）；建议只选其一 |
| Modern Revolver（单品） | 已在 Modern Guns Bundle 中覆盖（RC425）；单买性价比低 |
| LPSP + LPAMG 同时购买 | LPSP 已内含 AnimatedLowPolyWeapons（与 LPAMG 高度重叠） |

### 仍需人工确认清单

以下事项无法从公开资料核实，必须在购买前由用户在 Fab 商品页确认：

1. **Tactical FPS Shooter Pack 的 7 把武器确切列表**——搜索结果只确认了 Assault Rifle 和 Pistol 两个独立包名称。7 武器 Bundle 涵盖哪些武器未核实。
2. **各产品当前 Fab 价格**——Fab 页面需登录和 JS 渲染，WebFetch 返回 403。建议用户在 UE 编辑器的 Fab 窗口中直接查看。
3. **Tactical FPS Animation Packs 是否明确支持 UE 5.8**——官方标注 UE 5.4+，但未逐一验证 5.8 兼容性。UE 5.x 的动画资产通常向上兼容，但 AnimBP/Blueprint 可能需重新保存。
4. **Modern Guns Bundle 武器列表 vs LPAMG 武器列表**——两者使用相同代码名（AG14W 等），但未确认是否 100% 对应。Modern Guns Bundle 可能包含不同的武器选择。
5. **Tactical FPS Packs 和 Modern Guns Bundle 的武器 Rig 兼容性**——是否能将 Modern Guns 的高质量 Mesh 直接套用 TFA 的动画，还是需要 Blender 重新 Rig。
6. **各 Bundle 是否含当前折扣或限时优惠**——搜索结果显示的日期（如 Modern Guns Bundle 2026/06/27 更新）可能伴随价格变化。
7. **Infima Games 是否在 Fab 之外有其他未索引的产品**——如 Discord 专属内容、Early Access 或 Gumroad 独家产品。

---

## 集成风险

### 骨架与动画风险

1. **SKEL_TFA_Mannequin vs Manny 原生骨架**：虽然官方文档声明 Manny 兼容，但实际骨骼层级可能存在微小差异（额外 IK 骨骼、手指骨骼命名等）。首次接入需做完整的 A-Pose 对照验证。
2. **动画重定向精度**：LPAMG（UE4 Mannequin → Manny）重定向的手指和武器握持位置可能出现偏差，需逐动画验证。
3. **Anim Notify 和 Curve**：TFA Packs 的 Notify 可能与 Apecox 的 GameplayCue 触发方式不完全匹配，需创建适配层。

### 技术集成风险

4. **TFA Demo Blueprint 污染**：安装时自动带入的 Demo 内容可能包含大量不必要资产。建议只迁移动画、Mesh、材质、音频和 VFX 到 Apecox 目录，丢弃 Demo 逻辑。
5. **材质/Shader 兼容性**：Infima 产品可能使用旧版材质系统（LPSP/LPAMG 为 UE4 时代产物），在 UE 5.8 的 Substrate 或 Lumen 环境下需重新验证。
6. **UE 5.8 兼容性**：TFA Packs 标注 UE 5.4+，Modern Guns Bundle 标注 UE 5.0-5.8。后者更安全。TFA 的 AnimBP 在 UE 5.8 中可能需要重新保存或小幅修正。

### 许可与分发风险

7. **Fab 许可层级**：Fab Professional 许可通常允许在商业和求职作品中使用。但用户应在购买前阅读具体许可条款，确认求职展示作品属于允许用途。
8. **Blender 源文件许可**：Infima 提供武器 Rig 的 Blender 源文件，但动画源文件不包含。如需修改动画，需自行在 UE 内编辑或从 FBX 逆向。

---

## 来源链接

### 官方来源

- Infima Games 文档站：`https://docs.infimagames.com/home.md`
- Infima Games 文档索引：`https://docs.infimagames.com/llms.txt`（返回 404，markdown 版本可用）
- LPSP 介绍：`https://docs.infimagames.com/product/low-poly-shooter-pack/getting-started/introduction.md`
- LPAMG 介绍：`https://docs.infimagames.com/product/low-poly-animated-modern-guns-pack/getting-started/introduction.md`
- TFA Pack 快速入门：`https://docs.infimagames.com/product/tactical-fps-animation-packs/getting-started/quick-start-guide.md`
- TFA Pack FAQ：`https://docs.infimagames.com/product/tactical-fps-animation-packs/getting-started/faq.md`
- TFA 动画列表：`https://docs.infimagames.com/product/tactical-fps-animation-packs/guides-and-tutorials/reference/animation-list.md`
- TFA 自定义角色教程：`https://docs.infimagames.com/product/tactical-fps-animation-packs/guides-and-tutorials/editor/how-to-assign-a-custom-character-model-in-unreal-engine-5-ue5-manny.md`
- Rigged Gun Model Packs 快速入门：`https://docs.infimagames.com/product/rigged-gun-model-packs/getting-started/quick-start-guide.md`
- LPAMG 更新日志：`https://docs.infimagames.com/product/low-poly-animated-modern-guns-pack/project-info/changelog.md`

### Fab 商品页（需登录 UE Fab 窗口查看）

- Modern Guns Bundle：`https://www.fab.com/listings/eb4258b3-cec4-4995-85fe-ace8c9b5d45f`
- LPAMG：`https://www.fab.com/listings/6914a185-4b14-475c-8f4f-cc0a6a3c589f`
- Tactical FPS Shooter Pack：`https://www.fab.com/listings/76c5f2b0-ea55-461c-8b3c-811a9deb3279`
- LPSP：`https://www.fab.com/listings/90ba076a-dc9a-4782-9ac8-dc2ed4f06405`

### Gumroad

- Infima Games Gumroad 商店：`https://infimagames.gumroad.com`（ECONNREFUSED 于调研时）
- Modern Revolver：`https://infimagames.gumroad.com/l/modern-revolver-rigged-ue5`
- LPSP：`https://infimagames.gumroad.com/l/low-poly-shooter-pack`

### 旧 Marketplace

- LPSP：`https://www.unrealengine.com/marketplace/en-US/product/low-poly-fps-pack`
- Animated Low Poly Weapons v1.0：`https://www.unrealengine.com/marketplace/zh-CN/product/animated-low-poly-weapons-v1-0`（2022，未迁移到 Fab）

### 第三方参考（非官方，仅供交叉验证）

- PSDly Modern Guns Bundle 页：`https://www.psdly.co.uk/modern-guns-bundle-rigged-game-ready-models`
- PSDly LPSP v6.0 页：`https://www.psdly.co.uk/low-poly-shooter-pack-for-unreal-engine`
- GameAssetDeals LPSP v4.3 页：`https://www.gameassetdeals.com/asset/54947/low-poly-shooter-pack-v4-3`
- GameContentDeals LPAMG 页：`https://gamecontentdeals.com/assets/3d/low-poly-animated-modern-guns-pack-built-in-3/`

---

*报告结束。本轮未执行任何购买、下载、项目修改、MCP 或 Git 操作。*
