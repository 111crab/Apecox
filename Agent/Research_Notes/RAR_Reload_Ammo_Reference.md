# RAR 换弹、备用弹药与空仓行为调查

更新日期：2026-09-11。调查对象：`D:\UE_Resource\Realistic Assault Rifle Template\UE5\RAR\RAR.uproject`。全部操作为只读加载与文本导出，没有保存 RAR 包。

## 玩法规则

- `DT_RAR_AssaultRifle_Settings.Default`：`Can Reload Full=false`、`Auto Reload On Empty=false`、`Auto Reload On Empty Fire=false`、`Fire Rate Empty=450`。
- 弹匣 DataTable 容量为 30。Character 按 Ammo Type 保存 Starting/Current/Max 弹药池；通用 Character 默认配置中步枪弹药起始值为 150、最大 200。
- `Can Trigger Reload` 同时检查弹匣未满、角色有对应备用弹药、Bolt 状态与动作 Tag。Reload 开始后结束 Fire、Inspect、Run 等冲突动作。
- 普通与空仓换弹不是同一动画的参数分支，而是两组独立 Character/Weapon Montage。弹药在 Weapon Montage 的 `BP_IG_AN_Ammunition_Update` 通知点转移，不等到动画结束。

## 资源与时点

| 分支 | Character Sequence | Weapon Sequence | 动画总长 | 弹药提交点 | 声音 |
| --- | --- | --- | ---: | ---: | --- |
| 普通 | `A_RAR_FP_PCH_AssaultRifle_Reload` | `A_RAR_FP_WEP_AssaultRifle_Reload` | 2.3667 s | 1.101991 s | `SC_RAR_AssaultRifle_Reload` |
| 空仓 | `A_RAR_FP_PCH_AssaultRifle_Reload_Empty` | `A_RAR_FP_WEP_AssaultRifle_Reload_Empty` | 2.8 s | 1.424366 s | `SC_RAR_AssaultRifle_Reload_Empty` |

2026-09-17补充导出确认：两个Sound Cue都是Mixer。普通Cue同时混合`S_RAR_AssaultRifle_Reload`和`S_IG_Voice_Reloading`，空仓Cue同时混合`S_RAR_AssaultRifle_Reload_Empty`和`S_IG_Voice_ReloadingEmpty`。人物喊叫来自Cue的明确语音分支，不是换弹Montage或Apecox代码重复播放。Apecox不需要该语音，因此`DA_WeaponPresentation_Rifle`直接引用两条机械SoundWave，保留原换弹播放时点和中断逻辑；普通与空仓换弹均已由用户验证通过。

空仓扣扳机使用 `A_RAR_FP_PCH_AssaultRifle_Fire_Empty`，原 Character Montage 在约 0.025 秒播放 `SC_IG_WEP_Fire_Empty`；Weapon Montage DataTable 的 Fire-Empty 行为空。空仓反馈动画长约 1.1333 秒，声音长约 0.8146 秒，输入节流采用 450 RPM，即约 0.1333 秒一次。

RAR Character Montage 还含备用弹匣显隐、武器弹匣显隐和供应商动作 Tag Notify；Apecox 不迁移这些厂商 Montage。Apecox 从五条原始 Sequence 创建自有 `DefaultSlot` Montage，由 C++ 管理动作状态，并按 Weapon Montage 的实测通知时间提交弹药。

## Apecox 取舍

Apecox 当前只有一把步枪，第二把武器与完整弹药类型系统已经暂缓。因此本轮在 `UApecoxRangedWeaponInstance` 保存 OwnerOnly 的 150 发备用弹药，与 RAR Character 的步枪弹药池起始值一致；弹匣仍为 30。这个状态边界是临时但可迁移的：将来共享 Ammo Type 上线时，换弹事务改从 Inventory/PlayerState 弹药池取数，输入、HUD、动画和弹匣接口无需推翻。

只读证据位于 `Saved/Diagnostics/Day5/RAR_Reload/`，包含报告、DataTable JSON、Blueprint/Montage T3D 和资源元数据。
