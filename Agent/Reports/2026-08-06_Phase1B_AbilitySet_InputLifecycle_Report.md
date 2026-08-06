# Phase 1B 实施报告：AbilitySet 与输入生命周期（含审查修复）

日期：2026-08-06
执行者：ClaudeCode（子代理）
状态：首轮实施 + Codex 审查后修复完成，-NoLink 编译通过

---

## 一、实际修改/新增文件清单

### 新增文件（10 个）

| 文件 | 职责 |
| --- | --- |
| `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h` | Native GameplayTag 声明 |
| `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp` | Native GameplayTag 定义 |
| `Source/Apecox/Public/AbilitySystem/Abilities/ApecoxGameplayAbility.h` | GA 基类 + 2 个枚举声明 |
| `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp` | GA 基类实现 |
| `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySet.h` | AbilitySet 数据结构 + 资产类 |
| `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySet.cpp` | AbilitySet Grant/Remove 实现 |
| `Source/Apecox/Public/Input/ApecoxInputConfig.h` | InputAction → GameplayTag 映射 DataAsset |
| `Source/Apecox/Private/Input/ApecoxInputConfig.cpp` | InputConfig 查找实现 |
| `Source/Apecox/Public/Input/ApecoxInputComponent.h` | EnhancedInputComponent 子类（模板绑定） |
| `Source/Apecox/Private/Input/ApecoxInputComponent.cpp` | RemoveBinds 实现 |

### 修改文件（7 个 + Config）

| 文件 | 修改内容 |
| --- | --- |
| `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h` | 输入缓存、并发组（MAX 数组）、Generic Replicated Event、OnAvatarSet |
| `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp` | 完整实现 ~370 行 |
| `Source/Apecox/Public/Player/ApecoxPlayerController.h` | 新增 `PostProcessInput` override |
| `Source/Apecox/Private/Player/ApecoxPlayerController.cpp` | PostProcessInput 实现 |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | InputConfig/IMC/AbilitySet 配置 + 运行时成员 + IMC 清理 helper |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | SetupPlayerInputComponent + IMC 对称 + Pawn AbilitySet |
| `Config/DefaultInput.ini` | `DefaultInputComponentClass` 改为 `ApecoxInputComponent` |

---

## 二、所有新增 C++ 类、父类、职责

| 类 | 父类 | 职责 |
| --- | --- | --- |
| `UApecoxGameplayAbility` | `UGameplayAbility` | Apecox 所有 GA 抽象基类；激活策略、并发组、强类型 Getter、运行时 ChangeActivationGroup |
| `UApecoxAbilitySet` | `UPrimaryDataAsset` | 不可变可撤销授予包；Authority-only Grant/Remove；InputTag 在 GiveAbility 前写入 Spec |
| `UApecoxInputConfig` | `UDataAsset` | InputAction → GameplayTag 映射 |
| `UApecoxInputComponent` | `UEnhancedInputComponent` | 模板绑定；Pressed=Triggered，Released=Completed+Canceled |

---

## 三、两个枚举

### EApecoxAbilityActivationPolicy
`OnInputTriggered` / `WhileInputActive` / `OnAvatarSet`（默认 `OnInputTriggered`）

### EApecoxAbilityActivationGroup
`Independent` / `ExclusiveReplaceable` / `ExclusiveBlocking` / `MAX`（默认 `Independent`，MAX 为哨兵值用于固定计数数组 `[static_cast<int32>(MAX)] = {}`）

---

## 四、关键公开 API

### UApecoxGameplayAbility
- `GetActivationPolicy()` / `GetActivationGroup()` — BlueprintCallable
- `GetApecoxAbilitySystemComponentFromActorInfo()` / `GetApecoxPlayerControllerFromActorInfo()` / `GetApecoxPlayerCharacterFromActorInfo()` — 强类型 Getter，空指针安全
- `CanChangeActivationGroup(NewGroup)` → `bool` — 仅已实例化且 Active 时可用；检查目标组阻塞/可取消性
- `ChangeActivationGroup(NewGroup)` → `bool` — ASC Remove 旧组 → Add 新组 → 更新实例字段
- `TryActivateAbilityOnAvatarSet()` — **protected**；条件检查 + 实际调用 `ASC->TryActivateAbility(Spec.Handle)`；OnGiveAbility 和 ASC AvatarSet 路径共用
- `OnPawnAvatarSet()` — **protected**（ASC 通过 `friend class UApecoxAbilitySystemComponent` 访问）
- `CanActivateAbility()` — 使用传入的 `ActorInfo->AbilitySystemComponent`（不依赖 `CurrentActorInfo`）；非 Apecox ASC 时 ensure + 返回 false

