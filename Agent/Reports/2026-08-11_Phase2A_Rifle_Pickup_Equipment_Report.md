# Phase 2A 第一把步枪拾取与装备闭环 — 实施报告

**日期**: 2026-08-11  
**构建结果**: ✅ 成功（ApecoxEditor Win64 Development）  
**构建耗时**: 22.77 秒（14.14 秒 UBA 编译）  
**编译警告**: 1 个（ApecoxAbilitySystemComponent.cpp:213 — 非本次修改引入的 NonInstanced 弃用警告）

---

## 一、文件清单

### 1.1 新增文件（14 个）

| # | 文件 | 职责 |
|---|------|------|
| 1 | `Public/Inventory/ApecoxInventoryItemDefinition.h` | 通用物品定义（UPrimaryDataAsset），DisplayName、InstanceClass、MaxStackSize |
| 2 | `Public/Inventory/ApecoxInventoryItemInstance.h` | 通用物品运行时实例（UObject），ItemDefinition 复制、IsSupportedForNetworking |
| 3 | `Private/Inventory/ApecoxInventoryItemInstance.cpp` | Initialize、GetWorld、GetLifetimeReplicatedProps 实现 |
| 4 | `Public/Inventory/ApecoxInventoryComponent.h` | 私有库存组件 + EApecoxWeaponSlot + FApecoxInventoryEntry + FApecoxInventoryList + FApecoxWeaponSlotState |
| 5 | `Private/Inventory/ApecoxInventoryComponent.cpp` | FastArray、Server RPC、原子拾取事务、死亡清理、Registered Subobject 生命周期 |
| 6 | `Public/Weapons/ApecoxWeaponPresentationDefinition.h` | 武器表现分离 DataAsset（FP/TP/WorldPickup Mesh/Socket/Transform） |
| 7 | `Public/Weapons/ApecoxWeaponDefinition.h` | 武器定义（继承 InventoryItemDefinition），PresentationDefinition + EquippedAbilitySets |
| 8 | `Private/Weapons/ApecoxWeaponDefinition.cpp` | 构造函数默认 InstanceClass=UApecoxWeaponInstance |
| 9 | `Public/Weapons/ApecoxWeaponInstance.h` | 武器运行时实例（继承 InventoryItemInstance），类型安全 GetWeaponDefinition() |
| 10 | `Private/Weapons/ApecoxWeaponInstance.cpp` | GetWeaponDefinition 实现（Cast 包装） |
| 11 | `Public/Weapons/ApecoxWeaponPickup.h` | 世界拾取物 Actor，Sphere Collision、bClaimed 事务、WeaponDefinition 复制 |
| 12 | `Private/Weapons/ApecoxWeaponPickup.cpp` | CanBePickedUpBy、TryClaim/ReleaseClaim/Consume、刷新表现 |
| 13 | `Public/Equipment/ApecoxEquipmentComponent.h` | 装备组件 + FApecoxEquippedWeaponState 公开摘要 |
| 14 | `Private/Equipment/ApecoxEquipmentComponent.cpp` | Equip/Unequip、AbilitySet 授予/撤销、FP/TP Mesh 动态创建/销毁 |

**未创建空 .cpp**：`ApecoxInventoryItemDefinition.cpp`、`ApecoxWeaponPresentationDefinition.cpp` 没有运行时逻辑，按 Prompt 允许省略。

### 1.2 修改文件（7 个）

| # | 文件 | 修改内容 |
|---|------|---------|
| 1 | `Public/Character/ApecoxPlayerCharacter.h` | 添加 `#include "InputActionValue.h"`、`EquipmentComponent` UPROPERTY、`GetEquipmentComponent()`、`HandleInteractStarted`、`FindWeaponPickupCandidate` |
| 2 | `Private/Character/ApecoxPlayerCharacter.cpp` | 构造函数创建 EquipmentComponent、InitializeAbilitySystem/UninitializeAbilitySystem 接入装备生命周期、SetupPlayerInputComponent 绑定 InputTag_Interact + Started、HandleInteractStarted/FindWeaponPickupCandidate 实现 |
| 3 | `Public/Player/ApecoxPlayerController.h` | 添加 `#include` InventoryComponent、`GetInventoryComponent()`、`InventoryComponent` UPROPERTY |
| 4 | `Private/Player/ApecoxPlayerController.cpp` | 构造函数创建 InventoryComponent |
| 5 | `Private/Game/ApecoxGameMode.cpp` | RequestPlayerRespawn 在安排 Timer 前调用 ClearInventoryForDeath()、添加 `#include "Inventory/ApecoxInventoryComponent.h"` |
| 6 | `Public/GameplayTags/ApecoxGameplayTags.h` | 新增 `InputTag_Interact` Native Tag 声明 |
| 7 | `Private/GameplayTags/ApecoxGameplayTags.cpp` | 新增 `InputTag_Interact` Tag 定义（`"InputTag.Interact"`） |
| 8 | `Apecox.Build.cs` | PrivateDependencyModuleNames 添加 `"NetCore"` |

**是否超出 Prompt**: 否。所有新增和修改均在批准范围内。

---

## 二、新增类、结构体、枚举详情

