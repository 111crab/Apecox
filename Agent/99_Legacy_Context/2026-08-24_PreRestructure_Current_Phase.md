# 当前局部阶段（2026-08-24 重构前快照）

更新日期：2026-08-21

## 顶层阶段

Phase 2 - 第一把步枪纵向切片。

## 已完成

Phase 2B-1 自动步枪腰射 Hitscan 已完成：输入、自动射击、弹匣、客户端预测、服务器逐发验证、权威伤害和 Shot Confirmation 均已通过单人及 Listen Server 双端验证。

Phase 2B-2A RAR 资产基线已完成：RAR 工程可在 UE 5.8 正常打开，第一人称 Arms、枪械动画、Mesh、Niagara 和 SFX 可用。RAR 角色 Fire Montage 使用 `SKEL_IG_Mannequin`，包含 `Overlay.Overlay Standing` 与 `Overlay.Overlay Aiming` 两条 Slot Track；厂商 `Update Tags` Notify 不进入 Apecox 玩法依赖。

Phase 2B-2B C++ 与局部修复已经完成最终复审：双视角 Fire Cue 表现入口、7 个武器表现字段、FP/TP 分流、Dedicated Server 早退、Camera/Arms 新层级和严格的 Montage 播放边界均已构建通过。

## 当前局部任务

Phase 2B-2C：模块化武器表现 Actor 与 RAR 完整枪械装配入口。

当前状态：**方案与公开命名已获用户批准，等待子代理实施 C++。Phase 2B-2B 的旧人工清单暂停执行，代码复审后再覆盖。**

## 已批准口径

- 第一轮只接入腰射开火表现，不实现 Reload、ADS、配件、后坐力、散布或相机震动。
- 第一人称正式使用 RAR 专用 Arms，摄像机不再附着于完整 Manny 的头部；第三人称继续使用 Manny 与 Rifle Pro 候选动作。
- `GameplayCue.Weapon.Fire` 是表现入口；现有 Hitscan GA、预测、服务器验证、扣弹和伤害流程不改。
- Owning Player 播放 FP Arms + FP Weapon；Simulated Proxy 播放 TP Character + 可选 TP Weapon。
- `UApecoxWeaponPresentationDefinition` 保存武器表现配置；`UApecoxEquipmentComponent` 管理当前装备的双视角 Mesh 并播放表现。
- 当前不新增独立 Weapon Presentation Component；等 Reload、ADS 等动作令表现职责明显膨胀时再拆分。
- 使用 `AApecoxWeaponPresentationActor` 承载每个视角的武器主体、默认模块与显式 `MuzzlePoint`；Actor 纯表现、非复制、无玩法真相。
- `UApecoxWeaponPresentationDefinition` 选择 FP/TP Presentation Actor Class，不再假设一个 SkeletalMesh 可以代表完整模块化枪械。
- RAR 默认弹匣、护木和机械瞄具由项目自建 Presentation Blueprint 组装，不依赖 RAR Demo Gameplay Blueprint。
- 第三方资产保留供应商命名空间；项目自建适配资产进入 `/Game/Blueprints/Weapons/Rifle`。
- GameplayCue Blueprint 由用户在 C++ 审查通过后手工创建；子代理不得通过 MCP 创建或修改 `.uasset`。

## 本轮不做

- 不修改 `UApecoxHitscanFireAbility` 的 TargetData、Trace、伤害、弹药、PredictionKey 或 Shot Confirmation 逻辑。
- 不使用 AnimNotify 决定一发子弹是否成立。
- 不迁移完整 RAR 工程，也不依赖厂商 `Update Tags` Notify。
- 不实现 Impact Cue 资产、弹孔、弹壳、曳光、Reload、ADS 或第三人称持枪 Locomotion。
- 不新增 GameplayTag、GA、GE、Attribute 或 AbilityTask。

## 当前执行顺序

1. 子代理按新 Prompt 实施 `AApecoxWeaponPresentationActor` 与 Equipment/Definition 调整并构建。
2. Codex 审查代码、公开 API、Actor 生命周期和网络表现边界。
3. 审查通过后覆盖 `Current_UE_Manual_Steps.md`，用户创建 FP/TP Presentation Blueprint 并装配 RAR 默认部件。
4. 用户执行 Standalone 与 Listen Server 双端验证。
5. 验证通过后完成 Phase 2B-2C 收口。

## 当前待确认

无。`AApecoxWeaponPresentationActor`、`FirstPersonPresentationActorClass`、`ThirdPersonPresentationActorClass`、`WeaponMesh` 和 `MuzzlePoint` 已获批准。

## 下一步

执行子代理 Prompt：

```text
Agent/Prompts/2026-08-21_Phase2B2C_Rifle_Modular_Presentation_Actor_ClaudeCode_Prompt.md
```
