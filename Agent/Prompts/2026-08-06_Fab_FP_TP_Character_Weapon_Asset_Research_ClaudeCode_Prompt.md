# ClaudeCode 调研 Prompt：Fab 第一/第三人称角色、枪械与动画资产

更新日期：2026-08-06

## 一、你的身份与任务

你是 Apecox 项目的资产调研子代理。本次只做公开网络调研，不写代码、不操作 Unreal Editor、不下载或购买资产。

请以 Epic 官方 Fab 商城为主要来源，寻找适合 Apecox 多人英雄射击原型的角色、枪械、第一人称动画和第三人称动画资产。优先寻找免费资产；付费资产只有在明显补足关键缺口、匹配度较高时才进入推荐清单。

项目路径：

```text
D:/UnrealProject/Apecox
```

开始前阅读：

1. `D:/UnrealProject/Apecox/Agent/90_References/Inherited_Framework/New_FPS_Project_Direction_And_Asset_Baseline.md`
2. `D:/UnrealProject/Apecox/Agent/01_Project/Product_And_Portfolio_Direction.md`
3. `D:/UnrealProject/Apecox/Agent/02_CombatFramework/Apecox_Combat_Framework_Architecture_RFC.md` 中与第一/第三人称、动画、武器表现有关的部分。

## 二、项目目标与使用场景

Apecox 是 UE 5.8 C++ 多人英雄射击求职作品，不是商业级完整大逃杀。当前希望完成高质量的小型多人战斗垂直切片：

- 本地玩家主要看到第一人称手臂和武器表现。
- 其他玩家看到第三人称全身角色、武器和动作。
- 玩家出生为空手，可拾取并携带两把普通武器。
- 首批武器方向：步枪、霰弹枪、狙击枪；手枪、冲锋枪、重武器后续扩展。
- 需要腰射，之后会加入 ADS、换弹、切枪、弹药、配件和瞄具。
- 角色以后可能有弓箭、火箭筒、无人机等技能专用武器或战斗实体。
- 未来希望支持冲刺、跳跃、蹲伏、滑铲/滑铲跳，但本次不要求一个资产包全部覆盖。
- 权威逻辑、GAS、复制和预测由项目自建；商城蓝图逻辑不是购买重点。

## 三、当前已经拥有的资产

不要重复推荐功能高度重合、价值不明显的包。

### 1. Stephen_FPS（UE 5.7 内容项目）

已有：

- 步枪和手枪模型。
- 第一人称手臂及步枪/手枪动作。
- UE5 Manny/Quinn 第三人称动作。
- 第三人称开火、换弹、装备、蹲伏、跳跃、转身、Aim Offset、Blend Space。
- 枪口火焰、曳光、弹壳、环境命中特效、音频和部分 HUD 素材。

定位：第一把步枪双视角纵向切片的首选资产。

### 2. Modern Guns Pack（本地包标注 UE 5.4）

已有：

- 步枪、轻机枪、冲锋枪、手枪、狙击枪、左轮、霰弹枪、火箭筒等 8 个武器族。
- 武器骨骼网格、模块化部件、两套第一人称手臂。
- 逐武器第一人称开火、瞄准、换弹、空仓换弹、检视、移动、冲刺、跳跃、收枪、手雷和近战动作。
- 音效、Sound Cue 和基础枪口表现。

风险：UE4 Mannequin 骨骼；部分表现为旧 Cascade。第一人称手臂可以保持独立骨骼，不要求全部转到 Manny。

### 3. Rifle Pro MoCap Pack（旧 UE4 来源，附 FBX）

已有：

- 大量第三人称步枪 In-Place、Root Motion、Aim Offset、开火、换弹、收枪与切换动作。
- M4、1911 和 FBX/Maya/MotionBuilder 源文件。

定位：第三人称步枪动作补充库；需要时通过 UE5 IK Retarget 使用。

## 四、本次重点寻找的缺口

按照以下优先级调研。某个包只解决一个缺口也可以，不要求万能资产包。

### P0：第一/第三人称武器动画协同

重点寻找：

- 同一武器或同一武器家族同时提供 FP Arms 与 TP Full Body 动作的资产。
- 动作至少覆盖 Equip/Unequip、Hip Fire、Reload、Empty Reload；有 ADS、Sprint、Jump 更好。
- 第一人称和第三人称不要求逐帧完全一致，但关键语义点必须可对齐，例如开火、弹匣移除/插入、拉栓和装备完成。
- 可以是动画包、带完整动画的枪械包，或明确展示双视角工作流的内容示例。