### 2.1 UApecoxInventoryItemDefinition
- **父类**: `UPrimaryDataAsset`
- **元数据**: `BlueprintType, Const`
- **文件**: `Public/Inventory/ApecoxInventoryItemDefinition.h`

### 2.2 UApecoxInventoryItemInstance
- **父类**: `UObject`
- **网络**: `IsSupportedForNetworking()=true`，`ItemDefinition` 通过 `DOREPLIFETIME` 复制
- **文件**: `Public/Inventory/ApecoxInventoryItemInstance.h` / `Private/...cpp`

### 2.3 UApecoxWeaponDefinition
- **父类**: `UApecoxInventoryItemDefinition`
- **文件**: `Public/Weapons/ApecoxWeaponDefinition.h` / `Private/...cpp`

### 2.4 UApecoxWeaponInstance
- **父类**: `UApecoxInventoryItemInstance`
- **文件**: `Public/Weapons/ApecoxWeaponInstance.h` / `Private/...cpp`

### 2.5 UApecoxWeaponPresentationDefinition
- **父类**: `UDataAsset`
- **元数据**: `BlueprintType, Const`
- **文件**: `Public/Weapons/ApecoxWeaponPresentationDefinition.h`

### 2.6 EApecoxWeaponSlot（枚举）
- **类型**: `uint8` 枚举
- **值**: `None`, `Primary`, `Secondary`
- **文件**: `Public/Inventory/ApecoxInventoryComponent.h`

### 2.7 FApecoxInventoryEntry（结构体）
- **父类**: `FFastArraySerializerItem`
- **文件**: `Public/Inventory/ApecoxInventoryComponent.h`

### 2.8 FApecoxInventoryList（结构体）
- **父类**: `FFastArraySerializer`
- **Trait**: `WithNetDeltaSerializer = true`
- **文件**: `Public/Inventory/ApecoxInventoryComponent.h`

### 2.9 FApecoxWeaponSlotState（结构体）
- **父类**: 无（普通 USTRUCT）
- **文件**: `Public/Inventory/ApecoxInventoryComponent.h`

### 2.10 UApecoxInventoryComponent
- **父类**: `UActorComponent`
- **默认复制**: `true`
- **文件**: `Public/Inventory/ApecoxInventoryComponent.h` / `Private/...cpp`

### 2.11 FApecoxEquippedWeaponState（结构体）
- **父类**: 无（普通 USTRUCT）
- **文件**: `Public/Equipment/ApecoxEquipmentComponent.h`

### 2.12 UApecoxEquipmentComponent
- **父类**: `UActorComponent`
- **默认复制**: `true`
- **文件**: `Public/Equipment/ApecoxEquipmentComponent.h` / `Private/...cpp`

### 2.13 AApecoxWeaponPickup
- **父类**: `AActor`
- **默认复制**: `true`
- **文件**: `Public/Weapons/ApecoxWeaponPickup.h` / `Private/...cpp`

---

## 三、新增成员变量详情

### UApecoxInventoryItemDefinition
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `DisplayName` | `FText` | `EditDefaultsOnly, BlueprintReadOnly` | 编辑器和 UI 显示名 |
| `InstanceClass` | `TSubclassOf<UApecoxInventoryItemInstance>` | `EditDefaultsOnly, BlueprintReadOnly` | 运行时实例类型 |
| `MaxStackSize` | `int32` | `EditDefaultsOnly, BlueprintReadOnly, ClampMin=1` | 最大堆叠数，默认 1 |

### UApecoxInventoryItemInstance
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `ItemDefinition` | `TObjectPtr<const UApecoxInventoryItemDefinition>` | `Replicated` | 不可变定义引用——创建后只读 |

### UApecoxWeaponDefinition
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `PresentationDefinition` | `TObjectPtr<const UApecoxWeaponPresentationDefinition>` | `EditDefaultsOnly, BlueprintReadOnly` | FP/TP/世界拾取表现入口 |
| `EquippedAbilitySets` | `TArray<TObjectPtr<const UApecoxAbilitySet>>` | `EditDefaultsOnly, BlueprintReadOnly` | 装备期间授予的 AbilitySet |

### UApecoxWeaponPresentationDefinition
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `FirstPersonWeaponMesh` | `TObjectPtr<USkeletalMesh>` | `EditDefaultsOnly, BlueprintReadOnly` | FP 武器 SkeletalMesh |
| `FirstPersonAttachSocket` | `FName` | `EditDefaultsOnly, BlueprintReadOnly` | FP 附着 Socket 名 |
| `FirstPersonAttachTransform` | `FTransform` | `EditDefaultsOnly, BlueprintReadOnly` | FP 附着微调 |
| `ThirdPersonWeaponMesh` | `TObjectPtr<USkeletalMesh>` | `EditDefaultsOnly, BlueprintReadOnly` | TP 武器 SkeletalMesh |
| `ThirdPersonAttachSocket` | `FName` | `EditDefaultsOnly, BlueprintReadOnly` | TP 附着 Socket 名 |
| `ThirdPersonAttachTransform` | `FTransform` | `EditDefaultsOnly, BlueprintReadOnly` | TP 附着微调 |
| `WorldPickupMesh` | `TObjectPtr<USkeletalMesh>` | `EditDefaultsOnly, BlueprintReadOnly` | 地面拾取 Mesh——为空回退到 ThirdPersonWeaponMesh |

