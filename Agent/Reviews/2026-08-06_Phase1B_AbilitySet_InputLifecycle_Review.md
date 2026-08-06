# Phase 1B 代码审查：AbilitySet 与输入生命周期

审查日期：2026-08-06
结论：**未通过，必须修复后重新审查。当前不要创建 UE 测试资产。**

## P1：输入闭环没有成立

文件：`Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`

- `AbilityInputTagPressed()` 只把 Handle 加入 `InputPressedSpecHandles`，没有加入 `InputHeldSpecHandles`。
- `AbilityInputTagReleased()` 又只有在 Handle 已位于 Held 时才生成 Released。
- 结果：`WhileInputActive` 永远没有 Held 输入，`WaitInputRelease` 也收不到松键事件。

正确语义：Pressed 同时加入 Pressed 与 Held；Released 无条件为匹配 Spec 加入 Released，并从 Held 移除。

## P1：OnAvatarSet 的“已有 Avatar 后授予”路径不会激活

文件：`Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp`

`TryActivateAbilityOnAvatarSet()` 当前只完成条件检查并返回 `true`，没有调用 `ASC->TryActivateAbility(Spec.Handle)`。`OnGiveAbility()` 忽略返回值，因此在 ActorInfo 已经就绪后新授予的 OnAvatarSet Ability 永远不会自动激活。

Helper 应在条件通过后真正调用 `TryActivateAbility` 并返回结果；ASC 遍历路径不得再重复调用第二次。

## P1：并发组取消参数与目标组实现错误

文件：

- `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
- `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp`

问题：

1. `bReplicateCancelAbility` 被误写成 `bCancelAll`。当它为 true 时，当前实现会绕过谓词取消全部活跃 Ability，而不是决定取消是否复制。
2. 新的 `ExclusiveBlocking` 激活时取消的是旧 Blocking，而不是旧 `ExclusiveReplaceable`。
3. 缺少多个排他 Ability 同时运行、数组越界和计数溢出的防护。

正确语义：任何新的排他 Ability 都只替换旧 `ExclusiveReplaceable`；现有 Blocking 会在激活检查阶段阻止新的排他 Ability；布尔参数只传给 `CancelAbility(..., bReplicateCancelAbility)`。

## P1：CanActivateAbility 查询了错误的 ActorInfo 来源

文件：`Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp`

`CanActivateAbility()` 已经收到本次检查对应的 `ActorInfo`，却通过依赖 `CurrentActorInfo` 的 Getter 获取 ASC。激活前检查可能运行在 CDO 或尚未建立实例上下文的路径上，这会使并发组检查被跳过。

应直接从函数参数 `ActorInfo->AbilitySystemComponent` 安全转换为 Apecox ASC；无效或类型错误时返回 false 并用 ensure 暴露项目不变量。

## P1：AbilitySet 未完整遵守 Authority 与 Spec 构造顺序

文件：`Source/Apecox/Private/AbilitySystem/ApecoxAbilitySet.cpp`

- `RemoveFromAbilitySystem()` 没有 Authority guard，与批准设计不符。
- InputTag 在 `GiveAbility()` 后才写入返回 Spec。正确顺序应是在本地 Spec 上先写 Dynamic Spec Source Tag，再 GiveAbility，使 OnGive 与初次复制都看到完整 Spec。
- AbilityLevel/EffectLevel 未按 Prompt 校验。

## P2：运行时并发组切换 API 与设计意图相反

文件：`ApecoxGameplayAbility.h/.cpp`

当前 `CanChangeActivationGroup()` 只允许未激活 Ability，`ChangeActivationGroup()` 只改枚举，不更新 ASC 计数。这样所谓“运行时切换”既无法在运行时使用，也会在未来造成计数不对称。

采用长期版本：API 接收 `NewGroup` 并返回 bool；仅允许已实例化且活跃的 Ability 切换；切换时先从旧组移除、加入新组，再更新实例字段。

## P2：并发计数数组定义错误

文件：`ApecoxAbilitySystemComponent.h`

`ActivationGroupCounts[4]` 与注释 `sizeof(EApecoxAbilityActivationGroup) == 4` 都不正确。枚举底层类型是 `uint8`；数组长度应来自 `EApecoxAbilityActivationGroup::MAX`，并显式零初始化。

## P2：Generic Replicated Event 缺少 PredictionKey 回退

文件：`ApecoxAbilitySystemComponent.cpp`

Primary Instance 缺失时当前代码使用默认无效 PredictionKey 继续派发。应按批准 Prompt：优先实例的 ActivationPredictionKey，无实例时在局部禁用弃用警告后回退到 `Spec.ActivationInfo`。`InvokeReplicatedEvent` 是本地 delegate 派发；客户端到服务器由 `WaitInputPress/Release` Task 的回调完成。

## P2：Pawn IMC 只添加、不移除

文件：`ApecoxPlayerCharacter.h/.cpp`

当前每次 Setup 都 AddMappingContext，但 Pawn 解绑/销毁时没有 RemoveMappingContext。换 Pawn 或重建输入时可能保留旧 Pawn 的输入上下文。应加入小型对称清理 helper，在本地 Pawn 解绑/销毁前移除；Setup 重装前也先移除同一 Context，保证幂等。

## P2：存在不安全且语义错误的未批准宏

文件：`ApecoxGameplayAbility.h:9`

`APECOX_ABILITY_INPUT_PRESSED(TAG)` 未被使用、没有空指针保护，并把“ASC 拥有某 Tag”错误等同于“输入处于 Pressed”。应删除。

## 报告必须纠正

实施报告中的网络矩阵和 Generic Replicated Event 描述不准确：

- 远端客户端输入不会先作为原始 IA RPC 到达服务器 PlayerController。
- Simulated Proxy 不处理本地玩家输入，也没有其他玩家的 PlayerController。
- `InvokeReplicatedEvent` 不会向所有客户端广播。
- 对 LocalPredicted Ability，客户端的 GAS 激活请求到服务器与 WaitInputRelease Task 的 Generic Event 上行是两条由 GAS 管理、使用 SpecHandle/PredictionKey 关联的路径。

同时修正 InputComponent 回调参数顺序、并发组 API、计数数组大小和 OnAvatarSet 的实际行为说明。

## 已通过的部分

- Public/Private 目录对称。
- Native Tag 的命名和注册方式符合批准设计。
- `DefaultInputComponentClass` 修改正确。
- InputConfig 的 Exact Tag 查询和 InputComponent 的 Triggered/Completed/Canceled 绑定方向正确。
- `PostProcessInput()` 的职责边界与调用位置正确。
- `git diff --check` 没有空白错误，仅有 Git 的 LF/CRLF 提示。