### P0：特殊武器动作缺口

重点寻找：

- 霰弹枪：尤其是管式霰弹枪逐发装填、打断装填后开火；弹匣式霰弹枪可复用 Rifle 家族。
- 狙击枪：拉栓、空仓/普通换弹、肩射与 ADS。
- 手枪/左轮：如果免费或高匹配，记录弹匣式手枪与左轮弹巢动作。
- Heavy：火箭筒肩扛、装备和装填，仅作为较低优先级候选。

### P1：通用第三人称武器姿态与移动

重点寻找 UE5 Manny/Quinn 可用或容易重定向的：

- Unarmed、Rifle、Pistol、Heavy 姿态族。
- Idle、四向 Walk/Jog/Run、Sprint、Crouch、Jump/Fall/Land、Turn In Place。
- Aim Offset、上下瞄准、持枪转向。
- 左手 IK 或武器 Socket 调整友好。

第三人称按语义族复用，不要求每把枪有独立完整动作。步枪动作可近似覆盖突击步枪、冲锋枪、轻机枪、狙击枪和弹匣式霰弹枪。

### P1：可复用角色外观

寻找适合现代或近未来英雄射击的角色资产：

- 最好兼容 UE5 Manny/Quinn，或明确提供 IK Rig/Retargeter/FBX。
- 可以是单个高质量角色，也可以是模块化角色包。
- 需要第三人称全身 Mesh、合理材质和基本 LOD；面部系统不是当前重点。
- 不要求每名英雄拥有独立动画骨架；优先能共享项目动作体系的角色。
- 免费角色即使风格不完全统一，也可以进入“原型可用”清单。

### P1：可用枪械模型与模块化部件

已有 Modern Guns，不需要为了数量重复购买。只记录明显优于或补足现有资产的候选：

- 至少一把高质量步枪、霰弹枪或狙击枪。
- 最好是 Skeletal Mesh，弹匣、枪栓、扳机等可动画；纯 Static Mesh 也可记录，但要标明局限。
- 有枪口、弹壳抛出口、握持点 Socket 或容易自行添加。
- 有独立弹匣、瞄具或模块化部件更好。
- 现代/近未来、写实或轻度风格化均可；不要求与已有包完全同风格。

### P2：表现与移动补充

可选记录：

- Niagara 枪口火焰、曳光、弹壳、不同表面弹着、命中与护盾破裂效果。
- 第一/第三人称冲刺、滑铲、滑铲跳动作。
- 弓箭、火箭筒、无人机等技能专用资产。

这些不应挤占 P0/P1 调研篇幅。

## 五、筛选原则：不要过度严格

本项目允许通过多套资产拼装原型，请遵守：

- **允许重定向**：UE4 Mannequin、UE5 Manny/Quinn 或自定义骨骼都可以进入候选，但必须说明工作量。
- **允许分开来源**：FP Arms、TP Full Body、Weapon Mesh、VFX、SFX 可以来自不同包。
- **允许动作族复用**：不要求步枪、冲锋枪、狙击枪拥有完全独立的第三人称 Locomotion。
- **允许版本升级**：明确支持 UE 5.4 及以上通常可考虑用于 UE 5.8；更旧资产若附 FBX、源文件或重定向路径，也可作为风险候选。
- **允许原型质量**：动作和模型只要语义正确、视觉不突兀、能支持技术验证即可，不以商业成品标准淘汰。
- **优先可检查的真实内容**：页面需要能确认 Mesh、Skeleton、Animation、数量、版本或演示视频中的至少部分信息。

但以下情况不要列入主要推荐：

- 只有教程视频或蓝图系统，没有可复用美术内容。
- 页面无法确认资产究竟包含模型还是只包含展示图。
- 强依赖其封闭玩法框架，素材无法合理拆出。
- 只有渲染结果，没有 UE 资产、FBX 或可导入源文件。
- 授权来源不明确，或不是 Fab/Epic 可核验的合法商品页面。

## 六、每个候选必须核对的字段

对每个进入报告的候选，尽量核对：

