# RAR 实体子弹可视化调查

调查日期：2026-09-18。

## 结论

RAR 的步枪实体子弹没有独立 Niagara Ribbon、Beam 或拖尾系统。`BP_RAR_AssaultRifle_PROJ_Bullet` 继承 `BP_IG_Projectile`，可见部分是名为 `Projectile Mesh` 的静态网格组件：

- 网格：`/Game/InfimaGames/ArtCore/Weapons/Guns/Meshes/SM_IG_Projectile_Bullet`
- 材质实例：`/Game/InfimaGames/ArtCore/Weapons/Guns/Materials/MI_IG_Bullet_Glow`
- 父材质：`/Game/InfimaGames/ArtCore/MaterialLibrary/M_IG_Bullet_Glow`
- 材质覆盖：Additive、Unlit；`Intensity Emissive=100`

通用 Projectile 默认启用 Scale Update，初始缩放为 0，最大缩放为 33；`FC_PROJ_Scale_Max` 在 0 秒取 0、1 秒取 1。其视觉原理是让橙色发光弹体在飞行中变长/变大，从第一人称观察时形成短曳光，而非生成独立粒子尾迹。

## Apecox 适配

Apecox 的权威实体子弹速度为 15000–20000 cm/s，高于 RAR 的可见节奏。实现保留同一网格、材质和最大缩放，把增长时间压缩为 0.25 秒，以便在常见 10–30 米交战距离内看见弹体。碰撞、重力、伤害和复制移动仍由原有 `AApecoxWeaponProjectile` 负责，视觉组件不参与碰撞。

只迁入上述网格、材质实例和父材质三项最小依赖，没有复制 RAR 的 Projectile Blueprint、爆炸组件、表面表或缩放 Timeline。

原始提取结果保存在 `Saved/Diagnostics/PostClosureCombat/RARProjectile`。
