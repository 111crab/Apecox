# ClaudeCode 执行 Prompt：Phase 2A 第一把步枪拾取与装备闭环

## 一、你的身份与协作边界

你是 Unreal Engine 5.8 C++ 项目 Apecox 的代码实施子代理。

- 用户负责审核架构与公开命名、执行 UE 编辑器资产配置、完成最终 PIE 验证。
- Codex 负责高层设计、审查你的代码和给出修复意见。
- 你负责严格按已批准设计修改 C++、执行构建并提交中文实施报告。

项目路径：

```text
D:/UnrealProject/Apecox
```

引擎路径：

```text
E:/UE_5.8
```

本轮不操作 UE 编辑器，不调用 MCP，不创建或修改 `.uasset/.umap`，不执行 Git add/commit/push，不重新生成 Rider/Visual Studio 工程文件。

## 二、开始前必须阅读

按顺序完整阅读：

1. `Agent/README.md`
2. `Agent/00_Coordination/Working_Agreement.md`
3. `Agent/00_Coordination/Current_Phase.md`
4. `Agent/00_Coordination/Current_Code_Design.md`
5. `Agent/00_Coordination/Subagent_Review_Checklist.md`
6. `Agent/02_CombatFramework/Apecox_Combat_Framework_Architecture_RFC.md` 中第 7、8、15、19、20 节，只用于理解职责，不扩大实现范围。
7. 当前相关源码：
   - `Source/Apecox/Public/Player/ApecoxPlayerController.h`
   - `Source/Apecox/Private/Player/ApecoxPlayerController.cpp`
   - `Source/Apecox/Public/Character/ApecoxPlayerCharacter.h`
   - `Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp`
   - `Source/Apecox/Public/Game/ApecoxGameMode.h`
   - `Source/Apecox/Private/Game/ApecoxGameMode.cpp`
   - `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySet.h`
   - `Source/Apecox/Private/AbilitySystem/ApecoxAbilitySet.cpp`
   - `Source/Apecox/Public/AbilitySystem/ApecoxAbilitySystemComponent.h`
   - `Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h`
   - `Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp`
   - `Source/Apecox/Public/Input/ApecoxInputConfig.h`
   - `Source/Apecox/Public/Input/ApecoxInputComponent.h`
   - `Source/Apecox/Apecox.Build.cs`

为了核对引擎 API，可以定向阅读本机 UE 5.8 源码。可以参考 Lyra 的 Inventory、Equipment 和 WeaponInstance 实现，但不得复制 Lyra 的 GameFeature、GameplayMessage、QuickBar 或任意 Fragment 系统。

`Current_Code_Design.md` 是本轮已经由用户批准的规格。若它与真实源码或 UE 5.8 API 冲突，先停止冲突部分并在报告中写明证据，不得静默改名、换所有权或扩大架构。

## 三、本轮目标

实现以下服务器权威、可多人验证的闭环：

```text
玩家空手出生
-> 本地按 E 选择视线和距离内的步枪拾取物
-> PlayerController 的 InventoryComponent 向服务器提交请求
-> 服务器验证并原子占用拾取物
-> 创建私有 WeaponInstance
-> 放入第一个空普通武器槽
-> 当前 Pawn 的 EquipmentComponent 装备
-> Owner 显示 FP 武器，其他客户端显示 TP 武器
-> 死亡时卸下并清空库存
-> 新 Pawn 空手重生
```

本轮不实现开火、ADS、换弹、弹药、伤害、准星、武器切换、死亡掉落、持枪 AnimBP 或 Montage。

## 四、允许新增的文件

头文件必须位于 `Public`，实现必须位于镜像 `Private` 目录。

