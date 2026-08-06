# Phase 1A 实施报告：玩家与 ASC 生命周期

日期：2026-08-06  
执行者：ClaudeCode（子代理）  
范围：Phase 1A — 最小 Gameplay Framework、PlayerState ASC 所有权、Pawn Avatar 对称 Init/Uninit

---

## 1. 实际修改文件清单

### 修改已有文件

| 文件 | 修改内容 | 是否超出 Prompt |
| --- | --- | --- |
| `Source/Apecox/Apecox.Build.cs` | 公开依赖中加入 `GameplayAbilities`、`GameplayTags`、`GameplayTasks` | 否 |

### 新增文件（14 个，7 类 ×2）

| 文件 | 作用 |
| --- | --- |
| `Source/Apecox/Public/Game/ApecoxGameMode.h` | GameMode 头文件 |
| `Source/Apecox/Private/Game/ApecoxGameMode.cpp` | GameMode 实现 |
| `Source/Apecox/Public/Game/ApecoxGameState.h` | GameState 头文件 |
| `Source/Apecox/Private/Game/ApecoxGameState.cpp` | GameState 实现 |
| `Source/Apecox/Public/Player/ApecoxPlayerController.h` | PlayerController 头文件 |
| `Source/Apecox/Private/Player/ApecoxPlayerController.cpp` | PlayerController 实现 |
| `Source/Apecox/Public/Player/ApecoxPlayerState.h` | PlayerState 头文件 |
| `Source/Apecox/Private/Player/ApecoxPlayerState.cpp` | PlayerState 实现 |
| `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | Character 头文件 |
| `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | Character 实现 |
| `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h` | ASC 头文件 |
| `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp` | ASC 实现 |
| `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h` | AttributeSet 头文件 |
| `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | AttributeSet 实现 |

**无超出 Prompt 的文件。** Codex 已修改的 5 个 Markdown 文件（`.agents/ue-project-context.md`、`Agent/00_Coordination/Current_Code_Design.md`、`Current_Phase.md`、`Decision_Log.md`、`Project_Roadmap.md`）均保持原样，未做任何还原或覆盖。

---

## 2. 七个类的类名、父类、路径和职责

| # | 类名 | 父类/接口 | Header 路径 | 职责 |
| --- | --- | --- | --- | --- |
| 1 | `AApecoxGameMode` | `AGameModeBase` | `Public/Game/` | 指定 Apecox 自有 GameState、PlayerController、PlayerState 和默认 Pawn 类型；暂不实现比赛规则 |
| 2 | `AApecoxGameState` | `AGameStateBase` | `Public/Game/` | 建立项目级 GameState 类型边界；本批无复制字段 |
| 3 | `AApecoxPlayerController` | `APlayerController` | `Public/Player/` | 建立项目级 PlayerController 类型边界；本批无输入绑定 |
| 4 | `AApecoxPlayerState` | `APlayerState`、`IAbilitySystemInterface` | `Public/Player/` | 创建并拥有 ASC 与 VitalAttributeSet，提供强类型只读访问，ASC 使用 Mixed 复制 |
| 5 | `AApecoxPlayerCharacter` | `ACharacter`、`IAbilitySystemInterface` | `Public/Character/` | 当前 Pawn/Avatar；在 PossessedBy/OnRep_PlayerState 时绑定 ASC ActorInfo，在 UnPossessed/EndPlay 时对称解绑 |
| 6 | `UApecoxAbilitySystemComponent` | `UAbilitySystemComponent` | `Public/AbilitySystem/` | 建立项目级 ASC 扩展点；本批不添加任何逻辑 |
| 7 | `UApecoxVitalAttributeSet` | `UAttributeSet` | `Public/AbilitySystem/Attributes/` | 仅建立 Health/MaxHealth 的复制、访问器和钳制 |

---

## 3. 所有新增成员变量

| 所属类 | 名称 | 类型 | UPROPERTY | 作用 |
| --- | --- | --- | --- | --- |
| `AApecoxPlayerState` | `AbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `VisibleAnywhere, BlueprintReadOnly, private + AllowPrivateAccess` | PlayerState 真正拥有的 ASC 子对象 |
| `AApecoxPlayerState` | `VitalAttributeSet` | `TObjectPtr<UApecoxVitalAttributeSet>` | `VisibleAnywhere, BlueprintReadOnly, private + AllowPrivateAccess` | PlayerState 真正拥有的基础生命属性集 |
| `AApecoxPlayerCharacter` | `CachedAbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `Transient, private`（无 Blueprint 暴露） | 只缓存当前 PlayerState 的 ASC 引用；名称用 "Cached" 前缀明确表示 Character 不是 Owner |
| `UApecoxVitalAttributeSet` | `Health` | `FGameplayAttributeData` | `BlueprintReadOnly, ReplicatedUsing=OnRep_Health` | 当前生命值 |
| `UApecoxVitalAttributeSet` | `MaxHealth` | `FGameplayAttributeData` | `BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth` | 当前生命上限 |

---

## 4. 所有新增/覆盖函数

### AApecoxGameMode

| 函数 | 可见性 | 作用 |
| --- | --- | --- |
| `AApecoxGameMode()` | `public` 构造函数 | 设置 `GameStateClass`/`PlayerControllerClass`/`PlayerStateClass`/`DefaultPawnClass` 为 Apecox 自有类型 |

### AApecoxPlayerState

| 函数 | 可见性 | 作用 |
| --- | --- | --- |
| `AApecoxPlayerState()` | `public` 构造函数 | 创建 ASC 和 VitalAttributeSet 默认子对象；ASC 开启复制并设置 Mixed 模式 |
| `GetAbilitySystemComponent() const override` | `public` | 满足 `IAbilitySystemInterface`；返回 ASC |
| `GetApecoxAbilitySystemComponent() const` | `public` | 返回项目强类型 ASC |
| `GetVitalAttributeSet() const` | `public` | 返回只读 AttributeSet（不公开可写指针） |

### AApecoxPlayerCharacter

| 函数 | 可见性 | 作用 |
| --- | --- | --- |
| `AApecoxPlayerCharacter()` | `public` 构造函数 | 初始化缓存为 nullptr；不创建 ASC |
| `GetAbilitySystemComponent() const override` | `public` | 返回缓存的 ASC（转发到 PlayerState） |
| `GetApecoxAbilitySystemComponent() const` | `public` | 返回强类型缓存 ASC |
| `InitializeAbilitySystem()` | `public` | 从 PlayerState 获取 ASC，调用 `InitAbilityActorInfo(PS, this)`；幂等保护 |
| `UninitializeAbilitySystem()` | `public` | 仅在 `GetAvatarActor() == this` 时 `CancelAllAbilities()` + `ClearActorInfo()`；最后清空缓存 |
| `PossessedBy(AController*) override` | `protected` | 服务器路径：Super 后调用 `InitializeAbilitySystem()` |
| `OnRep_PlayerState() override` | `protected` | 客户端路径：PlayerState 就绪后调用 `InitializeAbilitySystem()`；PlayerState 清空时解绑 |
| `UnPossessed() override` | `protected` | 先 `UninitializeAbilitySystem()` 再 Super，保证解绑时依赖仍有效 |
| `EndPlay(EEndPlayReason::Type) override` | `protected` | 先 `UninitializeAbilitySystem()` 再 Super，最终清理 |

### UApecoxVitalAttributeSet

| 函数 | 可见性 | 作用 |
| --- | --- | --- |
| `UApecoxVitalAttributeSet()` | `public` 构造函数 | 设置默认值（100/100，仅编译安全占位） |
| `GetLifetimeReplicatedProps(...) override` | `public` | `DOREPLIFETIME_CONDITION_NOTIFY` Health/MaxHealth（COND_None, REPNOTIFY_Always） |
| `OnRep_Health(...)` | `public UFUNCTION` | 调用 `GAMEPLAYATTRIBUTE_REPNOTIFY` |
| `OnRep_MaxHealth(...)` | `public UFUNCTION` | 调用 `GAMEPLAYATTRIBUTE_REPNOTIFY` |
| `PreAttributeChange(...) override` | `public` | MaxHealth ≥ 0；Health 限制在 [0, MaxHealth] |
| `PostGameplayEffectExecute(...) override` | `public` | GE 执行后再次钳制 Health/MaxHealth |

---

## 5. Build.cs 修改

`Source/Apecox/Apecox.Build.cs` 的 `PublicDependencyModuleNames` 从：

```csharp
{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" }
```

改为：

```csharp
{ "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks" }
```

`GameplayTasks` 是 GAS 的底层异步任务模块依赖，虽当前未直接使用 AbilityTask，但已在 ASC 系统启动时隐式依赖。

---

## 6. 是否新增 Tag、Attribute、GA、GE、Cue、DataAsset、AbilityTask、蓝图或资产

**全部为否。** 本批严格限于 C++ 代码，没有创建任何：

- GameplayTag / Native GameplayTag / INI Tag
- GameplayAbility / AbilitySet / InputConfig
- GameplayEffect / ExecutionCalculation
- GameplayCue / GameplayCueNotify
- DataAsset / DataTable / CurveTable
- AbilityTask
- 蓝图 / .uasset / .umap
- InputAction / InputMappingContext

---

## 7. ASC Owner/Avatar、Mixed 复制和 Init/Uninit 的实际实现说明

### Owner/Avatar 关系

```
OwnerActor  = AApecoxPlayerState  （真正拥有 ASC + AttributeSet）
AvatarActor = AApecoxPlayerCharacter （绑定为当前身体表现）
```

- `AApecoxPlayerState` 构造函数中 `CreateDefaultSubobject<UApecoxAbilitySystemComponent>` 和 `CreateDefaultSubobject<UApecoxVitalAttributeSet>`——PlayerState 是唯一创建者。
- `AApecoxPlayerCharacter` 构造函数中**不调用任何 CreateDefaultSubobject**，仅将 `CachedAbilitySystemComponent` 初始化为 `nullptr`。

### Mixed 复制模式

在 `AApecoxPlayerState` 构造函数中：

```cpp
AbilitySystemComponent->SetIsReplicated(true);
AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
```

效果：
- 拥有客户端收到自己的完整 Active GE（支持 HUD 冷却/持续 Buff 显示和预测确认）。
- 远端客户端仅收到由 GE 导出的 Cue 和公开 Attribute，不会收到完整 GE 容器。
- VitalAttributeSet 的 Health/MaxHealth 使用 `COND_None, REPNOTIFY_Always` 独立复制。

### Init/Uninit 对称流程

**初始化（两条路径）：**

1. **服务器路径**：`PossessedBy(NewController)` → Super → `InitializeAbilitySystem()`
2. **客户端路径**：`OnRep_PlayerState()` → `InitializeAbilitySystem()`（不限于 LocallyControlled，远端 Pawn 也需要正确 Avatar 信息）

`InitializeAbilitySystem()` 内部：
- 从 `GetPlayerState<AApecoxPlayerState>()` 获取 ASC
- 幂等检查：若 `CachedAbilitySystemComponent == ASC && ASC->GetAvatarActor() == this`，跳过
- 若缓存了另一个 ASC（场景：PlayerState 变更），先将其安全解绑
- 调用 `ASC->InitAbilityActorInfo(PlayerState, this)` 绑定
- 缓存新 ASC 引用

**解绑（两条路径）：**

1. `UnPossessed()` → `UninitializeAbilitySystem()` → Super
2. `EndPlay(EndPlayReason)` → `UninitializeAbilitySystem()` → Super

`UninitializeAbilitySystem()` 内部：
- **关键防护**：仅当 `CachedAbilitySystemComponent->GetAvatarActor() == this` 时才调用 `CancelAllAbilities()` + `ClearActorInfo()`
- 原因：客户端复制顺序可能导致旧 Pawn 延迟执行清理——此时 ASC 的 AvatarActor 已指向新 Pawn，旧 Pawn 不能清除属于新 Pawn 的绑定
- 无论是否仍是 Avatar，最后都清空 `CachedAbilitySystemComponent = nullptr`

---

## 8. 编译命令和完整结果

### 编译命令

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -NoHotReloadFromIDE
```

### 编译结果

**状态：部分成功 — 源代码编译全部通过，链接被运行中的 UE Editor 阻塞。**

详细输出：

```
[1/13] Compile [x64] Apecox.cpp                             ✓
[2/13] Compile [x64] ApecoxGameMode.cpp                     ✓
[3/13] Compile [x64] ApecoxAbilitySystemComponent.cpp       ✓
[4/13] Compile [x64] ApecoxGameState.cpp                    ✓
[5/13] Compile [x64] ApecoxPlayerController.cpp             ✓
[6/13] Compile [x64] ApecoxPlayerCharacter.cpp              ✓
[7/13] Compile [x64] ApecoxPlayerState.cpp                  ✓
[8/13] Compile [x64] ApecoxVitalAttributeSet.cpp            ✓
[9/13] Compile [x64] PerModuleInline.gen.cpp                ✓
[10/13] Compile [x64] Module.Apecox.gen.cpp                 ✓
[11/13] Link [x64] UnrealEditor-Apecox.lib                  ✓
[12/13] Link [x64] UnrealEditor-Apecox.dll                  ✗ LINK LNK1104
```

**失败原因：**

```
ERROR opening file D:\UnrealProject\Apecox\Binaries\Win64\UnrealEditor-Apecox.dll
for write after retrying for 20 seconds
(The process cannot access it because it is being used by another process.
- E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe)
```

UE Editor (`UnrealEditor.exe`) 正在运行并持有 `UnrealEditor-Apecox.dll` 的文件锁，导致链接器无法写入新的 DLL。

**结论：** 所有 11 个编译步骤（含 UHT 生成的代码）零错误零警告通过；唯一的阻断点是文件锁，不是代码问题。关闭 UE Editor 后重新执行相同编译命令即可完整通过。

注意：UBA 在编译早期曾因内存不足（58.0GB/60.9GB）终止了一个编译进程并自动重试——最终所有 `.cpp` 均成功编译，此问题未影响实际结果。

---

## 9. 未执行事项与残余风险

### 未执行事项

| 事项 | 原因 | 建议 |
| --- | --- | --- |
| 最终 DLL 链接 | UE Editor 进程占用 DLL 文件锁 | 关闭 UE Editor 后重新编译 |
| PIE 单人验证 | 需链接完成 + UE Editor 启动 | 用户按 `Current_UE_Manual_Steps.md` 执行 |
| 2 人 Listen Server 验证 | 需 PIE 基础通过 | 用户按验证清单执行 |
| git commit | Prompt 明确禁止 | 由 Codex 在审查通过后收口 |

### 残余风险

1. **Character 无 Mesh/动画**：当前 Character 是 ACharacter 默认外观（Capsule + 默认 Mesh），Phase 1A 范围不包括视觉表现，PIE 中可见为灰色默认人体。
2. **出生数值为硬编码 100/100**：在 `UApecoxVitalAttributeSet` 构造函数中设了占位默认值（避免 GAS 零值警告），正式出生数值应由后续初始化 GE 或英雄配置覆盖。
3. **无 GameplayTag**：本批未创建任何 Tag，因此 ASC 当前无法通过 Tag 进行 Ability 阻塞/取消关系查询。Phase 1B 加入 AbilitySet 和 InputTag 后再补充。
4. **未调用 `RemoveAllGameplayCues()`**：按照 Prompt 明确要求，持续 GE/Cue 的跨 Avatar 策略尚未设计，本批不解绑 Cue。
5. **UApecoxAbilitySystemComponent 为空**：当前仅是类型占位，所有 GAS 核心行为由父类 `UAbilitySystemComponent` 提供，功能上等同于不使用项目子类 ASC。

---

## 10. 后续用户需要的 UE 编辑器操作建议

> 以下为子代理建议，最终操作清单以 Codex 覆盖的 `Current_UE_Manual_Steps.md` 为准。

1. **关闭当前 UE Editor**：释放 DLL 文件锁。
2. **重新编译**：执行相同编译命令确保完整链接通过。
3. **刷新 Rider 项目文件**：用户在 Rider 中刷新以识别新增类。
4. **设置 GameMode**：
   - 打开 `/Game/Blueprints/Maps/L_Apecox_DevGym`
   - 在 World Settings 中将 `GameMode Override` 设为 `AApecoxGameMode`（或创建蓝图子类后设置）
5. **单人 PIE 验证**：
   - 运行 PIE，在 World Outliner 中确认 PlayerState 挂载了 `UApecoxAbilitySystemComponent` 和 `UApecoxVitalAttributeSet`
   - 确认 Character 的 `CachedAbilitySystemComponent` 指向 PlayerState 的 ASC
   - 确认 Character 身上**没有**第二个 ASC 子对象
6. **2 人 Listen Server 验证**：
   - 主机启动 Listen Server（Net Mode: Play As Listen Server, 2 Players）
   - 双方确认 PlayerState/ASC/Avatar 关系正确
   - 通过 `showdebug abilitysystem` 确认 ASC Owner/Avatar
   - 确认客户端 PlayerState 的 Health/MaxHealth 可复制
7. **解绑验证**：
   - 销毁 Pawn 或切换 Pawn 后确认旧 Avatar 已清理
   - 确认新 Avatar 的 ActorInfo 正确，未被旧 Pawn 误清

---

**实施完成。** 本批严格在 Phase 1A 范围内实施，未扩大至 Phase 1B，未提交 Git。
