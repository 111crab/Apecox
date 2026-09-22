# Phase 2A 第一把步枪拾取与装备闭环：验证收口

更新日期：2026-08-11

## 阶段结论

**Phase 2A 验证通过。**

用户已按 `Current_UE_Manual_Steps.md` 完成 UE 资产配置，并明确反馈所有验证均通过；未反馈新的运行错误或异常日志。

## 已验证范围

- 玩家空手出生，`E` 键通过 `InputTag.Interact` 发起拾取。
- 合法距离和视线内可以拾取，超距和遮挡会被服务器拒绝。
- 第一把进入 Primary，第二把进入 Secondary，普通武器槽满后不能继续吞掉 Pickup。
- Pickup 只消耗一次，争抢时不会给两名玩家重复发放实例。
- Host 与 Client 的私有库存互不污染。
- Owner 使用 FP 武器表现，其他客户端观察到 TP 武器表现。
- 死亡时装备和库存清空，新 Pawn 空手重生，其他玩家状态不受影响。
- 修复后的 FastArray、Registered Subobject、公开装备摘要和事务回滚在实际运行中没有暴露异常。

## 本阶段最终资产

- `/Game/Blueprints/Input/Actions/IA_Interact`
- `/Game/Blueprints/Input/DA_Apecox_InputConfig` 中的 Native `InputTag.Interact` 映射
- `/Game/Blueprints/Input/IMC_Apecox_Gameplay` 中的 `E -> IA_Interact`
- `/Game/Blueprints/Weapons/Rifle/DA_WeaponPresentation_Rifle`
- `/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle`
- `/Game/Blueprints/Weapons/Rifle/BP_WeaponPickup_Rifle`
- `/Game/Blueprints/Maps/L_Apecox_DevGym` 中的验证用 Pickup 实例

## 明确未包含

Phase 2A 没有实现开火、Hitscan、弹匣、弹药、ADS、换弹、切枪、伤害、GameplayCue 或持枪动画。这些不是验证缺失，而是后续 Phase 2B 及以后阶段的设计范围。

## 下一步

Phase 2B 先讨论第一把步枪的腰射 Hitscan 纵向切片，重点确定配置边界、武器 GA/AbilityTask 职责、瞄准数据与服务器验证、预测、弹匣运行时状态和命中表现，然后再生成代码设计。
