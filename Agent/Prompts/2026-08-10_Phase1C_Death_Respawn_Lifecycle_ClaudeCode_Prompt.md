# ClaudeCode 执行 Prompt：Phase 1C 死亡、复活与换 Pawn 生命周期

你正在修改 Unreal Engine 5.8 C++ 项目：

```text
D:\UnrealProject\Apecox
```

你是具体实施子代理。Codex 负责架构规划和后续审查，用户负责关键命名审核与 UE 编辑器手工配置。不要自行扩大范围。

## 一、开始前必须阅读

按顺序完整阅读：

1. `Agent/README.md`
2. `Agent/00_Coordination/Working_Agreement.md`
3. `Agent/00_Coordination/Current_Phase.md`
4. `Agent/00_Coordination/Current_Code_Design.md`
5. `Agent/00_Coordination/Subagent_Review_Checklist.md`
6. 现有 Phase 1A/1B 相关源码和最近审查报告

`Current_Code_Design.md` 是本次实现的批准规格。实现前先核对真实源码；发现规格与 UE 5.8 API 或现有代码冲突时，停止该冲突部分并在报告中说明，不得悄悄更换架构。

## 二、实施目标

建立以下可运行链路：

```text
Instant GE 使 Health 归零
-> VitalAttributeSet 广播 OutOfHealth
-> HealthComponent 在 Authority 上发送 GameplayEvent.Death
-> DeathAbility 取消其他 Ability、清输入、进入死亡状态
-> Character 下一 Tick 解绑并销毁旧 Pawn
-> GameMode 等待 3 秒
-> RestartPlayer 生成新 Pawn
-> 原 PlayerState/ASC 绑定新 Avatar
-> 初始化 GE 恢复满生命并重新授予 Pawn AbilitySet
```

## 三、允许修改/新增的 C++ 文件

新增：

```text
Source/Apecox/Public/Character/ApecoxHealthComponent.h
Source/Apecox/Private/Character/ApecoxHealthComponent.cpp
Source/Apecox/Public/AbilitySystem/Abilities/ApecoxDeathAbility.h
Source/Apecox/Private/AbilitySystem/Abilities/ApecoxDeathAbility.cpp
```

修改：

```text
Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h
Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp
Source/Apecox/Public/AbilitySystem/Abilities/ApecoxGameplayAbility.h
Source/Apecox/Private/AbilitySystem/Abilities/ApecoxGameplayAbility.cpp
Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h
Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Public/Game/ApecoxGameMode.h
Source/Apecox/Private/Game/ApecoxGameMode.cpp
```

只有出现明确编译依赖时才允许修改 `Apecox.Build.cs`。不得修改第三方资产、旧项目、引擎源码或 `.uasset/.umap`。

## 四、强制实现约束

### 4.1 文件和命名

- 所有新增 `.h` 放 `Public`，`.cpp` 放 `Private`，两侧子目录完全对称。
- 使用批准的类名、函数名、成员变量和 Tag；不要创建同义包装类。
- C++ 注释使用中文，重点解释职责边界、网络原因和设计理由，避免逐行翻译代码。

### 4.2 AttributeSet

- 保留现有 Health/MaxHealth 钳制行为。
- 增加 `FApecoxAttributeEvent` 以及 Health、MaxHealth、OutOfHealth 委托。
- OutOfHealth 只在首次跨入 `Health <= 0` 时广播；Health 恢复后允许下一次死亡再次广播。
- OnRep 可以通知本地表现，但不得直接请求 GameMode、销毁 Pawn 或生成权威死亡 GameplayEvent。
- 不新增 Shield、Damage、Healing、Evolution 或伤害类型 Attribute。

### 4.3 HealthComponent

- 使用 `UActorComponent`，默认复制，禁用 Tick。
- 不保存第二份 Health/MaxHealth，只保存 ASC/VitalSet 引用和复制的 DeathState。
- `StartDeath` 与 `FinishDeath` 幂等。
- `Dying` 和 `Dead` Tag 互斥。
- `OnRep_DeathState` 必须支持复制合并导致的 `NotDead -> DeathFinished`。
- 只有 Authority 的 `HandleOutOfHealth` 发送 `GameplayEvent.Death`。
- 旧 Pawn 解绑时必须验证 ASC Avatar，不能清除新 Pawn 已建立的状态。

### 4.4 DeathAbility