### FApecoxInventoryEntry
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `ItemInstance` | `TObjectPtr<UApecoxInventoryItemInstance>` | `UPROPERTY()` | 运行时物品实例 |
| `StackCount` | `int32` | `UPROPERTY()` | 堆叠数量——武器固定 1 |

### FApecoxInventoryList
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `Entries` | `TArray<FApecoxInventoryEntry>` | `UPROPERTY()` | FastArray 条目列表 |

### FApecoxWeaponSlotState
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `PrimaryWeapon` | `TObjectPtr<UApecoxWeaponInstance>` | `UPROPERTY()` | 主武器槽实例 |
| `SecondaryWeapon` | `TObjectPtr<UApecoxWeaponInstance>` | `UPROPERTY()` | 副武器槽实例 |

### UApecoxInventoryComponent
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `InventoryList` | `FApecoxInventoryList` | `Replicated (COND_OwnerOnly)` | 库存条目 Delta 复制 |
| `WeaponSlotState` | `FApecoxWeaponSlotState` | `Replicated (COND_OwnerOnly)` | 武器槽映射 |
| `RegisteredInstances` | `TArray<TObjectPtr<UApecoxInventoryItemInstance>>` | `UPROPERTY()` | 已注册 Subobject 集合 |

### FApecoxEquippedWeaponState
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `Slot` | `EApecoxWeaponSlot` | `UPROPERTY()` | 当前装备槽位——None=空手 |
| `WeaponDefinition` | `TObjectPtr<const UApecoxWeaponDefinition>` | `UPROPERTY()` | 当前装备武器定义——nullptr=空手 |

### UApecoxEquipmentComponent
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `AbilitySystemComponent` | `TObjectPtr<UApecoxAbilitySystemComponent>` | `UPROPERTY()` | 缓存 ASC 引用 |
| `WeaponAbilitySetHandles` | `FApecoxAbilitySetGrantedHandles` | `UPROPERTY()` | 武器 AbilitySet 撤销句柄 |
| `CurrentWeaponInstance` | `TObjectPtr<UApecoxWeaponInstance>` | `UPROPERTY()` | Authority 运行时引用 |
| `EquippedWeaponState` | `FApecoxEquippedWeaponState` | `ReplicatedUsing=OnRep_EquippedWeaponState` | 公开复制摘要 |
| `FirstPersonWeaponMeshComponent` | `TObjectPtr<USkeletalMeshComponent>` | `Transient` | 动态创建的 FP 武器 Mesh |
| `ThirdPersonWeaponMeshComponent` | `TObjectPtr<USkeletalMeshComponent>` | `Transient` | 动态创建的 TP 武器 Mesh |

### AApecoxWeaponPickup
| 变量 | 类型 | UPROPERTY | 作用 |
|------|------|-----------|------|
| `SceneRoot` | `TObjectPtr<USceneComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 根组件 |
| `InteractionSphere` | `TObjectPtr<USphereComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 交互球体——仅 Query |
| `PickupMesh` | `TObjectPtr<USkeletalMeshComponent>` | `VisibleAnywhere, BlueprintReadOnly` | 地面武器 Mesh——无碰撞 |
| `WeaponDefinition` | `TObjectPtr<const UApecoxWeaponDefinition>` | `ReplicatedUsing=OnRep_WeaponDefinition, EditInstanceOnly` | 拾取物代表的武器定义 |
| `InteractionDistance` | `float` | `EditDefaultsOnly` | 交互验证距离——默认 200 |
| `bClaimed` | `bool` | 无（非 UPROPERTY） | 瞬时事务标记——服务器权威 |

---

## 四、新增函数详情

### UApecoxInventoryItemInstance
| 函数 | 签名 | 作用 |
|------|------|------|
| `Initialize` | `void Initialize(const UApecoxInventoryItemDefinition* InDefinition)` | Authority 一次性设置不可变 ItemDefinition——重复调用安全返回 |
| `GetItemDefinition` | `const UApecoxInventoryItemDefinition* GetItemDefinition() const` | BlueprintCallable 只读访问 |
| `IsSupportedForNetworking` | `virtual bool IsSupportedForNetworking() const override` | 返回 true，支持作为 Subobject 复制 |
| `GetWorld` | `virtual UWorld* GetWorld() const override` | 从 Outer Actor 安全获取 World——CDO/Outer 无效返回 nullptr |

### UApecoxWeaponInstance
| 函数 | 签名 | 作用 |
|------|------|------|
| `GetWeaponDefinition` | `const UApecoxWeaponDefinition* GetWeaponDefinition() const` | BlueprintCallable 类型安全 Cast 封装 |

