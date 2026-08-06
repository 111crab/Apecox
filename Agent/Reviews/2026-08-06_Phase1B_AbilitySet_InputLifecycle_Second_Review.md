# Phase 1B 第二轮代码审查

审查日期：2026-08-06
结论：**主要功能问题已修复，但仍需一次小范围收口。当前仍不要创建 UE 测试资产。**

## 已确认修复

- Pressed 同时进入 Pressed/Held，Released 无条件产生并移出 Held。
- OnAvatarSet helper 已真正调用 `TryActivateAbility`，ASC 不再二次激活。
- `CanActivateAbility` 已改用传入 ActorInfo 查询项目 ASC。
- 并发取消参数已恢复为 `bReplicateCancelAbility`，新的排他 Ability 只替换旧 Replaceable。
- AbilitySet 已在 GiveAbility 前写入 InputTag，Grant/Remove 均有 Authority guard。
- PredictionKey 已包含 Spec ActivationInfo 回退。
- IMC 已实现本阶段要求的对称清理。
- 错误输入宏已删除；NoLink 编译与 `git diff --check` 均通过。

## P2：非法并发组仍可造成字段与计数失配

文件：

- `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp`
- `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`

`CanChangeActivationGroup()` 没有拒绝 `MAX` 或底层值越界的枚举。当前路径可能发生：

1. 从旧组移除计数。
2. `AddAbilityToActivationGroup(MAX)` 拒绝添加。
3. `ChangeActivationGroup()` 仍把实例字段更新为 MAX。
4. Ability End 时无法从正确组移除，计数与字段永久失配。

同时，ASC 的 Add/Remove 只检查 `== MAX`，不能防止经 C++ cast 传入大于 MAX 的值；在取数组下标后会越界。`IsActivationGroupBlocked(MAX)` 目前也错误返回未阻止。

必须统一使用范围检查：`Index >= 0 && Index < static_cast<int32>(MAX)`。无效组在 CanChange 返回 false，在 ASC 查询中保守视为 blocked，在 Add/Remove 中 ensure 后返回。

## P2：CanActivateAbility 的无效 ActorInfo 分支返回 true

文件：`ApecoxGameplayAbility.cpp:127`

注释和已批准约束都要求 ActorInfo/ASC 无效时激活失败，但当前代码返回 true。即使 Super 通常会先拒绝，这个分支仍不应留下相反语义。改为 false。

## P2：并发计数缺少溢出防护

文件：`ApecoxAbilitySystemComponent.cpp:292`

批准 Prompt 要求在 `++` 前检查计数未达到 `INT32_MAX`。加入 check/ensure 后再递增；不要先溢出再检查。

## P3：TryActivateAbilityOnAvatarSet 可见性仍与批准设计不符

文件：`ApecoxGameplayAbility.h:83`

该 helper 目前仍位于 public 区域，报告却写为 protected。把它移到 protected，并继续通过已经存在的 `friend class UApecoxAbilitySystemComponent` 供 ASC 调用。

## P2：实施报告的网络矩阵仍然错误

根据 UE 5.8 `APlayerController::PlayerTick` 的源码注释，只有拥有 `PlayerInput` 的本地 PlayerController 才执行 PlayerTick/PostProcessInput。因此：

- 独立客户端的 Owning Client 执行 Enhanced Input 与 ProcessAbilityInput。
- Listen Server 主机的 PlayerController 同时是 Authority 和 Local，也执行它们。
- Dedicated Server 上代表远端玩家的 PlayerController 不执行这套本地输入处理。
- Simulated Proxy 没有对应的本地 PlayerController，不执行 ProcessAbilityInput。
- LocalPredicted Ability 的服务器副本由 GAS 激活 RPC 建立，不是服务器收到原始 IA 后再次 ProcessAbilityInput。
- Generic Replicated Event 不广播给 Simulated Proxy；WaitInputRelease 客户端任务按 SpecHandle + PredictionKey 上行服务器，服务器对应任务消费。
- Simulated Proxy 主要观察复制后的 Avatar 状态、属性、GameplayCue、蒙太奇/表现等，不应写成“接收 ReplicatedEvent 后激活 GA”。

报告中的 Authority/Owning Client/Simulated Proxy 表格及其前后说明必须按以上事实改写。

## 非阻塞残余风险

当前 Character 直接管理 IMC，满足本阶段同一种 Pawn/同一 Context 的验证。未来出现不同 Pawn 使用不同 Context、客户端旧 Pawn 延迟销毁时，应升级为 Controller/Pawn 初始化状态驱动的输入上下文所有权；本阶段不扩大实现。
