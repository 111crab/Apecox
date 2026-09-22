# 换弹弹匣视觉交接修复报告

日期：2026-09-11

## 现象与根因

常态持枪的 `MagazineDefault` 一直存在，First Person FOV 120 只是让它更容易进入画面。普通换弹无法正确把弹匣送入弹匣槽、空仓换弹后段只有手势没有弹匣，是另一条独立问题。

RAR 使用两只弹匣：固定弹匣留在 `SOCKET_Magazine`，备用弹匣附着武器骨架 `SOCKET_Magazine_Reserve`。Weapon Reload Animation 驱动备用 Socket 的运动，Arms Animation 与其配套；Character Montage 的 Notify 再控制固定/备用弹匣显隐。Apecox 迁移了两条原始动画，但自建 Montage 没有供应商 Notify，表现 Actor 也没有第二只弹匣，因此动画只能移动手和 Socket，不能显示一个不存在的物体。

## 修改

- `AApecoxWeaponPresentationActor` 增加本地纯表现组件 `MagazineReserve`，附着 `SOCKET_Magazine_Reserve`。
- 生成 FP 表现后，从 Blueprint 精确组件 `MagazineDefault` 复制 Static Mesh 和材质，不要求手工维护第二份资产配置。
- 普通换弹使用 RAR 的 1.816144/2.266666 秒进行固定弹匣隐藏与最终恢复；空仓换弹使用 0.833333/2.533333 秒。
- 换弹自然完成、中断、卸枪或表现销毁时统一恢复固定弹匣并隐藏备用弹匣。
- 弹匣显隐仍是本地表现；服务器弹药提交、HUD 和射击判定没有改变。

## 验证

- `ApecoxEditor Win64 Development`：UHT、编译、完整链接成功。
- 全量 `Apecox.*` 自动化：62 项 Success，0 失败，0 未运行；61 项无警告，1 项仅含既有 `GameplayCueNotifyPaths` 回退警告。
- UE 只读资产检查确认 `BP_Apecox_RiflePresentation_FP` 实例具有 `MagazineDefault` 与继承的 `MagazineReserve`；后者父组件为 `WeaponMesh`，Socket 为 `SOCKET_Magazine_Reserve`，默认隐藏。
- 用户已完成全部人工清单：普通换弹、空仓换弹、重复换弹、动作结束恢复以及既有持枪/射击/ADS/检视回归均表现正常。

RAR 空仓换弹还在约 0.654401 秒生成一只物理废弃弹匣。本轮恢复的是手中备用弹匣及其入槽交接；物理废弃弹匣作为后续可选表现增强记录，不影响弹药事务或本轮验收。

验收期间曾出现 R 键失效。运行日志确认 `IA_Apecox_Reload` 的引用存在，但对应 `.uasset` 没有保存到磁盘；用户在当前会话重新加入 IA 后验证通过。关闭编辑器前仍须将 `IA_Apecox_Reload` 保存到 `Content/Blueprints/Input/Actions`，并保存引用它的 `IMC_Apecox_Gameplay` 与 `DA_Apecox_InputConfig`。