- C++ 类为 `Abstract, Blueprintable`，以后由 `GA_Apecox_Death` 继承。
- 通过 `AbilityTriggers` 响应 `GameplayEvent.Death`。
- `ServerInitiated`、`ExclusiveBlocking`、`InstancedPerActor`。
- 清空 ASC 输入；取消其他活动 Ability 时忽略自身。
- `SetCanBeCanceled(false)` 后开始死亡。
- 提供 `K2_OnDeathStarted` 和 `FinishDeathAndEndAbility()`。
- 首版 `bAutoFinishDeath=true`；未来关闭后可由死亡蒙太奇结束时调用 Finish。
- 不播放动画、不使用 Delay Task、不访问 UI。

### 4.5 Character 与初始化 GE

- 构造时创建 HealthComponent 并绑定死亡委托。
- `PawnInitializationEffect` 是 Character 蓝图配置的 Instant GE 类。
- 只在 Authority、ASC ActorInfo 建立后应用；Context SourceObject 为当前 Character。
- 若配置的初始化 GE 不是 Instant，使用清晰的 `ensureMsgf` 拒绝应用，不能留下未跟踪 Active GE。
- Death Started 停止移动并关闭 Capsule 碰撞。
- Death Finished 只安排下一 Tick 销毁，不得同步 Remove Pawn AbilitySet。
- Authority 销毁旧 Pawn 前缓存 Controller；UnPossess/Detach 必须触发现有对称 ASC 清理。
- 旧 Pawn 使用短 LifeSpan，所有端隐藏；不要立即在死亡委托栈里 `Destroy()`。

### 4.6 GameMode 重生

- `RespawnDelaySeconds` 默认 3 秒，可在 GameMode Blueprint Defaults 修改。
- 只在服务器处理。
- 使用 Pending Controller 集合阻止重复 Timer。
- Timer 到期时 Controller 必须有效且当前没有 Pawn，才调用 `RestartPlayer()`。
- 不把 ASC、AttributeSet 或 AbilitySet 逻辑搬到 GameMode。

### 4.7 GA 死亡阻塞

- `UApecoxGameplayAbility` 默认把 `State.Death` 放入 GAS 原生 `ActivationBlockedTags`。
- 不在 ASC 输入循环中增加一串 Dying/Dead 特判。
- 不新增 `Ability.Behavior.SurvivesDeath`。

## 五、禁止事项

- 不操作 UE 编辑器和 MCP。
- 不创建、修改或删除 `.uasset/.umap`。
- 不创建 UI、动画、Cue、武器、护盾、库存、击杀消息或比赛规则。
- 不迁移 Lyra 源码；可以学习职责边界，但实现必须使用 Apecox 现有类型。
- 不提交、暂存、push Git。
- 不重新生成 Rider/Visual Studio 工程文件。
- 不修改 Agent 顶层规划文档；只写本次实施报告。
- 不顺手重构 Phase 1B 输入系统或无关代码。

## 六、实现后自检

至少检查：

1. Public/Private 目录对称。
2. 所有新增 UObject/Component/Ability 指针受 UPROPERTY 或生命周期保护。
3. 委托在初始化时绑定，在 Uninitialize/OnUnregister 中解除。
4. DeathState 注册 `DOREPLIFETIME` 并调用 Super。
5. Native Tag 使用声明/定义宏，不使用运行时字符串请求代替批准 Tag。
6. OutOfHealth、StartDeath、FinishDeath、RequestPlayerRespawn 都具有幂等保护。
7. Death Ability 不会取消自身。
8. 新 Pawn 绑定后，旧 Pawn 的清理路径不会把 ASC Avatar 置空。
9. Dedicated Server 路径不访问 LocalPlayer、Camera、Audio、HUD 或表现资产。
10. 代码注释说明为什么这么设计。

## 七、构建

优先执行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

如果编辑器、Rider 或 Live Coding 占用导致构建失败，记录真实错误，不要反复终止用户进程。若是代码编译错误，修复后重新构建，直到通过或明确阻塞。

## 八、报告

创建中文报告：

```text
Agent/Reports/2026-08-10_Phase1C_Death_Respawn_Lifecycle_Report.md
```

报告必须包含：

- 修改和新增文件列表。
- 新增类及父类。
- 每个新增成员变量、函数、枚举、委托和 Native Tag 的名称、类型、可见性和作用。
- 完整死亡/重生运行链路。
- Authority、Owning Client、Simulated Proxy 各自执行哪些部分。
- 与 Lyra 思想相同的边界，以及本项目主动删减的重型部分。
- 构建命令和完整结果。
- 未执行的 UE 手工资产操作清单，但不要实际操作。
- 风险、假设和需要 Codex 复审的点。
- 明确声明未 Git add/commit/push。

## 九、停止条件

满足以下条件后停止并等待 Codex 审查：

- 批准范围内代码全部完成。
- ApecoxEditor Development 构建通过，或给出无法自行排除的明确阻塞证据。
- 中文报告完成。
- 未进行 UE 资产操作和 Git 操作。

