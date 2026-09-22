# 当前代码设计（完整武器 Prefab 方案前快照）

更新日期：2026-08-24

## 当前状态

当前没有可交给子代理执行的已批准代码方案。

项目刚刚将表现方向收束为：

```text
固定第一人称操控 + 远端第三人称世界表现
```

因此，2026-08-21 形成的 `Phase 2B-2C 模块化武器表现 Actor` 方案不再自动视为当前实施方案。其核心思想可以保留，但类名、生成策略、FP/TP 职责和资产字段必须按新边界重新审阅。

## 已冻结且不应被表现重构破坏的运行时边界

### 玩法真相

- `UApecoxRangedWeaponInstance`：弹匣和射击运行时状态。
- `UApecoxHitscanFireAbility`：射击意图、TargetData、预测、服务器校验和权威结算。
- `UApecoxWeaponStateComponent`：Shot Confirmation 与拥有者射击状态观察。
- `UApecoxEquipmentComponent`：当前装备生命周期、AbilitySet 授予/撤销和公开装备摘要。

### 表现入口

- `UApecoxWeaponPresentationDefinition`：武器表现配置，不保存弹药、伤害或命中真相。
- `GameplayCue.Weapon.Fire`：把已经成立的一发翻译为动画、VFX 和声音。
- `AApecoxPlayerCharacter::GetFirstPersonMesh()`：Owning Player 的 FP Arms 入口。
- `AApecoxPlayerCharacter::GetMesh()`：其他客户端观察的 TP Manny 入口。

### 网络口径

- Dedicated Server 不生成或播放视觉表现。
- Owning Player 只播放 FP 路径。
- 非本地控制的可视角色只播放 TP 路径，包括客户端上的 Simulated Proxy 和 Listen Server 主机观察到的远端玩家。
- 不为了表现新增第二套射击 RPC、扣弹或命中逻辑。

## 下一轮设计必须回答

1. 模块化武器使用表现 Actor、组件集合还是其他可预览装配单元。
2. FP 和 TP 是否共享同一个表现 Actor 基类，但使用不同 Blueprint 子类。
3. 哪个对象负责创建、销毁和按装备摘要刷新本地表现。
4. MuzzlePoint、WeaponMesh 和附属部件如何暴露，才能支持 Montage、VFX 和后续附件。
5. FP 首轮只接 RAR 时，最少需要哪些资产字段；哪些字段应该等 TP 或 Reload 再加入。
6. 如何保证这次只解决完整枪械装配与对齐，不顺带扩展 ADS、Reload 和配件系统。

## 仍可考虑的候选命名

以下命名来自旧方案，只是候选，尚未重新批准：

```text
AApecoxWeaponPresentationActor
PresentationRoot
WeaponMesh
MuzzlePoint
FirstPersonPresentationActorClass
ThirdPersonPresentationActorClass
```

在用户审阅新方案以前，不生成新的子代理 Prompt，不修改 C++。