```text
Source/Apecox/Public/Inventory/ApecoxInventoryItemDefinition.h
Source/Apecox/Private/Inventory/ApecoxInventoryItemDefinition.cpp
Source/Apecox/Public/Inventory/ApecoxInventoryItemInstance.h
Source/Apecox/Private/Inventory/ApecoxInventoryItemInstance.cpp
Source/Apecox/Public/Inventory/ApecoxInventoryComponent.h
Source/Apecox/Private/Inventory/ApecoxInventoryComponent.cpp

Source/Apecox/Public/Equipment/ApecoxEquipmentComponent.h
Source/Apecox/Private/Equipment/ApecoxEquipmentComponent.cpp

Source/Apecox/Public/Weapons/ApecoxWeaponDefinition.h
Source/Apecox/Private/Weapons/ApecoxWeaponDefinition.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponInstance.h
Source/Apecox/Private/Weapons/ApecoxWeaponInstance.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponPresentationDefinition.h
Source/Apecox/Private/Weapons/ApecoxWeaponPresentationDefinition.cpp
Source/Apecox/Public/Weapons/ApecoxWeaponPickup.h
Source/Apecox/Private/Weapons/ApecoxWeaponPickup.cpp
```

若一个类没有真实 `.cpp` 逻辑，可以省略空 `.cpp`，但不得把 `.h` 和 `.cpp` 混在同一目录。

## 五、允许修改的现有文件

```text
Source/Apecox/Public/Player/ApecoxPlayerController.h
Source/Apecox/Private/Player/ApecoxPlayerController.cpp
Source/Apecox/Public/Character/ApecoxPlayerCharacter.h
Source/Apecox/Private/Character/ApecoxPlayerCharacter.cpp
Source/Apecox/Public/Game/ApecoxGameMode.h
Source/Apecox/Private/Game/ApecoxGameMode.cpp
Source/Apecox/Public/GameplayTags/ApecoxGameplayTags.h
Source/Apecox/Private/GameplayTags/ApecoxGameplayTags.cpp
Source/Apecox/Apecox.Build.cs（仅在真实依赖需要时）
```

除本 Prompt 明确要求外，不修改 Ability 输入策略、Health、AttributeSet、DeathAbility、RespawnDelay 或已有 AbilitySet API。

## 六、强制设计规格

### 6.1 通用物品基础

实现：

```text
UApecoxInventoryItemDefinition : UPrimaryDataAsset
UApecoxInventoryItemInstance : UObject
```

`UApecoxInventoryItemDefinition`：

- `BlueprintType, Const`。
- `DisplayName : FText`。
- `InstanceClass : TSubclassOf<UApecoxInventoryItemInstance>`。
- `MaxStackSize : int32`，默认 1，编辑器元数据限制最小值 1。
- 不加入图标、重量、稀有度、价格等未使用字段。

`UApecoxInventoryItemInstance`：

- Outer 必须使用拥有它的 `AApecoxPlayerController`，不能用世界拾取 Actor，也不要用裸 `new`。
- `ItemDefinition` 使用 `UPROPERTY(Replicated)` 保存。
- 提供只初始化一次的 `Initialize(...)` 和只读 `GetItemDefinition()`。
- 覆写 `IsSupportedForNetworking()` 返回 `true`。
- 覆写 `GetWorld()`，安全从 Outer Actor 获取世界；CDO/Outer 无效时返回 `nullptr`。
- 实现 `GetLifetimeReplicatedProps` 并调用 `Super`。

### 6.2 武器 Definition / Instance

实现：

```text
UApecoxWeaponDefinition : UApecoxInventoryItemDefinition
UApecoxWeaponInstance : UApecoxInventoryItemInstance
UApecoxWeaponPresentationDefinition : UDataAsset
```

`UApecoxWeaponDefinition`：

- `PresentationDefinition : TObjectPtr<const UApecoxWeaponPresentationDefinition>`。
- `EquippedAbilitySets : TArray<TObjectPtr<const UApecoxAbilitySet>>`。
- 默认 `InstanceClass` 必须是 `UApecoxWeaponInstance`。
- 创建实例前验证配置的 `InstanceClass` 是 `UApecoxWeaponInstance` 子类；错误时 `ensureMsgf` 并拒绝事务。
- 不加入 FireRate、Damage、AmmoType、Projectile、Hitscan 或 `FInstancedStruct`。

`UApecoxWeaponInstance`：