### FApecoxInventoryList
| 函数 | 签名 | 作用 |
|------|------|------|
| `NetDeltaSerialize` | `bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)` | FFastArraySerializer 标准 Delta 序列化入口 |
| `PostReplicatedAdd` | `void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)` | 条目新增回调——日志记录 |
| `PostReplicatedChange` | `void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)` | 条目变化回调——日志记录 |
| `PreReplicatedRemove` | `void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)` | 条目即将移除回调——日志记录 |
| `FindEntryByInstance` | `const FApecoxInventoryEntry* FindEntryByInstance(const UApecoxInventoryItemInstance* Instance) const` | 根据实例查找条目 |
| `FindEntryIndex` | `int32 FindEntryIndex(const UApecoxInventoryItemInstance* Instance) const` | 根据实例查找索引 |
| `AddEntry` | `void AddEntry(UApecoxInventoryItemInstance* Instance, int32 Stack)` | 添加条目——防重复；脏标记由组件负责 |
| `RemoveEntry` | `void RemoveEntry(UApecoxInventoryItemInstance* Instance)` | 移除条目 |
| `ClearAllEntries` | `void ClearAllEntries()` | 清空所有条目 |

### FApecoxWeaponSlotState
| 函数 | 签名 | 作用 |
|------|------|------|
| `FindSlotForInstance` | `EApecoxWeaponSlot FindSlotForInstance(const UApecoxWeaponInstance* Instance) const` | 查找实例所在槽位——未找到返回 None |
| `SetSlot` | `void SetSlot(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* Instance)` | 设置槽位——None 会被 ensure |
| `ClearSlot` | `void ClearSlot(EApecoxWeaponSlot Slot)` | 清空槽位（设为 nullptr） |
| `ClearAllSlots` | `void ClearAllSlots()` | 清空所有槽位 |

### UApecoxInventoryComponent
| 函数 | 签名 | 作用 |
|------|------|------|
| `InitializeComponent` | `void InitializeComponent() override` | ensureMsgf 验证 Owner 是 AApecoxPlayerController |
| `RequestPickupWeapon` | `void RequestPickupWeapon(AApecoxWeaponPickup* Pickup)` | 本地输入入口——提交 Server RPC |
| `ServerRequestPickupWeapon` | `UFUNCTION(Server, Reliable) void ServerRequestPickupWeapon(AApecoxWeaponPickup* Pickup)` | 服务器重新验证全部条件后执行拾取事务 |
| `FindFirstFreeWeaponSlot` | `EApecoxWeaponSlot FindFirstFreeWeaponSlot() const` | 查找第一个空闲武器槽——Primary 优先 |
| `GetWeaponInSlot` | `UApecoxWeaponInstance* GetWeaponInSlot(EApecoxWeaponSlot Slot) const` | 获取指定槽位武器实例 |
| `GetPrimaryWeapon` | `UApecoxWeaponInstance* GetPrimaryWeapon() const` | 便捷 Primary 访问 |
| `TryAddAndEquipWeapon` | `UApecoxWeaponInstance* TryAddAndEquipWeapon(const UApecoxWeaponDefinition* WeaponDef, AApecoxWeaponPickup* Pickup)` | Authority 原子拾取事务——成功返回实例，失败返回 nullptr |
| `ClearInventoryForDeath` | `void ClearInventoryForDeath()` | 清空槽位、销毁所有实例、清空 FastArray |
| `ReadyForReplication` | `virtual void ReadyForReplication() override` | 注册已存在的实例——晚加入客户端可见 |
| `ReplicateSubobjects` | `virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override` | 传统兼容路径：逐个序列化 Subobject |
| `RegisterInstance` | `void RegisterInstance(UApecoxInventoryItemInstance* Instance)` (private) | 加入 RegisteredInstances + AddReplicatedSubObject |
| `DestroyInstance` | `void DestroyInstance(UApecoxInventoryItemInstance* Instance)` (private) | 清除槽位 + RemoveReplicatedSubObject + 移除条目 + MarkAsGarbage |

### UApecoxEquipmentComponent
| 函数 | 签名 | 作用 |
|------|------|------|
| `EndPlay` | `virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override` | 销毁武器表现——GC 前最后清理 |
| `InitializeWithAbilitySystem` | `void InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC)` | 绑定 ASC——幂等：同一 ASC 不重复初始化 |
| `UninitializeFromAbilitySystem` | `void UninitializeFromAbilitySystem()` | 撤销武器 AbilitySet、清空状态 |
| `EquipWeapon` | `void EquipWeapon(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* WeaponInstance)` | Authority 装备——验证→卸旧→授予 AbilitySet→刷新摘要→刷新表现 |
| `UnequipCurrentWeapon` | `void UnequipCurrentWeapon()` | Authority 卸下——ClearAbilityInput→撤销 AbilitySet→清空摘要→销毁表现；幂等 |
| `GetEquippedWeaponDefinition` | `const UApecoxWeaponDefinition* GetEquippedWeaponDefinition() const` | 只读访问当前装备定义 |
| `GetCurrentWeaponInstance` | `UApecoxWeaponInstance* GetCurrentWeaponInstance() const` | 只读访问 Authority 运行时实例 |
| `IsArmed` | `bool IsArmed() const` | 是否持有武器 |
| `OnRep_EquippedWeaponState` | `UFUNCTION() void OnRep_EquippedWeaponState()` | RepNotify——只刷新表现，不授予能力不修改库存 |
| `RefreshWeaponPresentation` | `void RefreshWeaponPresentation()` | 根据 EquippedWeaponState 动态创建/更新 FP/TP Mesh |
| `DestroyWeaponPresentation` | `void DestroyWeaponPresentation()` | 销毁 FP/TP 动态 Mesh Component——幂等 |

