# ClaudeCode 二次修复 Prompt：Phase 1B 最终收口

更新日期：2026-08-06

这是小范围最终收口。先阅读：

1. `D:/UnrealProject/Apecox/Agent/Reviews/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Second_Review.md`
2. 当前 Phase 1B 源码与实施报告。

不要重构、不要创建 UE 资产、不要使用 MCP、不要执行 Git 操作。

## 1. 统一验证 ActivationGroup 合法性

合法值范围是：

```cpp
0 <= static_cast<int32>(Group)
&& static_cast<int32>(Group) < static_cast<int32>(EApecoxAbilityActivationGroup::MAX)
```

可在 ASC `.cpp` 和 GA `.cpp` 各自使用文件内小型 helper，或使用不扩大公共 API 的等价实现。

- `CanChangeActivationGroup(NewGroup)`：无效时返回 false。
- `IsActivationGroupBlocked(Group)`：无效时 ensure，并保守返回 true。
- `AddAbilityToActivationGroup` / `RemoveAbilityFromActivationGroup`：取数组下标前验证，失败 ensure 并返回。
- 不得只判断 `Group == MAX`，必须覆盖底层值大于 MAX 的 C++ cast。
- `ChangeActivationGroup` 只有在 CanChange 通过后才能 Remove/Add/更新字段。

## 2. 修正 CanActivateAbility 无效 ActorInfo

将无效 `ActorInfo` 或无效 `AbilitySystemComponent` 分支改为 `return false`。保留非 Apecox ASC ensure + false。

## 3. 加入计数溢出防护

`AddAbilityToActivationGroup` 在 `ActivationGroupCounts[Index]++` 前使用 check/ensure 确认 `< INT32_MAX`。若使用 ensure，失败必须直接返回，不能发生整数溢出。

## 4. 修正 helper 可见性

把 `TryActivateAbilityOnAvatarSet(...)` 从 public 移入 protected。保留 `friend class UApecoxAbilitySystemComponent`，不要把 ActorInfo 生命周期入口暴露成公共 API。

## 5. 准确改写实施报告网络矩阵

覆盖更新原报告，不新建另一份报告。明确写清：

- PostProcessInput 只在本地 PlayerController 路径运行。
- Owning Client 运行输入管线；Listen Server 主机因同时本地控制也运行。
- Dedicated Server 的远端 PlayerController 与 Simulated Proxy 不运行本地 ProcessAbilityInput。
- 服务器的 LocalPredicted GA 副本由 GAS 激活 RPC 建立。
- WaitInputRelease Generic Event 仅在相关客户端/服务器任务之间按 SpecHandle + PredictionKey 上行与消费，不广播给 Simulated Proxy。
- Simulated Proxy 主要接收属性、Cue、动画/表现等复制结果，不写成接收 ReplicatedEvent 后激活 GA。

同步修正报告中 `TryActivateAbilityOnAvatarSet` 的 protected 可见性。

## 6. 检查与编译

运行：

```powershell
git diff --check
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE -NoLink
```

更新原报告中的编译结果。完成后停止，等待 Codex 最终审查。