### UApecoxAbilitySystemComponent
- `AbilityInputTagPressed` — Pressed **同时** AddUnique 到 Pressed 和 Held
- `AbilityInputTagReleased` — **无条件** AddUnique 到 Released，从 Held 移除
- `ProcessAbilityInput` — 六步逐帧统一处理
- `IsActivationGroupBlocked` — Blocking 存在时阻止所有排他组
- `AddAbilityToActivationGroup` — Independent 不取消任何东西；新排他 Ability 只取消旧 `ExclusiveReplaceable`
- `CancelActivationGroupAbilities(Group, Ignore, bReplicateCancelAbility)` — `bReplicateCancelAbility` 原样传给 `CancelAbility`
- `AbilitySpecInputPressed/Released` — 优先 PrimaryInstance PredictionKey，无实例时在 `PRAGMA_DISABLE_DEPRECATION_WARNINGS` 内回退到 `Spec.ActivationInfo`
- `InvokeReplicatedEvent` — 本地 delegate 派发（不是"向所有客户端广播"）

### AApecoxPlayerCharacter
- `SetupPlayerInputComponent` — IMC 添加前先 Remove 同一 Context（幂等）
- `RemoveDefaultInputMappingContext()` — protected helper；UnPossessed/EndPlay 中调用
- `GrantPawnAbilitySets()` / `RemovePawnAbilitySets()` — Authority-only

### UApecoxAbilitySet
- `GrantToAbilitySystem` — Authority-only；InputTag 在 GiveAbility 前写入 Spec；AbilityLevel<1、EffectLevel<=0 时 ensure 并跳过
- `FApecoxAbilitySetGrantedHandles::RemoveFromAbilitySystem` — Authority-only；重复调用安全

---

## 五、两个 Native Tag

| C++ 名称 | 字符串 |
| --- | --- |
| `ApecoxGameplayTags::InputTag_Ability_Tactical` | `InputTag.Ability.Tactical` |
| `ApecoxGameplayTags::State_Input_AbilityBlocked` | `State.Input.AbilityBlocked` |

---

## 六、AbilitySet Grant/Remove 生命周期

### Grant（仅 Authority）
1. 校验 AbilityLevel ≥ 1、EffectLevel > 0
2. 本地构造 `FGameplayAbilitySpec`，**先在 Spec 上写入 InputTag**，再调用 `GiveAbility`
3. GE：拒绝 Instant；通过 `ApplyGameplayEffectToSelf(CDO, Level, Context)` 授予
4. AttributeSet：以 ASC Owner 为 Outer，`AddAttributeSetSubobject`

### Remove（仅 Authority）
1. `ClearAbility` → 2. `RemoveActiveGameplayEffect` → 3. `RemoveSpawnedAttribute` → 4. `Reset` 三组数组（重复调用安全）

---

## 七、输入调用链

```text
Q 按下 → Enhanced Input → IMC → IA_Ability_Tactical
→ UApecoxInputComponent: BindAction(IA, Triggered, this, &HandlePressed, InputTag)
→ AApecoxPlayerCharacter::HandleAbilityInputTagPressed(ActionValue, InputTag)
→ ASC::AbilityInputTagPressed(InputTag)
   └─ InputPressedSpecHandles.AddUnique + InputHeldSpecHandles.AddUnique  ← 修复 1

同一帧：
→ AApecoxPlayerController::PostProcessInput()
→ ASC::ProcessAbilityInput()
   ├─ 1. State.Input.AbilityBlocked? → Clear + return
   ├─ 2. Held → WhileInputActive 未激活 → 收集
   ├─ 3. Pressed → Spec.InputPressed=true；已激活→AbilitySpecInputPressed；OnInputTriggered→收集
   ├─ 4. 统一 TryActivateAbility（先去重）
   ├─ 5. Released → Spec.InputPressed=false；若仍活跃→AbilitySpecInputReleased
   └─ 6. 清空 Pressed/Released

松键：
→ HandleAbilityInputTagReleased → ASC::AbilityInputTagReleased(InputTag)
   └─ InputReleasedSpecHandles.AddUnique + InputHeldSpecHandles.Remove  ← 修复 1：无条件
→ 下一帧 ProcessAbilityInput 步骤 5 处理 Released
→ AbilitySpecInputReleased → InvokeReplicatedEvent(InputReleased, Handle, PredictionKey)
→ WaitInputRelease Task 在本地 delegate 回调中按 SpecHandle+PredictionKey 接收
```