1. 资产名称、作者/发行者。
2. Fab 直接链接，不能只给搜索结果链接。
3. 当前价格；标明“免费”“限时免费”“付费”或“价格无法确认”。价格具有时效性，记录查询日期。
4. Fab Standard License 或页面显示的许可证信息；不要自行提供法律保证。
5. 支持的 UE 版本、资源格式和最近更新时间。
6. 骨骼：UE4 Mannequin、UE5 Manny/Quinn、MetaHuman、自定义骨骼或未知。
7. 是否附 FBX、IK Rig、IK Retargeter、Control Rig 或源工程。
8. 是否包含 FP Arms、TP Full Body、Weapon Mesh；三者必须分开写，不能混为“支持第一/第三人称”。
9. 动画语义清单：Locomotion、Aim Offset、Fire、Reload、Empty Reload、Equip、ADS、Sprint、Jump、Slide，以及特殊装填。
10. 武器 Mesh 是否可动画、是否模块化、是否含弹匣/枪栓/瞄具。
11. 是否包含 VFX、SFX、AnimBP、示例地图；蓝图玩法逻辑只作参考。
12. 与现有 Stephen_FPS、Modern Guns、Rifle Pro 的重合度。
13. 预计接入方式：直接使用、UE5 IK Retarget、保留独立 FP 骨骼、FBX 重导入或仅作参考。
14. 未确认事项和风险，不能用猜测补齐。

## 七、调研方法

1. 优先使用 Fab 的分类、关键词和直接商品页面。
2. 可以使用搜索引擎定位 Fab 页面，但最终证据必须回到 Fab 商品页；必要时补充作者官方文档或演示视频。
3. 建议组合使用英文关键词：

```text
free FPS arms animations Unreal Engine
first person third person weapon animations UE5
Manny rifle animation pack
Manny firearm locomotion aim offset
shotgun reload animation first person third person
sniper bolt action animation UE5
modular weapon pack skeletal mesh
modular sci fi character Manny UE5
slide animation pack UE5 Manny
Niagara muzzle flash bullet impact pack
```

4. 至少查看 15 个具有直接商品页的候选，最多查看 30 个；达到足够覆盖后停止，不进行无止境搜索。
5. 如果 Fab 页面因登录、地区或动态加载无法读取，明确标记“无法核验”，不要编造价格、版本或内容。
6. 不登录用户账户，不领取、不加入购物车、不购买、不下载。

## 八、报告结构

将中文报告写入：

```text
D:/UnrealProject/Apecox/Agent/90_References/External/2026-08-06_Fab_FP_TP_Character_Weapon_Asset_Research.md
```

报告按以下结构编写：

### 1. 执行摘要

- 现有资产是否足以启动第一把步枪。
- 当前真正阻塞 Phase 2 的美术缺口。
- 是否有必要现在购买资产。

### 2. 现有资产覆盖与缺口矩阵

按 FP Arms、TP Body、Weapon Mesh、Locomotion、Fire、Reload、Special Reload、VFX、SFX、Character Appearance 列表对照。

### 3. 免费候选优先清单

最多保留 8 个值得实际领取/测试的候选，按优先级排序。每个候选包含第六节要求的字段。

### 4. 付费高匹配候选

最多保留 5 个。必须说明它比现有资产具体多解决了什么，以及为什么值得花钱。

### 5. 不推荐或暂缓候选

简要列出看似相关但重复度高、版本风险大、素材不可拆分或信息不足的候选，避免用户重复调查。

### 6. 推荐的拼装方案

至少给出三套：

- 零新增付费：完全依靠现有资产与免费补充。
- 最小购买：只购买一个最能补关键缺口的资产。
- 更完整方案：允许少量付费，覆盖双视角和第二武器家族。

每套写清 FP、TP、Weapon、VFX/SFX 分别来自哪里，以及主要重定向/兼容工作。

### 7. 最终行动建议

使用以下分组：

- `现在可免费领取/加入库`
- `Phase 2 前建议购买`
- `等实际缺口出现再购买`
- `无需购买，现有资产可解决`

## 九、停止条件与边界

满足以下条件后停止：

- 已核对 15 至 30 个直接 Fab 商品页，或 Fab 可访问候选已经耗尽。
- 免费与付费候选均完成分级。
- 已明确现有资产能覆盖什么、仍缺什么。
- 已给出三套可执行的拼装方案。
- 所有关键结论都有直接链接或明确标注无法核验。

完成后只汇报：报告路径、调研候选数量、免费推荐数量、付费推荐数量、最重要的三个结论。不要执行 Git 操作。
