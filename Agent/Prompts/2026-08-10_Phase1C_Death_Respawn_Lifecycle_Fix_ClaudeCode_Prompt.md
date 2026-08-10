# Claude Code 执行 Prompt：Phase 1C 死亡/重生生命周期审查修复

你正在继续维护 Unreal Engine 5.8 C++ 项目：

`D:\UnrealProject\Apecox`

你的角色是**具体实施子代理**。本轮不是增加功能，而是修复 Codex 对 Phase 1C 的代码审查问题。

开始前必须完整阅读：

1. `D:\UnrealProject\Apecox\Agent\Reviews\2026-08-10_Phase1C_Death_Respawn_Lifecycle_Review.md`
2. `D:\UnrealProject\Apecox\Agent\00_Coordination\Current_Code_Design.md`
3. `D:\UnrealProject\Apecox\Agent\Prompts\2026-08-10_Phase1C_Death_Respawn_Lifecycle_ClaudeCode_Prompt.md`
4. `D:\UnrealProject\Apecox\Agent\Reports\2026-08-10_Phase1C_Death_Respawn_Lifecycle_Report.md`

审查结论是**未通过**。不要创建 UE 资产，不操作 UE 编辑器，不执行任何 Git 命令，不扩展护盾、UI、动画、伤害类型或比赛规则。

## 一、必须修复：VitalAttributeSet 的 GE 前后值链路

修改：

- `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h`
- `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp`

要求：

1. 正确覆写 `bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)`。
2. 先调用 `Super`；只有允许执行时，才在这里统一保存 `HealthBeforeAttributeChange` 与
   `MaxHealthBeforeAttributeChange`。
3. `PreAttributeChange()` 只保留数值钳制，删除旧值快照职责。
4. `PostGameplayEffectExecute()` 可继续钳制最终值，但不得让内部 `SetHealth()` 覆盖 GE 前快照。
5. 完成钳制后统一判断：
   - MaxHealth 实际变化时广播 `OnMaxHealthChanged`；
   - Health 实际变化时广播 `OnHealthChanged`；
   - Health 首次从正数跨入 `<= 0` 时广播一次 `OnOutOfHealth`；
   - MaxHealth 降低并连带压低 Health 时，也必须触发对应 Health 变化与 OutOfHealth 判断。
6. 所有相关委托广播完成后，再用最终 Health 更新 `bOutOfHealth`，防止回调中恢复生命导致门控错误。
7. `OnRep_Health()` 应令客户端本地 `bOutOfHealth` 与最终 Health 同步：Health > 0 时也要恢复 false；
   客户端仍不得由 OnRep 发送权威死亡事件。

可参考 Lyra 的 `ULyraHealthSet::PreGameplayEffectExecute/PostGameplayEffectExecute`，但不要引入
Damage/Healing 元属性、免疫、消息系统等本轮范围外内容。

## 二、必须修复：HealthComponent 生命周期和网络顺序

修改：

- `Source/Apecox/Public/Character/ApecoxHealthComponent.h`
- `Source/Apecox/Private/Character/ApecoxHealthComponent.cpp`

要求：

1. 类声明恢复合法写法：
   `UCLASS(Blueprintable, ClassGroup=(Apecox), meta=(BlueprintSpawnableComponent))`。
2. `InitializeWithAbilitySystem()` 真正幂等：
   - 同一个 ASC 且 VitalSet 已完整绑定时直接返回；
   - 其他已有绑定先 `UninitializeFromAbilitySystem()`，再建立新绑定；
   - 找不到 VitalSet 时不要留下“ASC 非空但未初始化完成”的半初始化状态。
3. 在 HealthComponent 上提供 `OnHealthChanged`、`OnMaxHealthChanged` 原生委托，并在两个 Handler 中
   原样转发 AttributeSet 的六个参数。不要在组件保存第二份 Health/MaxHealth。
4. `StartDeath()`、`FinishDeath()` 保持幂等并只允许 Authority 驱动；状态修改并广播后调用
   `GetOwner()->ForceNetUpdate()`。
5. `OnRep_DeathState()` 必须先按新状态重建本地死亡 Tag，再广播 Started/Finished，确保客户端委托
   与 Authority 端观察到相同 Tag 状态；保留 `NotDead -> DeathFinished` 合并复制的重放。
6. 组件拥有的 Dying/Dead loose tag 使用 `SetLooseGameplayTagCount(Tag, 0/1)` 精确管理。
7. 删除只写不读的 `bDeathTagsApplied`。
8. 死亡 GameplayEvent 的 `EventData.Target` 改为当前 AvatarActor，不得使用 ASC OwnerActor(PlayerState)。

## 三、必须修复：DeathAbility 的稳定边界

修改：