**关键纠正**：
- `InvokeReplicatedEvent` 是**本地 delegate 派发**，不向所有客户端广播。
- 对 LocalPredicted GA：客户端的 GAS 激活请求发送到服务器 + WaitInputRelease Task 的 Generic Event 上行是**两条独立路径**，由 GAS 通过 SpecHandle+PredictionKey 关联。
- 远端客户端的输入不会作为原始 IA RPC 到达服务器；LocalPredicted Ability 由 GAS 激活 RPC 建立服务器副本，Generic Replicated Event 只在相关 Owning Client 与服务器任务之间传递。

---

## 八、Authority、Owning Client、Simulated Proxy 矩阵

**关键事实**（UE 5.8 `APlayerController::PlayerTick`）：
- 只有持有 `PlayerInput` 的本地 PlayerController 才执行 `PlayerTick` / `PostProcessInput`。
- 独立客户端的 Owning Client 运行 Enhanced Input 与 ProcessAbilityInput。
- Listen Server 主机的 PlayerController 同时是 Authority 和 Local，也运行它们。
- Dedicated Server 的远端 PlayerController **不**执行这套本地输入处理。
- Simulated Proxy **没有对应的本地 PlayerController**，不执行 ProcessAbilityInput。
- LocalPredicted GA 的**服务器副本由 GAS 激活 RPC 建立**，不是服务器"收到原始 IA 后再次 ProcessAbilityInput"。
- Generic Replicated Event **不广播**给 Simulated Proxy；WaitInputRelease 客户端任务按 SpecHandle+PredictionKey 上行服务器，服务器对应任务消费。
- Simulated Proxy 主要接收属性、GameplayCue、蒙太奇/表现等复制结果。**不**写成"接收 ReplicatedEvent 后激活 GA"。

| 环节 | Dedicated Server | Listen Server 主机 | Owning Client | Simulated Proxy |
| --- | --- | --- | --- | --- |
| Enhanced Input / IMC | 无 | 执行（本地 PlayerInput） | 执行 | 无 |
| PostProcessInput→ProcessAbilityInput | 无（远端 PC 无 PlayerInput） | 执行（本地 PC） | 执行 | 无 |
| AbilityInputTagPressed/Released | 不调用；激活 RPC 不等于输入 Tag 回调 | 本地调用 | 本地调用 | 无 |
| TryActivateAbility (LocalPredicted) | 通过 GAS 激活 RPC 建立服务器副本 | 本地权威执行 | 本地预测并请求服务器确认 | 通常不运行该 GA，只观察复制后的表现结果 |
| WaitInputRelease Generic Event | 服务器任务消费上行事件 | 本地 delegate + 服务器任务 | 本地 delegate→上行服务器 | **不接收** |
| GrantPawnAbilitySets | 执行（Authority） | 执行（Authority） | 不执行 | 不执行 |
| OnPawnAvatarSet | 对服务器已有 Ability 实例执行 | 执行 | 对拥有端已有 Ability 实例执行 | 通常没有 OwnerOnly AbilitySpec/实例，不作为输入 GA 执行端 |
| 并发组计数 | 对服务器活跃 GA 执行 | 对本地权威 GA 执行 | 对预测 GA 执行 | 通常不运行输入 GA，因此不维护这批 GA 的并发计数 |
| 属性/GameplayCue/表现 | 权威计算 | 权威计算 + 本地表现 | 本地预测 + 接收复制 | 接收复制结果 |

---

## 九、编译结果

```text
命令: Build.bat ApecoxEditor Win64 Development -NoLink

[1/9] ApecoxAbilitySet.cpp          ✓
[2/9] ApecoxAbilitySystemComponent.cpp ✓
[3/9] ApecoxGameMode.cpp            ✓
[4/9] ApecoxGameplayAbility.cpp     ✓
[5/9] ApecoxInputComponent.cpp      ✓
[6/9] ApecoxPlayerController.cpp    ✓
[7/9] ApecoxPlayerCharacter.cpp     ✓
[8/9] ApecoxPlayerState.cpp         ✓
[7/7] Module.Apecox.gen.cpp         ✓

Result: Succeeded
Total execution time: 14.59 seconds
```