- 只增加类型安全的 `GetWeaponDefinition()`。
- 本轮不增加弹匣、散布、热量、配件或 ShotId。

`UApecoxWeaponPresentationDefinition`：

- `BlueprintType, Const`。
- 直接引用 `FirstPersonWeaponMesh`、`ThirdPersonWeaponMesh`、`WorldPickupMesh`。
- 保存 FP/TP 各自的 `FName AttachSocket` 与 `FTransform AttachTransform`。
- `WorldPickupMesh` 为空时允许回退到 `ThirdPersonWeaponMesh`。
- 不加入 AnimBP、Montage、VFX、音效、准星或后坐力字段。

### 6.3 武器槽位和私有 Inventory

声明：

```text
EApecoxWeaponSlot::None
EApecoxWeaponSlot::Primary
EApecoxWeaponSlot::Secondary
```

要求：

- `None` 只是空手/无槽位哨兵，不能作为存储槽。
- 不使用 GameplayTag 表示槽位。

实现：

- `FApecoxInventoryEntry : FFastArraySerializerItem`，保存 `ItemInstance` 和 `StackCount`。
- `FApecoxInventoryList : FFastArraySerializer`，保存 Entries、OwnerComponent，提供 NetDeltaSerialize、增删和只读查询。
- `TStructOpsTypeTraits` 设置 `WithNetDeltaSerializer=true`。
- 所有结构指针使用 `UPROPERTY`，OwnerComponent 使用 `NotReplicated`。
- 武器条目的 StackCount 固定为 1。
- 不引入 GameplayMessageSubsystem；复制回调只做必要委托/日志。

`FApecoxWeaponSlotState` 保存：

- `PrimaryWeapon`
- `SecondaryWeapon`

它与通用 InventoryList 分开，避免未来弹药/电池获得武器槽字段。

实现 `UApecoxInventoryComponent : UActorComponent`：

- 默认复制，禁用 Tick。
- 只能由 `AApecoxPlayerController` 创建和拥有；初始化时对错误 Owner 使用清晰 `ensureMsgf`。
- InventoryList 和 WeaponSlotState 只复制到 Owner。即使 PlayerController 本身只存在于服务器与 Owner，也要显式使用合适的复制条件表达边界。
- 实现 `ReadyForReplication()` 注册已有 ItemInstance。
- 实现 Registered Subobject 的 Add/Remove 生命周期，同时保留 `ReplicateSubobjects(...)` 传统兼容路径。
- 创建实例时使用 PlayerController 作为 Outer；删除前从 Registered Subobject List 注销。
- 不允许客户端直接修改数组、槽位或实例。

主要 API 名称：

- `RequestPickupWeapon(AApecoxWeaponPickup* Pickup)`
- `ServerRequestPickupWeapon(AApecoxWeaponPickup* Pickup)`，`Server, Reliable`
- `TryAddAndEquipWeapon(...)`
- `FindFirstFreeWeaponSlot()`
- `GetWeaponInSlot(EApecoxWeaponSlot Slot)`
- `ClearInventoryForDeath()`

如果 UE 反射/RPC 要求 `_Implementation`，按引擎规范实现，不改公开函数语义。

### 6.4 拾取事务

实现 `AApecoxWeaponPickup : AActor`：

- `SceneRoot : USceneComponent`
- `InteractionSphere : USphereComponent`
- `PickupMesh : USkeletalMeshComponent`
- `WeaponDefinition : TObjectPtr<const UApecoxWeaponDefinition>`，复制并通过 RepNotify 刷新 Mesh。
- `InteractionDistance : float`，默认使用合理的近距离数值并暴露为 `EditDefaultsOnly`。
- Actor 默认复制、禁用 Tick。
- Sphere 只做 Query，能够被本地 Visibility Trace 命中；不要模拟物理。

主要 API：

- `CanBePickedUpBy(const APawn* Pawn) const`
- `TryClaim()`
- `ReleaseClaim()`
- `Consume()`
- `OnRep_WeaponDefinition()`
- `RefreshPickupPresentation()`

事务要求：