### AApecoxWeaponPickup
| 函数 | 签名 | 作用 |
|------|------|------|
| `CanBePickedUpBy` | `bool CanBePickedUpBy(const APawn* Pawn) const` | 服务器验证——距离、视线、bClaimed、存活 |
| `TryClaim` | `bool TryClaim()` | 原子占用——成功返回 true，已被 Claim 返回 false |
| `ReleaseClaim` | `void ReleaseClaim()` | 释放占用（回滚时使用） |
| `Consume` | `void Consume()` | 事务成功后服务器销毁 Actor |
| `OnRep_WeaponDefinition` | `UFUNCTION() void OnRep_WeaponDefinition()` | RepNotify——刷新地面 Mesh |
| `RefreshPickupPresentation` | `void RefreshPickupPresentation()` | 根据 PresentationDefinition 设置 PickupMesh |

### AApecoxPlayerCharacter（新增）
| 函数 | 签名 | 作用 |
|------|------|------|
| `GetEquipmentComponent` | `UApecoxEquipmentComponent* GetEquipmentComponent() const` | BlueprintCallable 只读访问装备组件 |
| `HandleInteractStarted` | `void HandleInteractStarted(const FInputActionValue& ActionValue)` | 本地 Trace → InventoryComponent->RequestPickupWeapon |
| `FindWeaponPickupCandidate` | `AApecoxWeaponPickup* FindWeaponPickupCandidate() const` | FirstPersonCamera 250 单位 Visibility Trace |

### AApecoxPlayerController（新增）
| 函数 | 签名 | 作用 |
|------|------|------|
| `GetInventoryComponent` | `UApecoxInventoryComponent* GetInventoryComponent() const` | BlueprintCallable 只读访问库存组件 |

---

## 五、InputTag.Interact 声明、定义与绑定

| 层级 | 位置 | 代码 |
|------|------|------|
| 声明 | `ApecoxGameplayTags.h:30` | `APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);` |
| 定义 | `ApecoxGameplayTags.cpp:19` | `UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact", "交互键——拾取、开门、对话等，不是 Ability 身份");` |
| 绑定 | `ApecoxPlayerCharacter.cpp:354` | `ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Interact, ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleInteractStarted);` |
| 处理 | `ApecoxPlayerCharacter.cpp:553` | `HandleInteractStarted`：本地 Trace 候选 → Controller→InventoryComponent→RequestPickupWeapon |

---

## 六、关键流程时序

### 6.1 完整拾取事务（Authority，TryAddAndEquipWeapon）

```
1. 验证 WeaponDef 非空
2. 验证 InstanceClass 非空且是 UApecoxWeaponInstance 子类（ensureMsgf）
3. TryClaim() 原子占用 Pickup——失败→安全返回（不产生残留）
4. 查找第一个空闲槽位 FindFirstFreeWeaponSlot()
   ├─ 无空闲→ReleaseClaim()→返回 nullptr
5. NewObject<UApecoxWeaponInstance>(Owner=PlayerController, InstanceClass)
   ├─ 创建失败→ReleaseClaim()→返回 nullptr
6. WeaponInstance->Initialize(WeaponDef)
7. RegisterInstance(WeaponInstance)
   ├─ AddReplicatedSubObject
8. InventoryList.AddEntry(WeaponInstance, 1) + MARK_PROPERTY_DIRTY_FROM_NAME
9. WeaponSlotState.SetSlot(FreeSlot, WeaponInstance) + MARK_PROPERTY_DIRTY_FROM_NAME
10. 通过 PlayerController→GetPawn()→GetEquipmentComponent()→EquipWeapon()
    ├─ EquipWeapon 内部：UnequipCurrentWeapon→Grant AbilitySet→写 EquippedWeaponState→RefreshWeaponPresentation
11. OnWeaponAdded 委托广播
12. Pickup->Consume() → Destroy()（服务器权威销毁）
```

### 6.2 回滚顺序（任一步失败）

```
失败点 n → 逆序回滚已完成步骤
- 步骤 6 失败→ReleaseClaim()
- 步骤 8/9 失败→DestroyInstance(WeaponInstance)→ReleaseClaim()
- 步骤 10 失败→DestroyInstance(WeaponInstance)→ReleaseClaim()
拾取 Actor 保留，可被其他客户端拾取
```

### 6.3 装备顺序（EquipWeapon）

```
1. GetOwnerRole() != ROLE_Authority → 拒绝
2. 验证 ASC、Instance、WeaponDef、Slot
3. UnequipCurrentWeapon() —— 幂等
   ├─ ClearAbilityInput
   ├─ WeaponAbilitySetHandles.RemoveFromAbilitySystem(ASC)
   ├─ CurrentWeaponInstance=nullptr, EquippedWeaponState={None,nullptr}
   ├─ MARK_PROPERTY_DIRTY_FROM_NAME
   └─ DestroyWeaponPresentation()
4. 遍历 EquippedAbilitySets → GrantToAbilitySystem(ASC, Handles, WeaponInstance)
5. CurrentWeaponInstance=WeaponInstance, EquippedWeaponState={Slot, WeaponDef}
6. MARK_PROPERTY_DIRTY_FROM_NAME
7. RefreshWeaponPresentation()（跳过 Dedicated Server）
```