**警告**：1 个 UE 5.8 弃用警告（`NonInstanced`）——合理使用。完整 DLL 链接需关闭 UE Editor。

---

## 十、审查修复逐项记录

### 首轮修复（10 项）

| # | 修复项 | 涉及文件 | 关键变更 |
| --- | --- | --- | --- |
| 1 | Pressed/Held/Released | ASC .cpp | Pressed 同时 AddUnique 到 Pressed+Held；Released 无条件 AddUnique+从 Held Remove |
| 2 | OnAvatarSet 实际激活 | GA .cpp / ASC .cpp | Helper 内部调用 `ASC->TryActivateAbility(Spec.Handle)`；ASC 遍历只累计返回值；OnPawnAvatarSet→protected+friend |
| 3 | CanActivateAbility ActorInfo | GA .cpp | 使用传入 `ActorInfo->AbilitySystemComponent`；非 Apecox ASC 时 ensure+返回 false |
| 4 | 并发组取消语义 | ASC .h/.cpp | `bCancelAll`→`bReplicateCancelAbility` 原样传给 CancelAbility；Independent 不取消；新排他→取消 Replaceable；MAX 越界检查；排他总数≤1 ensure |
| 5 | ChangeActivationGroup 运行时切换 | GA .h/.cpp | 新签名 `(NewGroup)→bool`；仅 Active+Instantiated 可用；检查组阻塞/可取消性；ASC Remove 旧组→Add 新组→更新字段；删除 LogTemp |
| 6 | 计数数组 | ASC .h | `int32 ActivationGroupCounts[static_cast<int32>(EApecoxAbilityActivationGroup::MAX)] = {}` |
| 7 | AbilitySet | AbilitySet .h/.cpp | InputTag 在 GiveAbility 前写入 Spec；Remove 加 Authority guard；AbilityLevel<1/EffectLevel≤0 ensure+跳过 |
| 8 | PredictionKey 回退 | ASC .cpp | 无实例时 PRAGMA 内回退到 `Spec.ActivationInfo` |
| 9 | IMC 对称管理 | Character .h/.cpp | 新增 `RemoveDefaultInputMappingContext()`；Setup 先 Remove 再 Add（幂等）；UnPossessed/EndPlay 中调用 |
| 10 | 删除错误宏 | GA .h / InputComponent .h | 删除 `APECOX_ABILITY_INPUT_PRESSED`；注释中回调签名修正为 `void(const FInputActionValue&, FGameplayTag)` |

---

## 十一、未执行操作

- 未创建任何 UE 资产（IA/IMC/GA/AbilitySet/BP/.uasset/.umap）
- 未使用 MCP 操作 UE 编辑器
- 未执行 Git add/commit/push
- 未修改 Codex 协作 Markdown
- 未创建正式技能、Cost、Cooldown、武器、伤害、死亡、UI、Tag Relationship Mapping

---

## 十二、二次修复（2026-08-06，最终收口）

| # | 修复项 | 涉及文件 | 关键变更 |
| --- | --- | --- | --- |
| 1 | 统一 ActivationGroup 范围验证 | ASC .cpp / GA .cpp | 各自文件内匿名 namespace `IsValidActivationGroup(Group)`；覆盖 `Index >= 0 && Index < MAX`（不仅是 `== MAX`）；`IsActivationGroupBlocked` 无效时 ensure+保守返回 true；`Add/Remove` 无效时 ensure+返回；`CanChangeActivationGroup` 无效时返回 false |
| 2 | CanActivateAbility 无效 ActorInfo → false | GA .cpp | `!ActorInfo \|\| !ASC` 分支从 `return true` 改为 `return false` |
| 3 | 计数溢出防护 | ASC .cpp | `AddAbilityToActivationGroup` 在 `++` 前 `ensure(Counts[Index] < INT32_MAX)`；失败立即返回 |
| 4 | TryActivateAbilityOnAvatarSet → protected | GA .h | 从 public 移入 protected；保留 `friend class UApecoxAbilitySystemComponent` |
| 5 | 网络矩阵修正 | 实施报告 | 重写第八节：明确 PostProcessInput 仅本地 PC 运行；Dedicated Server 远端 PC 不运行；Simulated Proxy 不接收 ReplicatedEvent；GA 服务器副本由 GAS 激活 RPC 建立 |

编译：**7/7，Result: Succeeded**（与首轮修复相同弃用警告）。`git diff --check` 无空白错误。