1. 客户端只提交候选 Actor，不提交“校验成功”结论。
2. Server RPC 验证 Owner Controller、当前 Pawn、Pawn 未死亡、Definition、实例类、空槽、距离、视线和拾取物未占用。
3. 在服务器 GameThread 使用 `TryClaim()` 先占用，防止两个 RPC 顺序到达时重复发放。
4. 创建实例、加入 FastArray、分配槽位、装备必须视为一个事务。
5. 任一步失败都撤回实例、槽位、Registered Subobject 和 Claim，拾取 Actor 保留。
6. 全部成功后才 `Consume()` 并由 Authority 销毁拾取 Actor。
7. 重复请求、晚到请求、死亡后的请求必须安全失败，不得产生残留实例。

不要让世界拾取 Actor 声明供客户端调用的 Server RPC；它不是客户端拥有的 Actor。

### 6.5 当前 Equipment

实现：

```text
FApecoxEquippedWeaponState
UApecoxEquipmentComponent : UActorComponent
```

`FApecoxEquippedWeaponState` 是一个整体 RepNotify 摘要：

- `Slot`
- `WeaponDefinition`

空手必须同时满足 `Slot=None` 和 `WeaponDefinition=nullptr`。

`UApecoxEquipmentComponent`：

- 由 `AApecoxPlayerCharacter` 创建，默认复制，禁用 Tick。
- `EquippedWeaponState` 对所有相关客户端复制。
- `CurrentWeaponInstance` 只作为 Authority 运行时引用，不作为远端表现真相。
- 保存每个 EquippedAbilitySet 对应的 `FApecoxAbilitySetGrantedHandles`，卸下时全部撤销。
- 缓存 ASC 的生命周期必须由 `InitializeWithAbilitySystem` / `UninitializeFromAbilitySystem` 成对管理。

主要 API 名称：

- `InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC)`
- `UninitializeFromAbilitySystem()`
- `EquipWeapon(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* WeaponInstance)`
- `UnequipCurrentWeapon()`
- `GetEquippedWeaponDefinition()`
- `GetCurrentWeaponInstance()`
- `OnRep_EquippedWeaponState()`
- `RefreshWeaponPresentation()`
- `DestroyWeaponPresentation()`

装备约束：

- `EquipWeapon/UnequipCurrentWeapon` 仅 Authority 改变战斗真相，代码中显式守卫。
- 装备前验证 ASC、实例、Definition 和 Slot。
- 使用现有 `UApecoxAbilitySet::GrantToAbilitySystem`，SourceObject 传入 WeaponInstance。
- 卸下必须只撤销该武器授予的句柄，不能 `CancelAllAbilities()` 误伤英雄技能。
- 复用现有 `FApecoxAbilitySetGrantedHandles::RemoveFromAbilitySystem`，不要重新实现另一套授予系统。
- 所有清理函数必须幂等。

### 6.6 FP / TP 表现

`UApecoxEquipmentComponent` 在本地根据 `EquippedWeaponState` 动态创建：

- `FirstPersonWeaponMeshComponent : USkeletalMeshComponent`
- `ThirdPersonWeaponMeshComponent : USkeletalMeshComponent`

要求：

- 两个动态组件使用 `UPROPERTY(Transient)` 保存。
- 使用 `NewObject` 后正确注册、附着和销毁；不把动态组件写入构造函数默认子对象。
- FP 武器附着到 `AApecoxPlayerCharacter::GetFirstPersonMesh()`，`OnlyOwnerSee=true`。
- TP 武器附着到 `AApecoxPlayerCharacter::GetMesh()`，`OwnerNoSee=true`。
- 使用 Presentation Definition 的独立 Socket 和 Transform。
- Mesh Component 不复制、无碰撞、不保存玩法状态。
- Dedicated Server 路径不得创建 Mesh Component，也不得访问 LocalPlayer、UI 或 CameraManager。
- Server 设置复制状态后要主动刷新自身的本地表现；不能期待 Server 自动执行 OnRep。
- `OnRep_EquippedWeaponState` 只刷新表现，不授予能力、不修改库存。

