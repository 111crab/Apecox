# ClaudeCode 修复 Prompt：Phase 1B AbilitySet 与输入生命周期

更新日期：2026-08-06

## 一、任务性质

这是对 Phase 1B 首次实现的定向修复。不要重新设计、不要创建 UE 资产、不要使用 MCP、不要执行 Git add/commit/push，也不要开始下一阶段。

先阅读：

1. `D:/UnrealProject/Apecox/Agent/Reviews/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Review.md`
2. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md`
3. `D:/UnrealProject/Apecox/Agent/Prompts/2026-08-06_Phase1B_AbilitySet_InputLifecycle_ClaudeCode_Prompt.md`
4. 当前 Phase 1B 全部源码与原实施报告。

保留用户和 Codex 的现有改动。禁止 reset、restore、checkout、clean。

## 二、必须修复的代码

### 1. 修复 Pressed/Held/Released

`UApecoxAbilitySystemComponent::AbilityInputTagPressed`：

- 匹配 InputTag 的 SpecHandle 同时 `AddUnique` 到 Pressed 和 Held。

`AbilityInputTagReleased`：

- 对每个匹配 Spec，无条件 `AddUnique` 到 Released。
- 从 Held 中 Remove；不要把是否生成 Released 依赖于 Contains(Held)。

保留 ProcessAbilityInput 的“先收集、统一激活、最后 Release”顺序。

### 2. 修复 OnAvatarSet 实际激活

`TryActivateAbilityOnAvatarSet` 在全部条件通过后必须调用：

```cpp
ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle)
```

并返回实际结果。这样 `OnGiveAbility()` 在“已有 Avatar 后授予”时能工作。

`UApecoxAbilitySystemComponent::TryActivateAbilitiesOnAvatarSet()` 只累计 helper 返回值，不得在 helper 成功后再调用第二次 `TryActivateAbility`。

把 `TryActivateAbilityOnAvatarSet`、Native `OnPawnAvatarSet` 和 `K2_OnPawnAvatarSet` 放回批准的 protected 区域；ASC 需要访问时用精确的 `friend class UApecoxAbilitySystemComponent`，不要扩大成公共 Blueprint API。

### 3. 修复 CanActivateAbility 的 ActorInfo

不要在 `CanActivateAbility(...)` 内通过依赖 `CurrentActorInfo` 的 Getter 查询 ASC。直接检查传入 `ActorInfo` 与其中的 AbilitySystemComponent，Cast 为 `UApecoxAbilitySystemComponent` 后检查 ActivationGroup。

项目 GA 运行在非 Apecox ASC 上属于配置错误：ensure 并返回 false，不要静默绕过并发组。

### 4. 修复并发组取消语义

将所有 `bCancelAll` 改回 `bReplicateCancelAbility` 的真实语义：

- `CancelActivationGroupAbilities(..., bool bReplicateCancelAbility)`。
- 内部 Predicate 只决定当前 Ability 是否匹配。
- 只有 Predicate 为 true 且 Ability `CanBeCanceled()` 时才调用 CancelAbility。
- `CancelAbility` 最后一个参数传 `bReplicateCancelAbility`。
- 绝不能因为该 bool 为 true 而取消不匹配的全部 Ability。

`AddAbilityToActivationGroup`：

- Independent 不取消任何东西。
- 新的 Replaceable 或 Blocking 都调用 `CancelActivationGroupAbilities(ExclusiveReplaceable, NewAbility, false)`。
- 不主动取消 Blocking；Blocking 应在 CanActivate 阶段阻止新排他 Ability。

加入：

- Group 合法性检查，禁止 `MAX`/越界访问。
- 计数溢出、下溢检查。
- Replaceable + Blocking 总数不得大于 1 的 ensure。

### 5. 实现正确的运行时 ChangeActivationGroup

API 改为：

```cpp
bool CanChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup) const;
bool ChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup);
```

并保持 BlueprintCallable。

语义：

- 只有已实例化且 Active 的 Ability 才能运行时切换。
- 同组返回 true。
- 当前不是 Blocking 时，目标组被 Blocking 阻止则返回 false；当前 Ability 自己是 Blocking 时允许离开该组。
- 不可取消 Ability 不能切到 ExclusiveReplaceable。
- 真正切换时：ASC Remove 旧组 -> ASC Add 新组 -> 更新实例的 ActivationGroup。

删除 `LogTemp`。失败返回 false；项目不变量可使用 ensure。

### 6. 修复并发计数数组

数组大小必须来自：

```cpp
static_cast<int32>(EApecoxAbilityActivationGroup::MAX)
```

并显式 `{}` 零初始化。为了访问完整枚举，可以在 ASC Header 包含项目 GA Header，整理掉多余 forward/include。删除错误的 `sizeof(...) == 4` 注释。

### 7. 修复 AbilitySet

- 在本地 `FGameplayAbilitySpec` 上先写有效 InputTag，再调用 GiveAbility。
- `RemoveFromAbilitySystem()` 和 `GrantToAbilitySystem()` 都要求 ASC 有效且 `IsOwnerActorAuthoritative()`；客户端安全返回。
- `AbilityLevel < 1`、`EffectLevel <= 0` 使用 ensure 并跳过。
- 保留 Instant GE 拒绝、SourceObject、撤销顺序和重复 Remove 安全性。

### 8. 修复 PredictionKey 回退

`AbilitySpecInputPressed/Released`：

- 优先使用 Primary Instance 的当前 ActivationPredictionKey。
- 没有实例时回退到 `Spec.ActivationInfo.GetActivationPredictionKey()`。
- 只在访问弃用字段附近使用 `PRAGMA_DISABLE_DEPRECATION_WARNINGS` / `PRAGMA_ENABLE_DEPRECATION_WARNINGS`，并注释这是兼容非预期/旧 Spec 的回退。
- 不直接调用 `ServerSetReplicatedEvent`。

### 9. 对称管理 Pawn Input Mapping Context

在 `AApecoxPlayerCharacter` 增加一个受保护的小型清理 helper，例如 `RemoveDefaultInputMappingContext()`：

- 只操作本地 PlayerController 的 Enhanced Input LocalPlayer Subsystem。
- Setup 添加前先移除同一个 Context，再添加，保证幂等。
- `UnPossessed` 在调用 Super 前清理。
- `EndPlay` 在调用 Super 前再次安全清理。
- 不新增 Controller 输入绑定或新资产。

### 10. 删除错误宏并修正文档注释

删除：

```cpp
#define APECOX_ABILITY_INPUT_PRESSED(...)
```

统一 InputComponent/Character 注释中的真实回调签名：

```cpp
void(const FInputActionValue&, FGameplayTag)
```

移除未使用 include，并保持中文学习型注释准确。

## 三、必须更新实施报告

覆盖更新：

`D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1B_AbilitySet_InputLifecycle_Report.md`

必须纠正：

- Pressed 同时进入 Held，Release 无条件产生 Released。
- OnAvatarSet helper 真正激活，而不只是检查。
- ChangeActivationGroup 新签名和运行时计数迁移。
- `bReplicateCancelAbility` 的真实含义。
- 计数数组来自 MAX，不写 `[4]`。
- InputTag 在 GiveAbility 前进入 Spec。
- Character 的 IMC 对称安装/移除。
- Owning Client、Authority、Simulated Proxy 的准确执行矩阵。
- `InvokeReplicatedEvent` 是本地 delegate 派发，不是“向所有客户端广播”。
- LocalPredicted GA 的激活请求由 GAS 发送给服务器；WaitInputRelease Task 在客户端 delegate 回调中按 SpecHandle + PredictionKey 把 Generic Event 上行并由服务器消费。

报告末尾列出本次修复的每个文件和完整编译状态。

## 四、验证

1. `git diff --check`。
2. 确认修改范围仅为 Phase 1B 源码、`DefaultInput.ini` 和原实施报告。
3. 运行：

```powershell
E:/UE_5.8/Engine/Build/BatchFiles/Build.bat ApecoxEditor Win64 Development -Project="D:/UnrealProject/Apecox/Apecox.uproject" -WaitMutex -NoHotReloadFromIDE -NoLink
```

4. 若 UE 已关闭且 DLL 未锁，可再做完整 Development Editor 构建；否则只如实报告 NoLink 结果，不要求用户关闭编辑器。

完成后停止，等待 Codex 二次审查。不要创建 IA、IMC、GA、AbilitySet 或蓝图资产。
