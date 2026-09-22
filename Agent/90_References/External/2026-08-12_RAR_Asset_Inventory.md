# Realistic Assault Rifle Template 本地资产盘点

更新日期：2026-08-12

## 本地来源

```text
D:/UE_Resource/Realistic Assault Rifle Template
```

目录中包含两份内容一致的完整工程：

| 目录 | EngineAssociation | 说明 |
| --- | --- | --- |
| `RAR_5.4/RAR` | 5.4 | 保留为原始备份 |
| `UE5/RAR` | 5.3 | 用作 UE 5.8 兼容性验证副本 |

总计约 4.19 GiB、3680 个 `.uasset`；两份工程的相对路径和文件大小完全一致，因此后续只从一份工程选择资产。

## 产品身份

本地内容使用：

```text
/Game/InfimaGames/RealisticAssaultRifle
A_RAR_FP_*
AM_RAR_FP_*
```

它属于旧版 `Realistic Assault Rifle Template (RAR)` 体系，不是官网当前：

```text
/Game/InfimaGames/TacticalFPSAnimations
A_TFA_FP_*
A_TFA_TP_*
```

因此不能按新版 TFA 商品页面推断本地资产包含第三人称动作。

## 有价值的核心资产

- 112 个 FP 角色动画资产：77 个 Sequence/Pose、32 个 Montage、3 个 Blend Space。
- 覆盖 Idle、移动、瞄准、开火、换弹、装备/收枪、检查、近战、手雷、冲刺、跳跃、翻越、蹲伏和爬行。
- 16 个枪械自身动画资产，覆盖 Fire、Reload、Inspect、MagCheck、Unholster 等。
- 模块化 SkeletalMesh 步枪、弹匣、瞄具、握把、枪托、消音器、激光器、子弹和弹壳。
- 专用 FP Arms/Mannequin Mesh，可替换当前完整 Manny 第一人称表现。
- Niagara：`NS_IG_MuzzleFlash`、`NS_IG_MuzzleFlash_Silencer`、`NS_IG_Impact_Bullet` 等。
- 步枪专属音频：16 个 SoundWave、14 个 SoundCue。

## 明确缺口

- 本地 RAR 没有 `A_RAR_TP_*` 或 `AM_RAR_TP_*` 动画。
- 第三人称持枪移动与战斗动作继续由 `Rifle Pro MoCap Pack` 提供。
- 本地下载不包含 FBX、Blender 源文件或文字文档。

## Apecox 使用边界

```text
FP：RAR Arms + 角色动画 + 步枪动画 + Mesh + VFX/SFX
TP：Manny + Rifle Pro MoCap
玩法：Apecox GAS / WeaponInstance / Inventory / 预测和服务器验证
```

`GameplayCore`、Demo Player、Demo Weapon、输入、库存和 DataTable 仅供依赖与时序研究，不作为 Apecox 运行时基础。正式迁移必须从实际引用资产出发选择性执行，并保留供应商命名空间。