本轮不切换 Character AnimBP，不播放 Montage，允许手部暂时没有完整持枪姿态。

### 6.7 PlayerController 接入

修改 `AApecoxPlayerController`：

- 构造 `InventoryComponent`。
- 新增只读 `GetInventoryComponent()`。
- 保留 `PostProcessInput()` 原有职责和实现语义。

不要把武器拾取 Trace、Mesh 或装备复制状态塞入 PlayerController。

### 6.8 PlayerCharacter 接入

修改 `AApecoxPlayerCharacter`：

- 构造 `EquipmentComponent`，提供只读 Getter。
- 新增 `HandleInteractStarted(const FInputActionValue&)`。
- 新增 `FindWeaponPickupCandidate()`，使用现有 `FirstPersonCamera` 做本地短距离 Visibility Trace，忽略自身。
- 命中有效 `AApecoxWeaponPickup` 后，通过 PlayerController Getter 调用 `InventoryComponent->RequestPickupWeapon(...)`。
- 本地 Trace 只负责体验和候选选择，Server RPC 必须重新验证。

ASC 生命周期：

- `InitializeAbilitySystem()` 在现有 ActorInfo 建立后调用 `EquipmentComponent->InitializeWithAbilitySystem(ASC)`。
- `UninitializeAbilitySystem()` 在 ASC 仍有效时撤销 Equipment，再执行现有 Pawn AbilitySet 和 ActorInfo 清理。
- 不破坏旧 Pawn 晚解绑保护。
- 不改变 HealthComponent、死亡委托和 PawnInitializationEffect 语义。

### 6.9 死亡清理与 GameMode

修改 `AApecoxGameMode::RequestPlayerRespawn(...)`：

- 仍然只在 Authority 执行。
- 在安排重生 Timer 前找到 Controller 的 InventoryComponent 并调用 `ClearInventoryForDeath()`。
- 保持同一 Controller 防重入和 `RespawnDelaySeconds=3`。
- 当前规则是删除库存，不生成世界掉落 Actor。
- GameMode 不访问 WeaponInstance 内部字段，不操作 ASC。

Character 解绑时 Equipment 已先撤销能力和表现；GameMode 随后清空 Controller 私有实例。注意顺序，不得先销毁仍被 Equipment 使用的实例。

### 6.10 输入 Tag 和绑定

新增 Native GameplayTag：

```text
C++：ApecoxGameplayTags::InputTag_Interact
Tag：InputTag.Interact
```

在 Character 现有 Native 输入绑定中：

```text
InputTag.Interact + ETriggerEvent::Started -> HandleInteractStarted
```

不要在 C++ 中硬编码 E 键；用户后续在 IMC 中配置。不要新增 `Weapon.Rifle`、`State.Equipped`、`State.Pickup`、槽位 Tag 或射击 Tag。

## 七、UE C++ 与网络硬规则

- 所有生命周期覆写调用 `Super`。
- `GetLifetimeReplicatedProps` 调用 `Super` 并使用正确的 `DOREPLIFETIME` 宏。
- 动态 UObject/Component/Actor 指针受 `UPROPERTY`、FastArray Entry 或明确弱引用保护。
- 不在构造函数访问 World、Controller、ASC 或运行时资产实例。
- 客户端无法通过伪造指针绕过距离、视线、死亡和槽位检查。
- Owning Client 私有 Inventory、公共 Equipment 摘要和 Simulated Proxy 表现边界必须清楚。
- 不在 Tick 中轮询库存、装备、死亡或表现。
- 不依赖 OnRep 在服务器执行。
- 不在客户端销毁权威 Pickup Actor。
- 不使用运行时字符串 `RequestGameplayTag` 代替批准的 Native Tag。

## 八、明确禁止

