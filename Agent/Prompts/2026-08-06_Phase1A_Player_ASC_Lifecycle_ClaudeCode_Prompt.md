# ClaudeCode 冷启动实施 Prompt：Phase 1A 玩家与 ASC 生命周期

你正在一个**全新的 ClaudeCode 对话窗口**中工作。不要假设自己拥有任何旧对话记忆；以下背景、必读文件、职责和实施边界都是本次任务的一部分。

## 1. 你的角色

这是一个多 Agent 协作项目：

- 用户负责审核架构、公开命名和设计，并执行需要视觉判断的 UE 编辑器操作与最终 PIE 验证。
- Codex 负责高层规划、架构设计、实施 Prompt、代码审查、验证方案和 Git 收口。
- 你（ClaudeCode）是**受约束的实施子代理**：只按照已经批准的设计编写代码、执行编译，并提交中文实施报告。

你无权自行扩大任务范围、重新设计公开 API、创建未批准 GameplayTag、修改 UE 资产、使用 MCP 操作编辑器或提交 Git。发现设计冲突或无法完成时，停止相关扩展，在报告中明确说明，不要自行发明替代架构。

## 2. 项目背景

- 项目：Apecox，多人英雄射击战斗原型，面向 UE 游戏客户端求职作品。
- 路径：`D:/UnrealProject/Apecox`
- Unreal Engine：UE 5.8，安装路径：`E:/UE_5.8`
- 主模块：`Apecox`
- 当前分支：`main`
- 当前代码只有 UE 自动生成的最小模块；Phase 0 编译、单人 PIE、2 人 Listen Server、Git/LFS 已完成。
- 官方 `GameplayAbilities` 插件已在 `.uproject` 中启用；`Build.cs` 尚未加入 GAS 模块依赖。
- 当前阶段只建立玩家、ASC 和 Pawn Avatar 的最小生命周期，不实现武器和技能内容。

长期架构原则：

- 玩家 ASC 由 PlayerState 持有，当前 Pawn/Character 作为 Avatar。
- 不做万能 GA 或万能配置；以后使用 GA 基类、稳定流程模板、AbilityTask、受控钩子和必要专用 GA。
- AbilitySet 是可撤销授予包，不是单技能完整定义。
- GameplayTag 只表达稳定跨系统语义；本任务完全不创建 Tag。
- 新业务 C++ 使用严格对称的 `Public/Private` 目录。

## 3. 开始前必读

按顺序阅读，读完再修改：

