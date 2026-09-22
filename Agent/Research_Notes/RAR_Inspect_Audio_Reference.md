# RAR 检视动作与声音调查

更新日期：2026-09-11。来源是本机拥有的 RAR 工程，只读资产与蓝图导出；没有修改或保存 RAR 商业资产。

## 实际资产与时间

- 人物/第一人称手臂：`AM_RAR_FP_PCH_AssaultRifle_Inspect`，长度约 6.3667 秒，Rate Scale 1；Slot 为 `Overlay Standing` 和 `Overlay Lowered`。
- 第一人称枪械：`AM_RAR_FP_WEP_AssaultRifle_Inspect`，长度约 6.3667 秒，Rate Scale 1；Slot 为 `Standing`、`Aiming` 和 `Running`。
- 声音：`SC_RAR_AssaultRifle_Inspect`，直接播放 `S_RAR_AssaultRifle_Inspect`，时长约 6.4 秒；能力数据中的声音延迟为 0。

## 状态与打断关系

- RAR 检视开始时拒绝正在瞄准、冲刺、检查弹匣、查看手表或已经检视的状态。
- 开火、瞄准、冲刺和其他主要武器动作会移除/取消 Inspect 状态，因此检视可以被新的战斗输入立即打断。
- 普通移动没有被 Inspect 的激活条件阻止；检视动画通过 Montage Overlay 覆盖当前基础姿势。
- RAR 依赖供应商 Gameplay Tag/Notify 收尾。Apecox 不复制该依赖，以 Arms Montage 的原生结束委托收尾，并让两条 Montage 和声音作为一个事务停止。

## 左手 IK 结论

人物检视 Montage 含 `Alpha Weapon Grip` 曲线，中段值约为 `-1`。这代表检视动作会撤掉常规武器握持层，使左手可以离开护木完成动作。若 Apecox 继续让已验收的 FABRIK 保持 Alpha=1，左手会被目标点拉回护木，产生手臂扭曲、穿模或两条动画不同步的观感。本批以 Character 的检视状态在整段动作期间关闭 `LeftHandIKAlpha`，结束或被打断后恢复。

## 诊断证据

只读脚本和导出位于 `Saved/Diagnostics/Day4/inspect_rar_inspect.py` 与 `Saved/Diagnostics/Day4/RAR_Inspect/`。其中包含两条 Montage、SoundCue 的 T3D 及 `report.json`。