- `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
- `Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp`

要求：

1. `CancelAbilitiesByFunc` 恢复为 ASC private helper，不为 DeathAbility 暴露接收 `TFunction` 的内部 API。
2. DeathAbility 使用 GAS 自带的 `CancelAbilities(nullptr, nullptr, this)` 取消除自身实例外的活动 Ability，
   并继续调用 `ClearAbilityInput()`。
3. 每次 `ActivateAbility()` 开始时重置 `bDeathFinished = false`。
4. 在执行取消、Tag 或蓝图事件前，先严格验证 ActorInfo、ASC、Avatar 与 HealthComponent。
   缺少任一关键对象时使用清晰的 `ensureMsgf` 暴露配置错误并安全结束。
5. `FinishDeathAndEndAbility()` 是 BlueprintCallable，必须防护 `CurrentActorInfo`、Avatar 和组件失效。
6. 死亡是系统权威反应，不依赖消耗/冷却。删除 `CommitAbility()`，避免未来误配 Cost/Cooldown 后死亡流程被拒绝。
7. `EndAbility()` 只在死亡已经成功开始但尚未完成时补调用 `FinishDeath()`；不得把激活前验证失败
   也伪装成一次完整死亡。可增加明确的 `bDeathStarted` 状态并在每次激活重置。

## 四、必须修复：Character 的解绑、销毁和重生请求顺序

修改：

- `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
- `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`

要求：

1. `UninitializeAbilitySystem()` 严格恢复已批准顺序：
   - 先移除 Character 对 HealthComponent 的死亡委托；
   - 再 `HealthComponent->UninitializeFromAbilitySystem()`，此时 ASC Avatar 仍必须是当前 Pawn，
     让它能清除死亡 Tag；
   - 再清输入、取消 Ability、移除 Pawn AbilitySet；
   - 最后清 Avatar/ActorInfo 与 Character 的 ASC 缓存。
2. 保持旧 Pawn 晚解绑保护：如果 ASC Avatar 已经不是当前 Pawn，不得清除新 Pawn 状态或 Avatar。
3. `OnDeathFinished()` 不要在同步死亡委托栈内直接销毁/解绑；改为 next-tick 调用
   `DestroyDueToDeath()`。
4. `DestroyDueToDeath()`：
   - 所有端隐藏旧 Pawn；
   - Authority 缓存当前 Controller，并向 GameMode 请求该 Controller 的独立延迟重生；
   - Authority 调用 `DetachFromControllerPendingDestroy()`，确保 Controller 在重生到期前没有 Pawn；
   - 给旧 Pawn 设置约 0.1 秒的短 LifeSpan 兜底销毁。
5. 不要保留 5 秒 LifeSpan，不要假设 `RestartPlayer()` 会销毁旧 Pawn。
6. 若 `CachedController` 不再需要跨函数保存，删除该成员；若保留，解释生命周期必要性。

## 五、必须修复：GameMode 的每玩家独立倒计时

修改：

- `Source/Apecox/Public/Game/ApecoxGameMode.h`
- `Source/Apecox/Private/Game/ApecoxGameMode.cpp`

要求：

1. 恢复已批准接口：
   - `RequestPlayerRespawn(AController* Controller)`
   - `RestartPlayerAfterDelay(TWeakObjectPtr<AController> Controller)`
2. `PendingRespawnControllers` 使用 `TSet<TWeakObjectPtr<AController>>`，只负责同一 Controller 防重入。
3. 每个 Controller 创建自己的 Timer/Delegate；不得共享一个全局到期时刻，不得批量重生。
4. Timer 到期先从 Pending 集合移除，再验证弱引用、Authority 和 Controller 状态。
5. 只有 `Controller->GetPawn() == nullptr` 时才调用 `RestartPlayer()`。
6. 若到期时仍有 Pawn，使用 `ensureMsgf` 暴露生命周期错误并拒绝复用旧 Pawn。
7. 删除全局 `RespawnTimerHandle` 和“RestartPlayer 会销毁旧 Pawn”的错误注释。

## 六、验证与报告

完成后必须：

1. 运行 `git diff --check`。
2. 使用 UE 5.8 构建 `ApecoxEditor Win64 Development`。
3. 检查 Public/Private 目录仍然对称。
4. 不要执行 UE 编辑器资产操作。
5. 不要执行 `git add`、`git commit`、`git push`、`git stash` 或分支操作。
6. 新建修复报告：
   `D:\UnrealProject\Apecox\Agent\Reports\2026-08-10_Phase1C_Death_Respawn_Lifecycle_Fix_Report.md`
7. 报告使用中文，必须包含：
   - 修改文件列表；
   - 每个审查问题的具体修复；
   - 最终死亡/旧 Pawn 解绑/独立计时/新 Pawn 生成顺序；
   - 新增、删除或改名的类、成员变量、函数及用途；
   - 构建命令和真实结果；
   - 未做 UE 资产与未做 Git 操作声明；
   - 不得继续声称 `RestartPlayer()` 会自动销毁已有 Pawn。

停止条件：上述审查问题全部修复、`git diff --check` 通过、ApecoxEditor 构建成功并写完报告后立即停止，等待 Codex 二次审查。