1. `D:/UnrealProject/Apecox/.agents/ue-project-context.md`
2. `D:/UnrealProject/Apecox/Agent/README.md`
3. `D:/UnrealProject/Apecox/Agent/00_Coordination/Working_Agreement.md`
4. `D:/UnrealProject/Apecox/Agent/00_Coordination/Subagent_Review_Checklist.md`
5. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Phase.md`
6. `D:/UnrealProject/Apecox/Agent/00_Coordination/Current_Code_Design.md`
7. `D:/UnrealProject/Apecox/Agent/02_CombatFramework/Apecox_Combat_Framework_Architecture_RFC.md` 的第 5 章和第 6.1 节；不要为本任务通读全部历史归档。

同时检查当前 `git status` 和现有源码。工作区中可能存在 Codex 更新的 Markdown，必须保留，不得还原或覆盖。

## 4. 本次唯一目标

实现 Phase 1A：

```text
OwnerActor  = AApecoxPlayerState
AvatarActor = 当前 AApecoxPlayerCharacter
Replication = Mixed
```

建立最小 Gameplay Framework、PlayerState 持有 ASC/Vital AttributeSet，以及 Character 对 ASC ActorInfo 的对称初始化和解绑。

## 5. 必须创建的类和路径

只创建以下七个类，不新增其他业务类：

| 类 | 父类/接口 | Header | CPP |
| --- | --- | --- | --- |
| `AApecoxGameMode` | `AGameModeBase` | `Source/Apecox/Public/Game/ApecoxGameMode.h` | `Source/Apecox/Private/Game/ApecoxGameMode.cpp` |
| `AApecoxGameState` | `AGameStateBase` | `Source/Apecox/Public/Game/ApecoxGameState.h` | `Source/Apecox/Private/Game/ApecoxGameState.cpp` |
| `AApecoxPlayerController` | `APlayerController` | `Source/Apecox/Public/Player/ApecoxPlayerController.h` | `Source/Apecox/Private/Player/ApecoxPlayerController.cpp` |
| `AApecoxPlayerState` | `APlayerState`、`IAbilitySystemInterface` | `Source/Apecox/Public/Player/ApecoxPlayerState.h` | `Source/Apecox/Private/Player/ApecoxPlayerState.cpp` |
| `AApecoxPlayerCharacter` | `ACharacter`、`IAbilitySystemInterface` | `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` |
| `UApecoxAbilitySystemComponent` | `UAbilitySystemComponent` | `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h` | `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp` |
| `UApecoxVitalAttributeSet` | `UAttributeSet` | `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h` | `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` |

`.h` 和 `.cpp` 的领域目录必须完全对称。不要通过 UE 编辑器创建类，也不要生成 IDE 工程文件。

## 6. 精确实现要求

### 6.1 `Apecox.Build.cs`

在合适的公开模块依赖中加入：

- `GameplayAbilities`
- `GameplayTags`
- `GameplayTasks`

保留现有依赖，不添加未使用模块。

### 6.2 `AApecoxGameMode`

- 继承 `AGameModeBase`。
- 构造函数中设置：
  - `GameStateClass = AApecoxGameState::StaticClass()`
  - `PlayerControllerClass = AApecoxPlayerController::StaticClass()`
  - `PlayerStateClass = AApecoxPlayerState::StaticClass()`
  - `DefaultPawnClass = AApecoxPlayerCharacter::StaticClass()`
- 不实现胜利、计分、死亡或复活逻辑。

### 6.3 `AApecoxGameState` 和 `AApecoxPlayerController`

- 只建立项目自有类型边界。
- 本任务不添加复制字段、输入绑定、UI、库存或 RPC。
- 不为了“看起来不空”而添加无业务意义的 Tick、BeginPlay 或占位变量。

### 6.4 `AApecoxPlayerState`

必须实现 `IAbilitySystemInterface`，并真正拥有以下默认子对象：

```cpp
TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent;
TObjectPtr<UApecoxVitalAttributeSet> VitalAttributeSet;
```

要求：

- 两个成员保持 `private`。
- 使用 `UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=..., meta=(AllowPrivateAccess="true"))`。
- 构造时通过 `CreateDefaultSubobject` 创建。
- ASC 开启复制并设置 `EGameplayEffectReplicationMode::Mixed`。
- 实现：
  - `virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override`
  - `UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const`
  - `const UApecoxVitalAttributeSet* GetVitalAttributeSet() const`
- 不直接公开可写 AttributeSet 指针；后续数值修改应通过 ASC/GameplayEffect。
- 不授予 Ability，不应用启动 GE，不创建 Tag。

### 6.5 `AApecoxPlayerCharacter`

必须实现 `IAbilitySystemInterface`，但**绝对不能创建 ASC 默认子对象**。

成员：

```cpp
UPROPERTY(Transient)
TObjectPtr<UApecoxAbilitySystemComponent> CachedAbilitySystemComponent;
```

它只是当前 PlayerState ASC 的缓存引用，不代表 Character 拥有 ASC。名称不得改成容易暗示所有权的 `AbilitySystemComponent`。

实现以下函数：

- `virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override`
- `UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const`
- `void InitializeAbilitySystem()`
- `void UninitializeAbilitySystem()`
- 覆盖 `PossessedBy(AController* NewController)`
- 覆盖 `OnRep_PlayerState()`
- 覆盖 `UnPossessed()`
- 覆盖 `EndPlay(const EEndPlayReason::Type EndPlayReason)`

生命周期要求：

1. `PossessedBy` 在服务器路径调用 `Super` 后初始化。
2. `OnRep_PlayerState` 在客户端 PlayerState 就绪后初始化；不要只对 `IsLocallyControlled()` 执行，远端 Pawn 后续也需要正确 Avatar 信息以承载 Cue/表现。
3. `InitializeAbilitySystem` 从 `AApecoxPlayerState` 取得强类型 ASC，调用：

```cpp
AbilitySystemComponent->InitAbilityActorInfo(ApecoxPlayerState, this);
```

4. 初始化必须幂等；如果已经绑定同一个 ASC/Avatar，不能重复产生副作用。
5. 如果缓存的是另一个 ASC，先安全解绑旧引用，再绑定新 ASC。
6. `UninitializeAbilitySystem` 只有在 `CachedAbilitySystemComponent->GetAvatarActor() == this` 时，才能对当前 Avatar 调用 `CancelAllAbilities()` 和 `ClearActorInfo()`。
7. 上述 Avatar 相等检查是硬性要求：客户端复制顺序可能让旧 Pawn 较晚执行清理，旧 Pawn 不能清掉已经绑定给新 Pawn 的 ActorInfo。
8. 无论是否仍是当前 Avatar，最后都清空本 Character 的缓存引用。
9. `UnPossessed` 和 `EndPlay` 必须使用这套统一解绑函数；保持调用顺序使解绑时依赖仍然有效。
10. 不调用 `RemoveAllGameplayCues()`；持续 GE/Cue 的跨 Avatar 策略尚未设计，本批不要猜测。

请为 ActorInfo 所有权、服务器/客户端双路径和旧 Avatar 防误清理写简短学习型注释，说明“为什么”，不要逐行翻译代码。

### 6.6 `UApecoxAbilitySystemComponent`

- 本批只建立项目级强类型扩展点。
- 不添加输入缓存、激活组、关系策略、RPC 或无依据的包装 API。
- 不要把 Character 的生命周期逻辑转移到一个含糊的万能函数中。

### 6.7 `UApecoxVitalAttributeSet`

只创建：

```cpp
FGameplayAttributeData Health;
FGameplayAttributeData MaxHealth;
```

要求：

- 两者使用 `BlueprintReadOnly` 和对应 `ReplicatedUsing`。
- 定义项目级宏 `APECOX_ATTRIBUTE_ACCESSORS`，为两项属性生成 GAS 标准访问器。
- 覆盖 `GetLifetimeReplicatedProps`，使用：

```cpp
DOREPLIFETIME_CONDITION_NOTIFY(..., COND_None, REPNOTIFY_Always)
```

- 实现 `OnRep_Health`、`OnRep_MaxHealth` 并调用 `GAMEPLAYATTRIBUTE_REPNOTIFY`。
- `PreAttributeChange`：保证 `MaxHealth >= 0`，`Health` 被限制在 `[0, MaxHealth]`。
- `PostGameplayEffectExecute`：对执行后的 `Health/MaxHealth` 做基本钳制。
- 不触发死亡，不广播 GameplayEvent，不生成 Cue，不添加 Shield/EvolutionProgress/Damage Meta Attribute。
- 不在 AttributeSet 中硬编码正式出生生命值；后续由初始化 GE 或英雄配置负责。

## 7. 编码和注释要求

- 遵守 UE 5.8 IWYU，头文件尽量使用前置声明，所需完整类型放在 `.cpp`。
- 所有 UObject 指针必须符合 UE GC/反射规范。
- 关键代码添加简短中文学习型注释，说明职责归属和设计原因；不要写大段论文式注释。
- 不抄入 Lyra 的版权注释或大段实现；只实现当前批准的轻量协议。
- 不修改与本任务无关的旧文件或格式。

## 8. 明确禁止

- 不创建 `UApecoxGameplayAbility`、AbilitySet、InputConfig、AbilityTask、GameplayEffect 或 GameplayCue。
- 不新增任何 Native GameplayTag、INI Tag 或临时 Tag。
- 不实现输入、相机、移动、Mesh、动画、武器、伤害、死亡、复活、护盾或 UI。
- 不创建或修改 `.uasset/.umap`。
- 不使用 MCP 操作 UE 编辑器。
- 不生成 `.sln` 或 Rider 项目文件。
- 不执行 `git commit`、`git push`、清理、还原或重置用户/Codex 改动。

## 9. 自检与编译

完成后先检查：

1. 新文件是否严格位于对称 Public/Private 路径。
2. Character 是否没有创建第二个 ASC。
3. PlayerState 是否是唯一 ASC/AttributeSet Owner。
4. ActorInfo 是否为 `PlayerState + Character`。
5. 解绑是否有 `GetAvatarActor() == this` 防护。
6. 是否没有超范围 Tag、Ability、资产和系统。
7. `git diff` 是否只包含本任务代码、Build.cs 和你的中文报告；保留 Codex 已修改的 Markdown。

然后执行：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

- 不要启动 UE GUI。
- 如果 UE/Live Coding/Rider 占用导致构建失败，不要强杀进程；记录完整原因。
- 编译成功不等于 PIE 验证成功，不得宣称多人功能已经验证。

## 10. 必须生成中文报告

报告路径：

`D:/UnrealProject/Apecox/Agent/Reports/2026-08-06_Phase1A_Player_ASC_Lifecycle_Report.md`

报告必须包含：

1. 实际修改文件清单，并说明是否超出 Prompt。
2. 七个类的类名、父类、路径和职责。
3. 所有新增成员变量：名称、类型、UPROPERTY 和作用。
4. 所有新增/覆盖函数：签名、可见性和作用。
5. Build.cs 修改。
6. 是否新增 Tag、Attribute、GA、GE、Cue、DataAsset、AbilityTask、蓝图或资产。
7. ASC Owner/Avatar、Mixed 复制和 Init/Uninit 的实际实现说明。
8. 编译命令和完整结果。
9. 未执行事项与残余风险。
10. 后续用户需要的 UE 编辑器操作建议，但不要代替 Codex 最终审查清单。

完成后停止。不要继续 Phase 1B，不要提交 Git。

