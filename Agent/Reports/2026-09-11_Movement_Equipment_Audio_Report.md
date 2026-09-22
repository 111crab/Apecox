# 移动与装备声音实施报告

更新日期：2026-09-11。

## 结果

- `AApecoxPlayerCharacter` 按水平移动距离生成脚步；站立/ADS/冲刺/低姿态最终使用 200/170/250/100 cm。冲刺在 900 cm/s 下为 3.6 次/秒，与 RAR RunFast 的 720/200 时间节奏相同。停止、腾空、死亡时不生成脚步。
- `Landed` 负责落地声；`OnStartCrouch`、`OnEndCrouch` 和成功的 `SetProne` 负责姿态声。蹲姿与趴姿直接交接有一次性抑制，避免一项输入播放两次布料声。
- `UApecoxWeaponPresentationDefinition` 增加 `EquipSound` 及 Arms/Weapon Equip Montage。装备组件只在本地第一人称表现新建并成功附着，且两条 2.4 秒 Montage 都成功启动后播放声音；装备活动期间关闭左手 IK，开火、ADS、冲刺和蹲趴可以同步打断动画与声音。
- 本批未添加 Anim Notify 或物理材质分流；这与 RAR 当前基础脚步实现一致。

## 验证

- `ApecoxEditor Win64 Development` 构建成功：`Saved/Diagnostics/Day4/MovementAudio/build.log`。
- 全量 `Apecox.*` 自动化共 55 项：54 Success、1 Success With Warnings、0 Failed、0 Not Run。唯一警告是既有 `GameplayCueNotifyPaths` 回退。
- 报告：`Saved/Diagnostics/Day4/MovementAudio/Tests/index.json`；日志：`Saved/Diagnostics/Day4/MovementAudio/tests.log`。
- 新增 `Apecox.Movement.CombatRules.MovementAudioDistances`，覆盖四种步距和跨阈值后余量保留。
- 用户随后确认除拾枪缺动画和冲刺声偏慢外均通过。修正后最终构建成功；全量自动化 55 项通过、0 失败、0 未运行（54 Success＋1 个既有 GameplayCueNotifyPaths 警告）。最终证据位于 `Saved/Diagnostics/Day4/EquipSprintFixFinal/`。
- 用户完成两条自有装备Montage、表现配置与冲刺步距设置后，于2026-09-11确认本轮全部单人PIE验证成功。
