# RAR 移动、姿态与装备声音调查

更新日期：2026-09-11。来源为本机拥有的 RAR 工程只读资产和蓝图导出；没有修改或保存 RAR 商业资产。

## 脚步

`BP_IG_Character::Play Footsteps` 的 Tooltip 明确要求每帧调用。图中先计算 `ActorLocation - LastPosition` 的向量长度并加入 `Move Distance`，再更新 `Last Position`；只处理 `MOVE_Walking`，首次调用只初始化。累计值大于 `Footstep Distance` 时减去一个步距，角色未停止且未 Falling 才在角色位置播放 `Sound Cue Footstep`。

RAR 步枪玩家 CDO 把 `Sound Cue Footstep` 配为 `SC_IG_CH_Footsteps_Random`。该 Cue 从 25 个脚步波形中随机选择。2026-09-11 重新直接导出 `DT_RAR_AssaultRifle_Settings_Movement.Default` 后确认：Walk 为 400/145、Aim 为 230/155、Run 为 650/200、Crouch 为 230/120、RunFast 为 720/200（速度 cm/s / Footstep Distance cm）。早先记录的 150/100/200/100/320 是角色运行缓存/默认字段，不能作为这把步枪的数据表最终配置。该基础实现没有地面物理材质 Trace 或表面类型分流。

## 落地与姿态

- `Sound Cue Land` 为 `SC_IG_CH_Jump_End`，由角色 Landed 事件触发。
- `Sound Crouch Start` 为 `SC_IG_CH_Crouch_Start`，`Sound Crouch Stop` 为 `SC_IG_CH_Crouch_Stop`；RAR 使用 `SpawnSoundAttached` 附着到玩家相机。
- RAR 资产目录没有独立 Prone/Crawl SoundCue。Apecox 因此复用蹲伏布料 Cue，并把播放点放在成功姿态提交处，避免低顶失败或蹲趴交接重复播放。

## 装备

`DT_RAR_WEP_AssaultRifle_Sounds` 的 Equip 行引用 `SC_RAR_AssaultRifle_Equip`，Quick Equip 另有 `SC_RAR_AssaultRifle_Equip_Quick`。角色 Montage 表的 `Unholster` 行引用 `AM_RAR_FP_PCH_AssaultRifle_Equip`，武器 Montage 表的同一行引用 `AM_RAR_FP_WEP_AssaultRifle_Unholster`；对应原始 Sequence 均为 2.4 秒。人物厂商 Montage 在 1.584 秒含 `BP_IG_AN_Tags_Update` Notify，因此 Apecox 应从 `A_RAR_FP_PCH_AssaultRifle_Equip` 与 `A_RAR_FP_WEP_AssaultRifle_Unholster` 创建自有 Montage，并由自身生命周期控制同步播放和打断。

## 诊断证据

- `Saved/Diagnostics/Day2/rar/BP_IG_Character.t3d`
- `Saved/Diagnostics/Day2/rar/BP_IG_Character_CDO.t3d`
- `Saved/Diagnostics/Day2/rar/BP_RAR_AssaultRifle_Player_CDO.t3d`
- `Saved/Diagnostics/Day3/rar_combat/DT_RAR_WEP_AssaultRifle_Sounds.json`
- `Saved/Diagnostics/Day4/RAREquip/summary.txt`
- `Saved/Diagnostics/Day4/RAREquip/AM_RAR_FP_PCH_AssaultRifle_Equip.t3d`
- `Saved/Diagnostics/Day4/RAREquip/AM_RAR_FP_WEP_AssaultRifle_Unholster.t3d`