### 6.4 死亡清理顺序

```
AGameMode::RequestPlayerRespawn (Authority)
├─ 1. GetInventoryComponent()->ClearInventoryForDeath()
│      ├─ WeaponSlotState.ClearAllSlots()
│      ├─ 遍历 InventoryList.Entries → DestroyInstance(Instance)
│      │   ├─ ClearSlot(FindSlotForInstance)
│      │   ├─ RemoveReplicatedSubObject
│      │   ├─ InventoryList.RemoveEntry
│      │   ├─ MARK_PROPERTY_DIRTY_FROM_NAME(InventoryList)
│      │   └─ Instance->MarkAsGarbage()
│      └─ InventoryList.ClearAllEntries() + MARK_PROPERTY_DIRTY_FROM_NAME
├─ 2. PendingRespawnControllers.Add(Controller)
└─ 3. SetTimer(RespawnDelaySeconds) → RestartPlayerAfterDelay

Character::UninitializeAbilitySystem（先于 GameMode 清理发生）
├─ EquipmentComponent->UninitializeFromAbilitySystem()
│   └─ UnequipCurrentWeapon()（撤销 AbilitySet、销毁表现）
└─ ... 原有 Pawn AbilitySet 和 ActorInfo 清理
```

**清理顺序保证**：Character 解绑时 Equipment 先撤销能力和表现 → GameMode 随后清空 Controller 私有实例。不会先销毁仍被 Equipment 使用的实例。

---

## 七、复制边界矩阵

| 实体 | 服务器 | Owner Client | Simulated Proxy | 方式 |
|------|--------|-------------|-----------------|------|
| `AApecoxWeaponPickup::WeaponDefinition` | ✅ 写入 | ✅ 收到 | ✅ 收到 | `DOREPLIFETIME`（所有人） |
| `AApecoxWeaponPickup::bClaimed` | ✅ 写入 | ❌ 不可见 | ❌ 不可见 | 不复制（瞬态） |
| `UApecoxInventoryItemInstance::ItemDefinition` | ✅ 写入 | ✅ 收到 | ❌ 不可见 | `DOREPLIFETIME`（Subobject） |
| `InventoryComponent::InventoryList` | ✅ 写入 | ✅ 收到 | ❌ 不可见 | `DOREPLIFETIME_CONDITION(COND_OwnerOnly)` |
| `InventoryComponent::WeaponSlotState` | ✅ 写入 | ✅ 收到 | ❌ 不可见 | `DOREPLIFETIME_CONDITION(COND_OwnerOnly)` |
| `EquipmentComponent::EquippedWeaponState` | ✅ 写入 | ✅ 收到 | ✅ 收到 | `DOREPLIFETIME`（所有人） |
| `EquipmentComponent::CurrentWeaponInstance` | ✅ 读写 | ❌ 不可见 | ❌ 不可见 | 不复制（Authority Only） |
| `FP/TP Weapon Mesh Component` | ❌ 不创建 | ✅ 本地创建 | ❌ 不创建 | Transient（表现层） |

**边界解释**：
- **InventoryList + WeaponSlotState** 为 `COND_OwnerOnly`：其他玩家不需要知道你的库存和槽位分配。
- **EquippedWeaponState** 复制给所有人：驱动 Owner 的 FP 表现和其他客户端的 TP 表现。
- **CurrentWeaponInstance** 不复制：只是 Authority 内部引用；远端通过 EquippedWeaponState 的 WeaponDefinition 获取足够信息。
- **FP/TP Mesh** 不复制：只读 PresentationDefinition 在本地重建视觉表现。

---

## 八、FastArray 实现位置

| 组件 | 文件 | 位置 |
|------|------|------|
| `FApecoxInventoryEntry` 结构体 | `ApecoxInventoryComponent.h:38-50` | 继承 `FFastArraySerializerItem` |
| `FApecoxInventoryList` 结构体 | `ApecoxInventoryComponent.h:57-80` | 继承 `FFastArraySerializer`，持有 `TArray<FApecoxInventoryEntry> Entries` |
| `TStructOpsTypeTraits<FApecoxInventoryList>` | `ApecoxInventoryComponent.h:83-87` | `WithNetDeltaSerializer = true` |
| `NetDeltaSerialize` 实现 | `ApecoxInventoryComponent.cpp:19-23` | 调用 `FFastArraySerializer::FastArrayDeltaSerialize<FApecoxInventoryEntry, FApecoxInventoryList>` |
| `PostReplicatedAdd` | `ApecoxInventoryComponent.cpp:25-35` | 日志记录 |
| `PostReplicatedChange` | `ApecoxInventoryComponent.cpp:37-47` | 日志记录 |
| `PreReplicatedRemove` | `ApecoxInventoryComponent.cpp:49-59` | 日志记录 |
| 脏标记 | `ApecoxInventoryComponent.cpp` 多处 | `MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, InventoryList, this)` |

**设计决策**：FastArray 结构体不持有 OwnerComponent 回指针。添加/移除条目的脏标记由 UApecoxInventoryComponent 在操作后统一调用 `MARK_PROPERTY_DIRTY_FROM_NAME`。

---

