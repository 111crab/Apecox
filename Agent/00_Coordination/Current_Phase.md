# 当前局部阶段

更新日期：2026-08-06

## 顶层阶段

Phase 1 - 玩家生命周期与 GAS 基线。

Phase 0 已完成并推送：`e115f8f chore: establish Apecox project baseline`。

Phase 1A 已完成并形成仅本地提交：

```text
86ed380 gas: establish player ASC lifecycle baseline
```

该提交已通过完整构建、单人 PIE 和两人 Listen Server，尚未 push。

## 当前局部任务

Phase 1B：AbilitySet、项目 GA 基类、Native GameplayTag 与按下/保持/松开输入闭环。**已完成。**

## 已批准范围

- 新建 `UApecoxGameplayAbility` 抽象基类。
- 建立 ActivationPolicy 与 ActivationGroup 两个封闭枚举。
- 新建完整 `UApecoxAbilitySet` 可撤销授予包。
- 新建 `UApecoxInputConfig` 和 `UApecoxInputComponent`。
- 扩展 ASC 的 Pressed/Held/Released 缓存、Generic Replicated Event、OnAvatarSet 和并发组。
- PlayerController 在 `PostProcessInput` 统一调用 ASC 输入处理。
- Character 配置并管理 Pawn 生命周期的 InputConfig、IMC 和 AbilitySet。
- Native Tag 仅加入：
  - `InputTag.Ability.Tactical`
  - `State.Input.AbilityBlocked`
- Tactical 默认绑定键盘 Q，不设计手柄输入。

## 当前执行顺序

1. Phase 1A 本地 commit，不 push。已完成。
2. Codex 更新 Phase 1B 代码设计和子代理 Prompt。已完成。
3. 用户运行 ClaudeCode Prompt。已完成。
4. Codex 审查源码、公开命名、GAS 输入事件、并发计数和生命周期。已完成三轮审查并通过。
5. 修复完成并编译通过后，Codex 覆盖 UE 人工操作清单。已完成。
6. 用户创建最小 Input/GA/AbilitySet/BP 资产。已完成。
7. 单人运行验证发现两个问题：瞬时 `Pressed` Trigger 导致按下后立刻产生 `Completed`；ASC 默认的 PlayerState Avatar 被误判为异常。已定位并修复。
8. 子代理完成运行时修复，Codex 复审通过。
9. IA/IMC Trigger 已改为中性物理边沿配置；AbilitySet 已按 DataAsset 规则命名为 `DA_Phase1B_AbilitySet`。
10. 完整构建、单人和两人 Listen Server 的按下、保持、松开验证全部通过。
11. 用户批准 Phase 1B 提交并 push，当前执行阶段收口。

## 成功标准

- Q 只激活带 `InputTag.Ability.Tactical` 的 AbilitySpec。
- OnInputTriggered 每次按下只激活一次，按住不会重复激活。
- GA 在 WaitInputRelease 中保持活跃，松键后本地预测端和服务器都结束。
- 两名玩家的输入、SpecHandle 和 PredictionKey 不串联。
- Avatar 解绑时清除 Held 输入并撤销 Pawn AbilitySet，不破坏 PlayerState Owner。
- 完整构建通过；代码范围内没有临时 GameplayTag、正式技能或武器逻辑。

## 当前待确认

无当前阻塞项。

后续技能配置设计仍需解决：`ActivationPolicy` 当前位于 GA CDO，如何让同一个 GA 类按不同角色/授予配置采用瞬发、按住或松发流程。该规则不得放回 IA。

## 当前下一步

今天结束工作。下一次先讨论 Phase 1C：最小死亡/复活/换 Pawn 与 Dedicated Server 生命周期闭环；方案经用户批准后再实施。