- 不实现 Fire、ADS、Reload、Switch、Drop、Ammo、Attachment、Damage 或 Crosshair。
- 不新增射击 GA、GE、GameplayCue、AbilityTask 或 `FInstancedStruct`。
- 不创建通用交互系统、QuickBar、Inventory Fragment、GameplayMessage 或 GameFeature。
- 不修改现有 ActivationPolicy、ActivationGroup、ASC 输入三组缓存或 AbilitySet 数据结构。
- 不修改第一/第三人称 Character Mesh 基线、PlayerCameraManager 或移动参数。
- 不操作 UE 编辑器、MCP、`.uasset/.umap`、Git 或项目文件生成。
- 不修改 `Agent/00_Coordination`、RFC 或其他规划文档。
- 不顺手清理第三方资产、旧报告或无关代码。

## 九、注释要求

新增代码使用中文学习型注释，重点解释：

- 为什么 Inventory 在 Controller、Equipment 在 Character。
- 为什么私有 WeaponInstance 与公开 EquippedWeaponState 分离。
- 为什么 UObject 需要 Registered Subobject 生命周期。
- 为什么拾取必须由拥有客户端的组件发 RPC，并由服务器重新验证。
- 为什么 FP/TP Mesh 只是本地表现，不能成为战斗真相。
- 为什么装备 AbilitySet 必须通过句柄成对撤销。

避免逐行翻译式注释；较长理由已经在设计文档中，不要把整段 RFC 复制进源码。

## 十、构建与自检

构建命令：

```powershell
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development -Project="D:\UnrealProject\Apecox\Apecox.uproject" -WaitMutex -FromMsBuild -architecture=x64
```

若 Live Coding、UE 编辑器或其他进程占用导致失败，记录真实原因，不要结束用户进程。代码编译错误应修复并重新构建，直到通过或出现无法自行排除的明确阻塞。

自检至少覆盖：

1. Public/Private 目录镜像正确。
2. FastArray 的 MarkItemDirty/MarkArrayDirty 使用正确。
3. ItemInstance 创建、注册、回滚、删除、死亡清理完整对称。
4. InventoryList 和 WeaponSlotState 不公开给其他玩家。
5. EquippedWeaponState 能驱动 Owner 与 Simulated Proxy 各自表现。
6. Dedicated Server 不创建视觉组件。
7. 两个客户端争抢时服务器只能成功一次。
8. 装备失败不会吞掉 Pickup 或留下库存条目。
9. 卸下不会移除 Pawn/英雄 AbilitySet。
10. Death/UnPossess/EndPlay 多次清理安全。
11. 原有 Tactical Ability、移动、跳跃、视角和重生代码语义未被破坏。
12. 没有创建射击或动画占位字段。

## 十一、中文实施报告

创建：

```text
Agent/Reports/2026-08-11_Phase2A_Rifle_Pickup_Equipment_Report.md
```

报告必须包含：

1. 实际新增与修改文件列表，指出是否超出 Prompt。
2. 每个新增类、结构体、枚举的父类、职责和文件位置。
3. 每个新增成员变量的完整名称、类型、UPROPERTY/复制方式和作用。
4. 每个新增函数的完整名称、参数、返回值、UFUNCTION/RPC 属性和作用。
5. `InputTag.Interact` 的声明、定义和绑定位置。
6. 完整拾取事务、回滚顺序、装备/卸下顺序、死亡清理顺序。
7. Authority、Owning Client、Simulated Proxy、Dedicated Server 分别执行和观察什么。
8. FastArray 和 Registered Subobject 的具体实现位置。
9. AbilitySet 授予、SourceObject 和撤销句柄如何处理。
10. FP/TP 动态 Mesh Component 的创建、附着、可见性和销毁方式。
11. 构建命令、结果和关键警告。
12. UE 编辑器仍需用户人工完成的必需事项，只列事项，不操作资产。
13. 风险、假设、未验证路径和需要 Codex 重点复审的内容。
14. 明确声明没有使用 MCP、没有修改 UE 资产、没有 Git 操作。

## 十二、停止条件

同时满足以下条件后停止并等待 Codex 审查：

- 批准范围内代码已完成。
- ApecoxEditor Win64 Development 构建通过，或提供明确且不可自行排除的阻塞证据。
- 中文实施报告已完成。
- 未操作 UE 编辑器、MCP、Git 或项目文件生成。