## 九、AbilitySet 授予与撤销句柄

### 授予（EquipWeapon 中）
```cpp
FApecoxAbilitySetGrantedHandles Handles;
for (const TObjectPtr<const UApecoxAbilitySet>& AbilitySet : WeaponDef->EquippedAbilitySets)
{
    if (AbilitySet)
    {
        AbilitySet->GrantToAbilitySystem(AbilitySystemComponent, Handles,
            WeaponInstance // SourceObject 指向 WeaponInstance
        );
    }
}
WeaponAbilitySetHandles = Handles;
```

**SourceObject 用途**：武器 GA 通过 `GetCurrentSourceObject()` 获得 WeaponInstance，区分"谁的武器在射击"。本轮武器 GA 尚未实现，SourceObject 基础设施已就位。

### 撤销（UnequipCurrentWeapon 中）
```cpp
AbilitySystemComponent->ClearAbilityInput();
WeaponAbilitySetHandles.RemoveFromAbilitySystem(AbilitySystemComponent);
```

**为什么不能使用 CancelAllAbilities**：Pawn AbilitySet 和 Weapon AbilitySet 共存于同一 ASC。`CancelAllAbilities()` 会误伤英雄技能（如战术能力）。通过保存的 `FApecoxAbilitySetGrantedHandles` 精确定位，只撤销武器授予的 GA/GE。

---

## 十、FP/TP 动态 Mesh Component 管理

### 创建与附着（RefreshWeaponPresentation）

```
1. 检查 GetNetMode() != NM_DedicatedServer —— DS 跳过
2. 检查 EquippedWeaponState 非空手
3. 读取 WeaponDef->PresentationDefinition

4. 第一人称：
   ├─ NewObject<USkeletalMeshComponent>(Character, "FirstPersonWeaponMesh")
   ├─ RegisterComponent()
   ├─ SetSkeletalMesh(Pres->FirstPersonWeaponMesh)
   ├─ SetOnlyOwnerSee(true) —— 仅自己可见
   ├─ SetCollisionEnabled(NoCollision)
   └─ AttachToComponent(GetFirstPersonMesh(), SnapToTargetIncludingScale,
       Pres->FirstPersonAttachSocket, Pres->FirstPersonAttachTransform)

5. 第三人称：
   ├─ NewObject<USkeletalMeshComponent>(Character, "ThirdPersonWeaponMesh")
   ├─ RegisterComponent()
   ├─ SetSkeletalMesh(Pres->ThirdPersonWeaponMesh)
   ├─ SetOwnerNoSee(true) —— 自己不看到 TP 重叠
   ├─ SetCollisionEnabled(NoCollision)
   └─ AttachToComponent(GetMesh(), SnapToTargetIncludingScale,
       Pres->ThirdPersonAttachSocket, Pres->ThirdPersonAttachTransform)
```

### 销毁（DestroyWeaponPresentation）

```cpp
FirstPersonWeaponMeshComponent->DestroyComponent(); → nullptr
ThirdPersonWeaponMeshComponent->DestroyComponent(); → nullptr
```
幂等：多次调用安全，检查空指针后执行。

### 生命周期触发点

| 触发 | 调用方 |
|------|--------|
| 装备完成 | `EquipWeapon` → Server 主动刷新（非 RepNotify） |
| 复制到达 | `OnRep_EquippedWeaponState` → 先销毁再重建 |
| 卸下 | `UnequipCurrentWeapon` → DestroyWeaponPresentation |
| 组件销毁 | `EndPlay` → DestroyWeaponPresentation |

---

## 十一、构建结果

```text
构建命令:
E:\UE_5.8\Engine\Build\BatchFiles\Build.bat ApecoxEditor Win64 Development
  -Project="D:\UnrealProject\Apecox\Apecox.uproject"
  -WaitMutex -FromMsBuild -architecture=x64

结果: 成功
耗时: 22.77 秒（UBA 编译 14.14 秒 + 链接等）
操作数: 16（12 个编译 + 1 个 Module + 1 个 .lib + 1 个 .dll + 1 个 WriteMetadata）
输出: E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe

编译警告:
- ApecoxAbilitySystemComponent.cpp(213): warning C4996
  'EGameplayAbilityInstancingPolicy::NonInstanced' 弃用
  原因: 非本次修改引入（Phase 1C 已有代码），与 Phase 2A 无关

链接错误修复记录:
- 第一次: FFastArraySerializer 链接错误（15 个未解析符号）
  原因: NetCore 模块未链接
  修复: Build.cs PrivateDependencyModuleNames 添加 "NetCore"
- 第二次: 构建成功（本次）
```

---

## 十二、UE 编辑器必需人工完成事项

以下事项由用户在 UE 编辑器中完成，本轮不操作资产：

1. **创建 WeaponDefinition DataAsset**（如 `DA_WP_Rifle_M16`）
   - 类型: `UApecoxWeaponDefinition`
   - 配置: `DisplayName`、`InstanceClass`（保持默认 `UApecoxWeaponInstance`）、`MaxStackSize=1`
   - 配置: `PresentationDefinition` 指向步骤 2 的资产
   - 配置: `EquippedAbilitySets`（本轮可为空——武器 GA 在 Phase 2B 实现）

