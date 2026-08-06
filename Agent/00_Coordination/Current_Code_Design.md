# 当前代码设计

更新日期：2026-08-06

## 当前任务

Phase 1A：建立最小 Gameplay Framework、PlayerState ASC 所有权和 Pawn Avatar 对称绑定。

本设计已经由用户批准。子代理只能按本文和对应 Prompt 实施，不得自行加入 AbilitySet、InputTag、死亡、复活、护盾、技能、武器或 UE 资产。

## 核心所有权

```text
OwnerActor  = AApecoxPlayerState
AvatarActor = 当前 AApecoxPlayerCharacter

AApecoxPlayerState
├── UApecoxAbilitySystemComponent
└── UApecoxVitalAttributeSet
```

- `AApecoxPlayerState` 是 ASC 和 AttributeSet 的真正所有者，保证它们可以跨 Pawn 生命周期存在。
- `AApecoxPlayerCharacter` 不创建第二个 ASC；它只负责把当前 Pawn 绑定为 Avatar，并向依赖 Avatar 的系统转发 ASC 访问。
- ASC 使用 `Mixed` 复制模式。
- 初始化和解绑必须成对，并且只有当前 Avatar 才能解除自己的 Avatar 关系。
- 正常换 Pawn 时保留 PlayerState OwnerActor，只把 AvatarActor 置空；仅 OwnerActor 已经失效时才使用 `ClearActorInfo()`。

## 已批准类与路径

| 类 | 父类/接口 | Header | CPP | 本批职责 |
| --- | --- | --- | --- | --- |
| `AApecoxGameMode` | `AGameModeBase` | `Source/Apecox/Public/Game/ApecoxGameMode.h` | `Source/Apecox/Private/Game/ApecoxGameMode.cpp` | 指定 Apecox 的 GameState、PlayerController、PlayerState 和默认 PlayerCharacter 类型；暂不实现比赛规则 |
| `AApecoxGameState` | `AGameStateBase` | `Source/Apecox/Public/Game/ApecoxGameState.h` | `Source/Apecox/Private/Game/ApecoxGameState.cpp` | 建立未来公共比赛状态边界；本批不添加复制字段 |
| `AApecoxPlayerController` | `APlayerController` | `Source/Apecox/Public/Player/ApecoxPlayerController.h` | `Source/Apecox/Private/Player/ApecoxPlayerController.cpp` | 建立本地输入、UI 和未来私有玩家状态入口；本批不绑定输入 |
| `AApecoxPlayerState` | `APlayerState`、`IAbilitySystemInterface` | `Source/Apecox/Public/Player/ApecoxPlayerState.h` | `Source/Apecox/Private/Player/ApecoxPlayerState.cpp` | 创建并拥有 ASC 与 Vital AttributeSet，提供强类型只读访问入口 |
| `AApecoxPlayerCharacter` | `ACharacter`、`IAbilitySystemInterface` | `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h` | `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp` | 当前 Pawn/Avatar；在服务器占有和客户端 PlayerState 就绪时绑定 ASC，在解除占有或销毁时解绑 |
| `UApecoxAbilitySystemComponent` | `UAbilitySystemComponent` | `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h` | `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySystemComponent.cpp` | 建立 Apecox 的项目级 ASC 扩展点；本批不加入输入缓存或激活组 |
| `UApecoxVitalAttributeSet` | `UAttributeSet` | `Source/Apecox/Public/AbilitySystem/Attributes/ApecoxVitalAttributeSet.h` | `Source/Apecox/Private/AbilitySystem/Attributes/ApecoxVitalAttributeSet.cpp` | 只建立 `Health` 与 `MaxHealth` 的复制、访问器和基本钳制，不触发死亡 |

所有新增 `.h/.cpp` 必须严格位于对称的 `Public/Private` 领域路径。

## 公开成员与函数

### `AApecoxPlayerState`

成员：