---

## 十三、首次运行修复（2026-08-06）

### 修复 1：PlayerState 默认 Avatar 误判

**文件**：`Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` — `InitializeAbilitySystem()`

**问题**：UE 5.8 的 `UAbilitySystemComponent::InitializeComponent()` 默认执行 `InitAbilityActorInfo(Owner, Owner)`。ASC 挂在 PlayerState 时，角色正式绑定前的 `Owner=PlayerState, Avatar=PlayerState` 是合法引导状态。但旧代码对此触发 `ensureMsgf` 并调用 `SetAvatarActor(nullptr)`，干扰正常初始化。

**修复**：在 `ExistingAvatar` 检查链中，最优先判断 `ExistingAvatar == ApecoxPS`——若命中，**不 ensure、不 SetAvatarActor(nullptr)**，直接落入后续 `InitAbilityActorInfo(ApecoxPS, this)`。代码旁已加入中文注释说明此为"合法默认过渡状态"。

**修改后的条件分支**：
1. `ExistingAvatar == nullptr` → 正常继续。
2. `ExistingAvatar == this` → 由幂等分支（`CachedAbilitySystemComponent == ASC && GetAvatarActor() == this`）提前返回。
3. `ExistingAvatar == ApecoxPS` → **新增**：静默通过，不清理。
4. `ExistingAvatar` 是其他 `AApecoxPlayerCharacter` → 调用旧 Character 的 `UninitializeAbilitySystem()`。
5. 其他未知 Actor → 保留 `ensureMsgf` + `SetAvatarActor(nullptr)` 防御。

### 修复 2：Ability 输入改为中性物理边沿

**文件**：`Source/Apecox/Public/Input/ApecoxInputComponent.h`

**问题**：旧代码 Pressed 绑定 `ETriggerEvent::Triggered`。若 `IA_Ability_Tactical` 上配置了瞬时 `Pressed` Trigger，UE 5.8 中 `UInputTriggerPressed` 只在输入跨过阈值的一帧返回 `Triggered`，持续按住时下一帧返回 `None`，Action 立即产生 `Completed`——表现为"按下后立刻 Release"。

**设计理由**：本项目允许同一个技能在不同角色配置下采用不同触发方式（瞬发/按住/蓄力/松发），因此技能玩法不能固化在 IA 或 C++ 触发事件选择中。Trigger 为空时：
- `Started` 精确表示 Digital Action 从未激活到开始激活的**一次物理边沿**。
- `AbilityInputTagPressed()` 只调用一次，但它同时将 Spec 放入 **Pressed 与 Held**，直到 `Completed`/`Canceled` 才移出 Held。

**修改**：
- Pressed 回调从 `ETriggerEvent::Triggered` → `ETriggerEvent::Started`。
- Released 回调继续使用 `ETriggerEvent::Completed + ETriggerEvent::Canceled`。
- 类注释重写：不再写 `Pressed = Triggered（one-shot）`，改为详细说明物理边沿语义和 IA Trigger 为空的要求。
- **不新增** `Hold`、`ReleaseToActivate` 等枚举或配置字段。

### 资产要求

- `IA_Ability_Tactical` 与对应 IMC Mapping 的 **Triggers 均须为空**（等待用户按当前 UE 手工清单修改并复核）。
- C++ 使用 `Started + Completed/Canceled` 采集物理边沿。
- AbilitySet 资产实例按父类规则命名为 **`DA_Phase1B_AbilitySet`**。

### 编译结果

```text
命令: Build.bat ApecoxEditor Win64 Development -NoLink

[1/3] ApecoxInputComponent.cpp      ✓
[2/3] Module.Apecox.gen.cpp         ✓
[3/3] ApecoxPlayerCharacter.cpp     ✓

Result: Succeeded
Total execution time: 14.54 seconds
```

`git diff --check` 无空白错误（仅 LF/CRLF 换行符标准化警告）。

### 最终运行验证

- 完整构建通过。
- 单人 PIE：按下只激活一次，按住期间保持 Active，松键时才 Release，连续重复正常。
- 两人 Listen Server：主机与客户端输入互不串联；客户端 LocalPredicted GA 在客户端与服务器各执行一次，松键事件两端均正常结束。
- 修复后未再出现 PlayerState 默认 Avatar 误判、WaitInputRelease、SpecHandle 或 PredictionKey 错误。

结论：Phase 1B 运行验证通过，可以提交收口。
