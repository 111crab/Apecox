# 换弹、备用弹药、空仓反馈与弹药 HUD 实施报告

更新日期：2026-09-11。

本轮在先调查 RAR 实际实现后，完成单步枪的弹药闭环。武器实例新增 OwnerOnly 备用弹药和换弹状态；Character 只向服务器请求换弹，Authority 在普通/空仓各自提交点原子转移弹药，并在动作结束后解除门控。客户端不能提交弹量、武器指针或换弹时间。

表现配置支持普通/空仓各一对 Arms＋Weapon Montage 和声音。换弹中关闭左手 FABRIK；自然结束允许 RAR 声音尾部继续，中断则一起停止。空仓扣扳机只产生节流后的 Arms 动作和空击声，不发射、不扣弹、不产生后坐。HUD 直接读取当前武器实例，在 ADS 中仍显示 `弹匣 / 备用`。

验证结果：`ApecoxEditor Win64 Development` UHT、编译与完整链接成功。全量 `Apecox.*` 自动化发现并执行 62 项，全部 Success、0 失败、0 未运行；新增 7 项覆盖默认时点、初始化、普通/空仓换弹、备用不足、提交幂等和提交前取消。报告位于 `Saved/Diagnostics/Day5/Reload/TestsGreen/index.json`，运行日志为 `Saved/Diagnostics/Day5/Reload/tests_green.log`。唯一成功附带警告是既有 GameplayCue 搜索路径回退。

UE 二进制资产仍由用户按当前人工清单配置，单人 PIE 视觉、声音、姿态与提交时点尚待验收。
