# 生命涌动 WaitInputRelease 不触发修复报告

*创建日期：2026-07-08*
*修复者：Claude*

## 问题

生命涌动（Vital Surge）按住 `2` 键后松开，技能不结算治疗。`UAbilityTask_WaitInputRelease` 的 `OnRelease` 回调从未被触发。

## 根因

`UAuraAbilitySystemComponent::AbilityInputTagReleased` 只调用了 `AbilitySpecInputReleased(AbilitySpec)`，但 `UAbilityTask_WaitInputRelease` 监听的是 `EAbilityGenericReplicatedEvent::InputReleased` —— 一个 GAS replicated event，不是 `AbilitySpecInputReleased` 回调。

UE 源码中 `UAbilitySystemComponent::AbilityLocalInputReleased` 做了两件事：
1. 调用 `AbilitySpecInputReleased(Spec)` 
2. 调用 `InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, ...)`

我们的实现在第 2 步缺失了，因此 `WaitInputRelease` 无响应。

## 修复

### 修改文件

`Source/Aura/Private/AbilitySystem/AuraAbilitySystemComponent.cpp` — `AbilityInputTagReleased` 函数

### 修改内容

```cpp
// 旧代码（只调用 AbilitySpecInputReleased）：
AbilitySpecInputReleased(AbilitySpec);

// 新代码（参考 AbilityLocalInputReleased）：
const bool bWasActive = AbilitySpec.IsActive();
AbilitySpecInputReleased(AbilitySpec);
if (bWasActive)
{
    FGameplayAbilityActivationInfo ActivationInfo = AbilitySpec.ActivationInfo;
    if (AbilitySpec.GetAbilityInstances().Num() > 0)
    {
        ActivationInfo = AbilitySpec.GetAbilityInstances().Last()->GetCurrentActivationInfoRef();
    }
    InvokeReplicatedEvent(
        EAbilityGenericReplicatedEvent::InputReleased,
        AbilitySpec.Handle,
        ActivationInfo.GetActivationPredictionKey()
    );
}
```

### 关键点

1. **`bWasActive` 记录**：在 `AbilitySpecInputReleased` 之前记录。`AbilitySpecInputReleased` 可能改变 Active 状态。

2. **`ActivationInfo` 获取方式**（参考 UE 5.8）：
   - 优先使用 `GetAbilityInstances().Last()->GetCurrentActivationInfoRef()`（instanced ability）
   - Fallback 到 `AbilitySpec.ActivationInfo`（non-instanced ability，已 deprecated）

3. **`InvokeReplicatedEvent` 的必要性**：
   - `WaitInputRelease` 是为多人设计的：客户端释放输入 → replicated event 通知服务端
   - Listen Server 下 Host 同时是客户端和服务端，但仍通过同一套 replicated event 触发
   - 不调用 `InvokeReplicatedEvent` 则 WaitInputRelease 永远收不到通知

## 编译结果

```
Result: Succeeded
Total execution time: 19.26 seconds
Exit code: 0
```

- 0 errors
- 仅旧代码 C4996 warnings（`DynamicAbilityTags`、`ActivationInfo`），不在本次修改范围

## 验证方式

1. 确保 `DA_VitalSurge` 已手动配置（资产字段）
2. 单人 PIE：
   - 按住 `2` 键 1 秒后松开 → 应触发 PartialCharge 持续治疗
   - 按住 `2` 键 3 秒后松开 → 应触发 FullCharge 瞬间治疗
   - Output Log 中应看到 `OnHoldInputReleased` 日志（TimeHeld=...）
3. 验证日志不再静默（之前松开后完全无日志输出）

## 未修改

- 未改 `InputConfig` / `InputMapping` / `DataAsset`
- 未改 `UAuraConfiguredActiveAbility`
- 未改其他文件
