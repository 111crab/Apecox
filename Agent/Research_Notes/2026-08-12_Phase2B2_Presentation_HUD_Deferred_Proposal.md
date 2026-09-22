# Phase 2B-2 射击表现与战斗 HUD 候选方案（延期讨论）

记录日期：2026-08-12  
状态：**候选方案，尚未由用户批准，不得据此直接实施。**

## 延期原因

Phase 2B-1 的自动步枪腰射 Hitscan 已完成单人和多人验证。用户希望先调查并补充合适的第一/第三人称射击美术资产，再进入射击表现实现，避免表现接口围绕不合适的占位资源反复调整。

## 候选目标

后续恢复讨论时，候选阶段为：

```text
Phase 2B-2：第一把步枪的射击表现与最小战斗 HUD 收口
```

候选范围：

- Owning Client 立即看到第一人称开火动作、枪口、声音和曳光。
- 其他客户端看到第三人称开火动作与枪口表现。
- 服务器确认的最终命中点播放 Impact 表现。
- HUD 显示准星、当前弹匣和服务器确认后的命中标记。
- 准星迁移到 UMG 后不再受 `showdebug` 替换 `AHUD::DrawHUD()` 的影响。

## 候选职责边界

- `UApecoxHitscanFireAbility` 只负责射击流程与发布预测/权威 GameplayCue，不直接持有具体 VFX、声音或 HUD。
- `UApecoxRangedWeaponInstance` 继续保存弹匣运行时真相，并向拥有者 UI 提供只读状态。
- `UApecoxWeaponPresentationDefinition` 保存稳定的武器表现配置；具体字段需结合最终选用资产重新审阅。
- Fire Cue 消费开火表现配置；Impact Cue 消费服务器最终命中位置和法线。
- HUD 只观察当前 WeaponInstance 与 Shot Confirmation，不修改弹药或命中真相。

## 尚未批准的候选命名

```text
UApecoxCombatHUDWidget : UUserWidget
FApecoxWeaponFirePresentationConfig
WBP_Apecox_CombatHUD
BP_ApecoxHUD
```

以上命名、字段、资产结构、FP/TP Montage 复用方式和 Niagara 方案均须在恢复该阶段时重新讨论确认。

