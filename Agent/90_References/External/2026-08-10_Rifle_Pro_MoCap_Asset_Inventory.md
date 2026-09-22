# Rifle Pro MoCap Pack 资产盘点

更新日期：2026-08-10

## 本机位置

- 根目录：`D:/UE_Resource/UE-FPS-人物动作/Rifle Pro MoCap Pack/Rifle Pro MoCap Pack`
- UE 资产：`data/Content/Rifle_01`
- 源 FBX：`data/Source/FBX/UE4_Rifle_01_PRO_v27A1_FBX`
- Maya 源文件：`data/Source/Maya`

## 结论

该资源是以完整 UE4 Mannequin 为目标的第三人称步枪动作库，适合补强 Apecox 的远端角色持枪移动、瞄准、射击、换弹和切枪表现。

它不是第一人称 Arms 动画包，不能解决空手第一人称跑动，也不能代替 Stephen_FPS 或 Modern Guns 提供的第一人称武器动画。

## 规模

- 动画目录 PDF 列出 546 个动作语义，并说明动作同时提供 Root Motion 和 In-Place/IPC 版本。
- 本地共有 917 个 `.uasset`，其中 `Animation` 目录有 886 个动画资产。
- 本地共有 892 个源 `.fbx`，另有 1 个 `.umap` 和 1 个 Maya `.ma` 文件。
- 动画资产命名统计：880 个 `W2_*` 步枪动作、4 个 `W1_*` 手枪转步枪动作、2 个 `NW_*` 无武器 Idle。

PDF 中的 546 是动作语义总数，本地文件数更大主要是因为 Root Motion、In-Place、Aim Offset、拆分跳跃和 FBX/UE 资产版本并存。

## 动作覆盖

### 站立与姿态

- Stand Aim、Stand Relaxed Idle。
- 多组 Fidget 和 Look Around。
- Aim 与 Relaxed 之间的转换。
- 45、90、135、180 度左右原地转身。
- 连续 Turn In Place 循环。

### 移动与跳跃

- 持枪 Walk、Jog、Run。
- 前后左右、斜向、Strafe 和 Backpedal。
- 站立到 Walk/Jog/Run 的起步和带转向过渡。
- Crouch Aim、Crouch Relaxed 和蹲伏移动。
- 多方向 Jump Start、Air Loop、Landing。
- Split Jumps，可按左右脚和移动方向选择起跳、空中及落地片段。

### 瞄准

- 站立 Aim Offset。
- 站立 Relaxed Look Offset。
- 蹲伏 Aim Offset。
- 覆盖中心、上下、左右和对角方向。

### 武器动作

- 单发、三连发、全自动循环。
- 大后坐力射击。
- Pump/Chamber Cycle。
- 站立与蹲伏的 Aim/Relaxed 换弹。
- Unjam 排障。
- Holster、Unholster、背后取枪和还枪。
- 步枪与手枪之间的切换衔接。
- 多方向持枪死亡动作。

## 第一人称与其他缺口

- 没有 FirstPerson、FP 或 Arms 命名的资产。
- 没有独立第一人称手臂 Mesh。
- 没有空手第一人称 Idle/Run/Sprint。
- 手枪只覆盖 4 个 `W1_*` 切换衔接，不是完整手枪动作族。
- 两个 `NW_*` 只是无武器 Idle，不构成空手 Locomotion。
- 资源不提供武器、装备、弹药或 GAS 游戏逻辑。

## Apecox 推荐用法

1. 玩家和远端角色的常规持枪 Locomotion 优先使用 In-Place 版本，由 CMC 负责真实位移、客户端预测和服务器校正。
2. Root Motion 只用于确实需要动画驱动位移的短 Montage，并单独验证网络同步，不用于日常 Walk/Jog/Run。
3. 优先选择源 FBX，通过 UE 5.8 的 IK Rig/IK Retargeter 重定向到 Manny；不要按旧文档直接覆盖 Manny 的参考姿势。
4. Aim Offset、Turn In Place、Fire、Reload、Equip/Swap 可作为第三人称 Rifle 动画 Profile 的主要资源。
5. 旧包可能使用 `wep` Weapon Bone 或 `middle_01` 手指 Socket。Apecox 应建立自己的武器 Socket、左手 IK 和动画 Profile，不让旧 Socket 约定成为运行时架构依赖。
6. 第一人称继续优先使用 Stephen_FPS 的 `SK_Mannequin_Arms` 与 Rifle/Pistol 动画，或 Modern Guns 的逐武器第一人称动作。

## 建议的首批候选

第一把步枪接入时，可优先验证以下语义，再按实际观感扩大迁移范围：

- Stand Aim/Relaxed Idle。
- Walk/Jog Aim 四向循环。
- Run Forward。
- Stand/Crouch Aim Offset。
- Turn In Place。
- Fire Single、Fire Continuous。
- Reload。
- Equip/Unholster 与 Holster。
- 持枪 Jump Start/Air/Land。

## 随包 PDF

- `data/Source/Rifle Pro 27 - UE4 Animation List.pdf`：完整动作目录。
- `data/Source/UE4VersionCompatiblilityandUsage.pdf`：旧 UE4 迁移、FBX 导入、Weapon Bone 和 Socket 说明。
- `data/Source/A POSE OR T POSE  CONVERSION.pdf`：旧 Retarget Manager 的 A/T Pose 转换流程，仅作历史参考。

## 风险

- 原始 `.uasset` 来源较老，UE 5.8 直接打开和保存后不可回退。
- UE4 Mannequin 与 UE5 Manny 骨架不同，需要验证手指、肩部、武器握持、Root Motion、Additive、Notify 和 Aim Offset。
- PDF 使用的是 UE4 Retarget Manager 工作流；Apecox 应采用 UE5 IK Retargeter。
- 不应全量迁入 886 个动画。按 Rifle Profile 的真实需求选择性迁入，减少仓库体积和资产维护成本。