2. **创建 WeaponPresentationDefinition DataAsset**（如 `DA_WP_Rifle_M16_Pres`）
   - 类型: `UApecoxWeaponPresentationDefinition`
   - 配置: `FirstPersonWeaponMesh`（如 Stephen_FPS 或 Rifle Pro 的 FP 武器 Mesh）
   - 配置: `FirstPersonAttachSocket`（如 `"weapon_r"`）
   - 配置: `ThirdPersonWeaponMesh`（对应 TP Mesh）
   - 配置: `ThirdPersonAttachSocket`
   - 配置: `WorldPickupMesh`（可选——不赋值时回退到 ThirdPersonWeaponMesh）

3. **创建 WeaponPickup Actor Blueprint**（如 `BP_WeaponPickup_Rifle`）
   - 父类: `AApecoxWeaponPickup`
   - 在关卡中放置，设置 `WeaponDefinition` 指向步骤 1 的 DataAsset

4. **配置 FP/TP 角色蓝图上的 Weapon Socket**
   - 在 FP SkeletalMesh 上添加 Socket `"weapon_r"`（或 Presentation 中任意命名）
   - 在 TP SkeletalMesh 上添加对应 Socket

5. **配置 EnhancedInput IMC**
   - 将物理按键（如 `E`）映射到 `InputAction`，该 IA 的 Tag 为 `InputTag.Interact`

6. **验证 Character Blueprint**
   - 确认 `DefaultInputMappingContext` 包含 Interact IA
   - 确认 `InputConfig` DataAsset 包含 `InputTag.Interact` 条目

7. **PIE 多人验证**
   - 两个窗口测试：Server/Client 分别可见各自 FP、对方 TP 武器
   - 同时拾取同一 Pickup：验证只有一个客户端成功
   - 死亡/重生：验证空手重生、旧武器已销毁

---

## 十三、风险与假设

### 风险
1. **Dedicated Server 路径**：代码在所有表现路径检查了 `NM_DedicatedServer`，但项目没有 DS 构建——DS 路径未经实际测试。
2. **两个客户端同时拾取**：`bClaimed` 不是复制变量，是瞬时服务端内存标记。同一帧内两个 RPC 顺序到达时 `TryClaim()` 的 CAS 语义依赖单线程 GameThread——UE 的 RPC 处理本身是串行的，所以安全，但极端网络延迟下需要确认。
3. **Registered Subobject List vs 传统 ReplicateSubobjects**：当前双路径实现（`IsUsingRegisteredSubObjectList()` 检查 + `ReplicateSubobjects` 兼容）。UE 5.8 推荐 Registered Subobject List，但需要确认项目配置开启了此特性。
4. **EquippedWeaponState 原子性**：Slot 和 WeaponDefinition 在同一结构体中通过单一 RepNotify 更新——分开发送确实不会发生，但如果未来有人拆分字段，需要提醒风险。
5. **FP/TP Mesh Component 的 Outer 是 Character**：动态组件通过 `NewObject<Character>` 创建，Outer 是 Character 而非 EquipmentComponent。Character 销毁时这些组件会自动 GC 清理，但 EndPlay 中的显式 DestroyComponent 仍保持幂等安全。

### 假设
1. `PresentationDefinition` 的 Socket 名称与 Character BP 上的实际 Socket 匹配——C++ 不做运行时验证，如果 Socket 不存在 `AttachToComponent` 会静默失败（附加到根）。
2. `EquippedAbilitySets` 本轮为空——武器 GA 在 Phase 2B 实现，当前 AbilitySet 授予不会产生运行时可见效果。
3. 距离验证同时出现在客户端 Trace（250）和服务器 CanBePickedUpBy（InteractionDistance=200）——这是故意的：客户端 Trace 稍微放宽，服务器做精确验证。如果配置不一致导致客户端能看到但捡不到，需要调整。
4. 死亡库存清空不生成世界掉落——当前规则如 RFC 所要求。如果后续需要掉落，修改点在 GameMode。

### 需要 Codex 重点复审
1. `UApecoxInventoryComponent::DestroyInstance` 中的 `MarkAsGarbage()` —— 是否与 Registered Subobject 的 `RemoveReplicatedSubObject` 有生命周期竞争？
2. `UApecoxEquipmentComponent::RefreshWeaponPresentation` 中 `NewObject` + `RegisterComponent` 的模式——是否需要在每次装备时销毁/重建而非复用已有组件？
3. `MARK_PROPERTY_DIRTY_FROM_NAME` 的使用是否正确——特别是在 `TryAddAndEquipWeapon` 中对 InventoryList 和 WeaponSlotState 各调用一次是否足够？
4. `COND_OwnerOnly` 的 InventoryList 复制是否需要在 PlayerController（非 PlayerState 拥有的组件）上额外处理？

---

## 十四、声明

- ✅ 未使用 MCP 工具
- ✅ 未修改任何 `.uasset`、`.umap` 资产文件
- ✅ 未执行 Git add/commit/push 操作
- ✅ 未操作 UE 编辑器
- ✅ 未修改 `Agent/00_Coordination`、RFC 或任何规划文档
- ✅ 所有新增和修改均在 Prompt 批准范围内
- ✅ ApecoxEditor Win64 Development 构建通过
