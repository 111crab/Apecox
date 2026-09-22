# Apecox 公开仓库外部资产依赖

更新日期：2026-09-22

## 原则

Apecox 的公开仓库提交项目源码、自建配置/蓝图资产、协作文档和测试；第三方原始美术与动画资产不提交。完整本地工程继续保留这些资源，用于编辑器运行、录屏和面试演示。

## 被 `.gitignore` 排除的内容根目录

| Apecox 目录 | 来源/用途 | 本地恢复方式 |
| --- | --- | --- |
| `Content/InfimaGames/` | Realistic Assault Rifle 及其依赖；第一人称武器、Arms 动画、VFX、声音 | 从用户拥有的 RAR 工程迁移，参考 `2026-08-12_RAR_Asset_Inventory.md` |
| `Content/ThirdParty/` | 已集中隔离的外部资产 | 从对应合法来源重新迁移 |
| `Content/Characters/` | Lyra/UE Mannequin 与第三人称动画依赖 | 从 `D:/UnrealProject/LyraStarterGame` 使用 Unreal Migrate 恢复 |
| `Content/Weapons/` | Lyra 第三人称 Rifle Mesh/动画依赖 | 从 `D:/UnrealProject/LyraStarterGame` 使用 Unreal Migrate 恢复 |
| `Content/Effects/`、`Content/PhysicsMaterials/` | Lyra 动画/武器引用的支持资产 | 随 Lyra 依赖迁移恢复 |
| `Content/SciFi_Props/` | 本地演示地图中的 Fab/SciFi Props 美术 | 从用户拥有的 Fab 资源重新添加/迁移 |
| `Content/Blueprints/Characters/Animations/FirstPerson/Unarmed/` | 已弃用的 RAR 空手动画副本 | 当前出生持枪原型不需要；仅恢复旧实验时重新迁移 |

## 保留在仓库中的项目资产

- `Content/Blueprints/AbilitySystem/`：Apecox AbilitySet、GE 等配置。
- `Content/Blueprints/Characters/`：项目 Character、AnimBP、Blend Space 等自建资产；被单独排除的外部动画副本除外。
- `Content/Blueprints/Weapons/`：Weapon Definition、Presentation Definition、拾取物和项目自建 Montage 配置。
- `Content/Blueprints/AI/`、`Content/Blueprints/Pickups/`：项目自建争夺点与拾取物蓝图。
- `Content/Blueprints/Maps/`：项目自建验证/演示地图。若地图引用被排除资源，本地必须先恢复对应依赖。

## 注意

项目自建 `.uasset/.umap` 通过 Git LFS 管理。公开仓库克隆完成后，还需要执行 `git lfs pull`，再恢复上述第三方资产，才能得到与本机演示工程一致的完整画面。