| 名称 | 类型 | 可见性 | 作用 |
| --- | --- | --- | --- |
| `AbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `private`，`VisibleAnywhere`、`BlueprintReadOnly` | PlayerState 真正拥有的 ASC |
| `VitalAttributeSet` | `TObjectPtr<UApecoxVitalAttributeSet>` | `private`，`VisibleAnywhere`、`BlueprintReadOnly` | PlayerState 真正拥有的基础生命属性集 |

函数：

- `virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override`：满足 GAS 标准接口。
- `UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const`：提供项目强类型 ASC。
- `const UApecoxVitalAttributeSet* GetVitalAttributeSet() const`：提供只读属性集访问；玩法修改必须经过 ASC/GE。

构造时创建两个默认子对象，开启 ASC 复制并设置 `EGameplayEffectReplicationMode::Mixed`。同时把 PlayerState 的 `NetUpdateFrequency` 设置为 `100.0f`，避免引擎默认 `1 Hz` 使 ASC/Attribute 更新产生明显延迟。

### `AApecoxPlayerCharacter`

成员：

| 名称 | 类型 | 可见性 | 作用 |
| --- | --- | --- | --- |
| `CachedAbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `private`、`Transient` | 只缓存当前 PlayerState ASC，不代表 Character 拥有 ASC |

函数：

- `virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override`。
- `UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const`。
- `void InitializeAbilitySystem()`：读取 `AApecoxPlayerState`；如果目标 ASC 仍绑定另一个 Apecox Character，先让旧 Character 对称解绑，再调用 `InitAbilityActorInfo(PlayerState, this)` 并缓存 ASC。
- `void UninitializeAbilitySystem()`：仅当 ASC 当前 Avatar 确实是 `this` 时取消当前 Avatar 的活动 Ability；OwnerActor 仍有效时调用 `SetAvatarActor(nullptr)`，OwnerActor 已失效时才调用 `ClearActorInfo()`；最后清空缓存。
- 覆盖 `PossessedBy`：服务器完成占有后初始化。
- 覆盖 `OnRep_PlayerState`：客户端收到 PlayerState 后初始化；不能只处理 Owning Client，远端 Pawn 也需要正确 Avatar 信息以支持后续 Cue/表现。
- 覆盖 `UnPossessed` 与 `EndPlay`：在依赖关系仍可取得时对称解绑；实现必须避免重复解绑破坏新 Avatar。
- `InitializeAbilitySystem`、`UninitializeAbilitySystem` 和上述生命周期覆盖均为 `protected`，不向普通外部调用者公开 ActorInfo 变更入口。

### `UApecoxVitalAttributeSet`

属性：

| 名称 | 类型 | 复制 | 当前语义 |
| --- | --- | --- | --- |
| `Health` | `FGameplayAttributeData` | `ReplicatedUsing=OnRep_Health` | 当前生命值，本批不驱动死亡 |
| `MaxHealth` | `FGameplayAttributeData` | `ReplicatedUsing=OnRep_MaxHealth` | 当前生命上限 |

要求：

- 使用项目名前缀属性访问器宏 `APECOX_ATTRIBUTE_ACCESSORS`。
- `GetLifetimeReplicatedProps` 使用 `DOREPLIFETIME_CONDITION_NOTIFY(..., COND_None, REPNOTIFY_Always)`。
- `OnRep_Health`、`OnRep_MaxHealth` 使用 `GAMEPLAYATTRIBUTE_REPNOTIFY`。
- `PreAttributeBaseChange` 与 `PreAttributeChange` 复用同一私有 `ClampAttribute`，同时约束基础值与最终值。
- `PostAttributeChange` 在 `MaxHealth` 降低时把现有 `Health` 压回新上限，保证 `0 <= Health <= MaxHealth`。
- `PostGameplayEffectExecute` 继续处理瞬时 GE 后的基本钳制，不发送死亡事件、不生成 Cue、不修改 Shield。
- 本批不硬编码正式出生数值；后续通过初始化 GameplayEffect 或英雄配置建立初始值。

## Build.cs 修改

在现有运行时依赖中加入：

- `GameplayAbilities`
- `GameplayTags`
- `GameplayTasks`

不得添加当前任务没有使用的模块。

## 本批明确不做

- 不新增任何 GameplayTag 或 `DefaultGameplayTags.ini`。
- 不创建 `UApecoxGameplayAbility`、AbilitySet、InputConfig、InputTag 路由或输入资产。
- 不实现 Health 归零、Death Ability、Respawn、Shield 或 EvolutionProgress。
- 不添加相机、Mesh、动画、武器、UI、蓝图、地图或其他 `.uasset`。
- 不使用 MCP，不生成 Rider/Visual Studio 项目文件，不提交 Git。

## 编译与后续验证

- 子代理必须执行 `ApecoxEditor Win64 Development` 编译；若环境阻止，必须原样报告原因。
- Codex 审查通过后才覆盖 UE 手工操作清单。
- 用户最终在单人和 2 人 Listen Server 中验证 PlayerState/ASC/Avatar 关系；Dedicated Server 留到 Phase 1 完整闭环统一验证。
