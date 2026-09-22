// Copyright Apecox. All Rights Reserved.

#include "Inventory/ApecoxInventoryComponent.h"
#include "Inventory/ApecoxInventoryItemInstance.h"
#include "Inventory/ApecoxInventoryItemDefinition.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponInstance.h"
#include "Weapons/ApecoxWeaponPickup.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Character/ApecoxHealthComponent.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Player/ApecoxPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

// ============================================================================
// FApecoxInventoryList — FastArray Serialization
// ============================================================================

bool FApecoxInventoryList::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FApecoxInventoryEntry, FApecoxInventoryList>(
		Entries, DeltaParms, *this);
}

void FApecoxInventoryList::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		if (Entries.IsValidIndex(Index))
		{
			UE_LOG(LogTemp, Log, TEXT("[Apecox] InventoryList: Entry added (Index=%d, Instance=%s)."),
				Index, *GetNameSafe(Entries[Index].ItemInstance));
		}
	}
}

void FApecoxInventoryList::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		if (Entries.IsValidIndex(Index))
		{
			UE_LOG(LogTemp, Log, TEXT("[Apecox] InventoryList: Entry changed (Index=%d, StackCount=%d)."),
				Index, Entries[Index].StackCount);
		}
	}
}

void FApecoxInventoryList::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		if (Entries.IsValidIndex(Index))
		{
			UE_LOG(LogTemp, Log, TEXT("[Apecox] InventoryList: Entry removed (Index=%d, Instance=%s)."),
				Index, *GetNameSafe(Entries[Index].ItemInstance));
		}
	}
}

const FApecoxInventoryEntry* FApecoxInventoryList::FindEntryByInstance(const UApecoxInventoryItemInstance* Instance) const
{
	const int32 Idx = FindEntryIndex(Instance);
	return (Idx != INDEX_NONE) ? &Entries[Idx] : nullptr;
}

int32 FApecoxInventoryList::FindEntryIndex(const UApecoxInventoryItemInstance* Instance) const
{
	if (!Instance)
	{
		return INDEX_NONE;
	}
	return Entries.IndexOfByPredicate([Instance](const FApecoxInventoryEntry& Entry)
	{
		return Entry.ItemInstance == Instance;
	});
}

void FApecoxInventoryList::AddEntry(UApecoxInventoryItemInstance* Instance, int32 Stack)
{
	if (!Instance)
	{
		return;
	}

	// 防重复——同一实例不重复条目
	if (FindEntryIndex(Instance) != INDEX_NONE)
	{
		return;
	}

	FApecoxInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemInstance = Instance;
	NewEntry.StackCount = Stack;

	// FastArray 必须维护自己的 ReplicationID/ReplicationKey；
	// MARK_PROPERTY_DIRTY_FROM_NAME 只服务于 Push Model，不能替代 MarkItemDirty
	MarkItemDirty(NewEntry);
}

void FApecoxInventoryList::RemoveEntry(UApecoxInventoryItemInstance* Instance)
{
	const int32 Idx = FindEntryIndex(Instance);
	if (Idx == INDEX_NONE)
	{
		return;
	}

	Entries.RemoveAt(Idx);
	MarkArrayDirty();
}

void FApecoxInventoryList::ClearAllEntries()
{
	if (Entries.Num() == 0)
	{
		return; // 幂等——空数组不标记脏
	}

	Entries.Reset();
	MarkArrayDirty();
}

// ============================================================================
// FApecoxWeaponSlotState
// ============================================================================

EApecoxWeaponSlot FApecoxWeaponSlotState::FindSlotForInstance(const UApecoxWeaponInstance* Instance) const
{
	if (PrimaryWeapon == Instance) return EApecoxWeaponSlot::Primary;
	if (SecondaryWeapon == Instance) return EApecoxWeaponSlot::Secondary;
	return EApecoxWeaponSlot::None;
}

void FApecoxWeaponSlotState::SetSlot(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* Instance)
{
	switch (Slot)
	{
	case EApecoxWeaponSlot::Primary:
		PrimaryWeapon = Instance;
		break;
	case EApecoxWeaponSlot::Secondary:
		SecondaryWeapon = Instance;
		break;
	default:
		ensureMsgf(false, TEXT("[Apecox] WeaponSlotState: Cannot set slot None."));
		break;
	}
}

void FApecoxWeaponSlotState::ClearSlot(EApecoxWeaponSlot Slot)
{
	SetSlot(Slot, nullptr);
}

void FApecoxWeaponSlotState::ClearAllSlots()
{
	PrimaryWeapon = nullptr;
	SecondaryWeapon = nullptr;
}

// ============================================================================
// UApecoxInventoryComponent
// ============================================================================

UApecoxInventoryComponent::UApecoxInventoryComponent()
	: InventoryList(this)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UApecoxInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Inventory 只能由 AApecoxPlayerController 拥有——PlayerController 跨 Pawn 存活，
	// 库存也应当跨 Pawn 保留（或在死亡时由 GameMode 显式清空）
	if (!ensureMsgf(Cast<AApecoxPlayerController>(GetOwner()),
		TEXT("[Apecox] UApecoxInventoryComponent must be owned by AApecoxPlayerController. "
			"Owner '%s' is invalid for inventory lifecycle."), *GetNameSafe(GetOwner())))
	{
		return;
	}
}

// ========================================================================
// 客户端拾取入口
// ========================================================================

void UApecoxInventoryComponent::RequestPickupWeapon(AApecoxWeaponPickup* Pickup)
{
	if (!Pickup)
	{
		return;
	}

	// 本地体验入口：从客户端提交 Server RPC
	// 服务器将完全重新验证距离、视线、死亡、空槽和 Claim——不信任客户端
	ServerRequestPickupWeapon(Pickup);
}

// ========================================================================
// Server RPC 实现
// ========================================================================

void UApecoxInventoryComponent::ServerRequestPickupWeapon_Implementation(AApecoxWeaponPickup* Pickup)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (!Pickup)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Null pickup."));
		return;
	}

	// 验证控制器和 Pawn
	AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(GetOwner());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Invalid Owner Controller."));
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: No pawn possessed."));
		return;
	}

	// 验证 Pawn 是有效的 AApecoxPlayerCharacter
	const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(Pawn);
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Pawn is not AApecoxPlayerCharacter."));
		return;
	}

	// 服务器验证 Pawn 是否存活——死亡中 Pawn 的晚到 RPC 必须拒绝，
	// 不能只依赖客户端输入阻断。通过 Character 现有 HealthComponent 查询，
	// 不修改 AttributeSet、DeathAbility，不新增 Getter 或临时 GameplayTag。
	const UApecoxHealthComponent* HealthComp = Character->GetHealthComponent();
	if (!HealthComp || HealthComp->IsDeadOrDying())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Pawn '%s' is dead or dying."),
			*GetNameSafe(Pawn));
		return;
	}

	// 验证拾取物是否可被此 Pawn 拾取（距离、视线、Claim 状态）
	if (!Pickup->CanBePickedUpBy(Pawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Pickup '%s' cannot be picked up by '%s'."),
			*GetNameSafe(Pickup), *GetNameSafe(Pawn));
		return;
	}

	// 验证 Definition
	const UApecoxWeaponDefinition* WeaponDef = Pickup->GetWeaponDefinition();
	if (!WeaponDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: Pickup has no weapon definition."));
		return;
	}

	// 验证空槽位
	if (FindFirstFreeWeaponSlot() == EApecoxWeaponSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] ServerRequestPickupWeapon: No free weapon slot for '%s'."),
			*GetNameSafe(Pawn));
		return;
	}

	// 执行原子事务
	TryAddAndEquipWeapon(WeaponDef, Pickup);
}

// ========================================================================
// 库存查询
// ========================================================================

EApecoxWeaponSlot UApecoxInventoryComponent::FindFirstFreeWeaponSlot() const
{
	// Primary 优先
	if (!WeaponSlotState.PrimaryWeapon)
	{
		return EApecoxWeaponSlot::Primary;
	}
	if (!WeaponSlotState.SecondaryWeapon)
	{
		return EApecoxWeaponSlot::Secondary;
	}
	return EApecoxWeaponSlot::None;
}

UApecoxWeaponInstance* UApecoxInventoryComponent::GetWeaponInSlot(EApecoxWeaponSlot Slot) const
{
	switch (Slot)
	{
	case EApecoxWeaponSlot::Primary:
		return WeaponSlotState.PrimaryWeapon;
	case EApecoxWeaponSlot::Secondary:
		return WeaponSlotState.SecondaryWeapon;
	default:
		return nullptr;
	}
}

// ========================================================================
// 原子拾取事务
// ========================================================================

UApecoxWeaponInstance* UApecoxInventoryComponent::TryAddAndEquipWeapon(
	const UApecoxWeaponDefinition* WeaponDef, AApecoxWeaponPickup* Pickup)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return nullptr;
	}

	if (!WeaponDef)
	{
		return nullptr;
	}

	// 1. 验证 InstanceClass 是 UApecoxWeaponInstance 子类
	const TSubclassOf<UApecoxInventoryItemInstance> InstanceClass = WeaponDef->InstanceClass;
	if (!ensureMsgf(InstanceClass,
		TEXT("[Apecox] WeaponDef '%s' has no InstanceClass configured."), *WeaponDef->GetName()))
	{
		return nullptr;
	}

	if (!InstanceClass->IsChildOf(UApecoxWeaponInstance::StaticClass()))
	{
		ensureMsgf(false,
			TEXT("[Apecox] WeaponDef '%s' InstanceClass '%s' is not a UApecoxWeaponInstance subclass."),
			*WeaponDef->GetName(), *InstanceClass->GetName());
		return nullptr;
	}

	// 2. 原子占用拾取物——TryClaim 防止两个 RPC 顺序到达时重复发放
	if (Pickup && !Pickup->TryClaim())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] Pickup already claimed."));
		return nullptr;
	}

	// 3. 验证空槽位（在 Claim 之后再次检查，防止竞态）
	const EApecoxWeaponSlot FreeSlot = FindFirstFreeWeaponSlot();
	if (FreeSlot == EApecoxWeaponSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] No free weapon slot."));
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	// 4. 验证 Character 和 EquipmentComponent 存在
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] TryAddAndEquipWeapon: Invalid Owner Controller."));
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(PC->GetPawn());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] TryAddAndEquipWeapon: No valid Character pawn."));
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	UApecoxEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] TryAddAndEquipWeapon: No EquipmentComponent on Character."));
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	// 5. 创建运行时 WeaponInstance——Outer 为 PlayerController
	AActor* Owner = GetOwner();
	UApecoxWeaponInstance* WeaponInstance = NewObject<UApecoxWeaponInstance>(
		Owner, InstanceClass);
	if (!WeaponInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[Apecox] Failed to create WeaponInstance."));
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	// 初始化实例（设置不可变 Definition）
	WeaponInstance->Initialize(WeaponDef);

	// 6. 注册为复制 Subobject
	RegisterInstance(WeaponInstance);

	// 7. 加入 FastArray 条目——AddEntry 内部调用 MarkItemDirty
	InventoryList.AddEntry(WeaponInstance, 1);

	// 8. 放入槽位
	WeaponSlotState.SetSlot(FreeSlot, WeaponInstance);
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, WeaponSlotState, this);

	// 9. 装备武器——EquipWeapon 返回 false 时必须回滚所有已执行步骤
	if (!Equipment->EquipWeapon(FreeSlot, WeaponInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon failed for '%s' — rolling back."),
			*WeaponDef->GetName());

		// 逆序回滚：槽位 → FastArray 条目 → Registered Subobject → Claim
		WeaponSlotState.ClearSlot(FreeSlot);
		MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, WeaponSlotState, this);

		InventoryList.RemoveEntry(WeaponInstance);

		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(WeaponInstance);
		}
		RegisteredInstances.Remove(WeaponInstance);

		// GC 自然回收——解除全部强引用和复制注册后不调用 MarkAsGarbage
		if (Pickup) Pickup->ReleaseClaim();
		return nullptr;
	}

	// 10. 广播委托（驱动任何额外监听者）
	OnWeaponAdded.ExecuteIfBound(FreeSlot, WeaponInstance);

	// 11. 事务成功——销毁拾取 Actor
	if (Pickup)
	{
		Pickup->Consume();
	}

	UE_LOG(LogTemp, Log, TEXT("[Apecox] TryAddAndEquipWeapon: Successfully added '%s' to slot %d."),
		*WeaponDef->GetName(), static_cast<int32>(FreeSlot));

	return WeaponInstance;
}

// ========================================================================
// 死亡清理
// ========================================================================

void UApecoxInventoryComponent::ClearInventoryForDeath()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// 先清除槽位引用——不销毁实例自身（由 DestroyInstance 负责）
	WeaponSlotState.ClearAllSlots();
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, WeaponSlotState, this);

	// 从 FastArray 移除所有条目并注销 Subobject
	TArray<UApecoxInventoryItemInstance*> InstancesToDestroy;
	for (const FApecoxInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Entry.ItemInstance)
		{
			InstancesToDestroy.Add(Entry.ItemInstance);
		}
	}

	for (UApecoxInventoryItemInstance* Instance : InstancesToDestroy)
	{
		DestroyInstance(Instance);
	}

	// ClearAllEntries 内部调用 MarkArrayDirty——幂等，只有确实存在条目时才标记
	InventoryList.ClearAllEntries();

	UE_LOG(LogTemp, Log, TEXT("[Apecox] ClearInventoryForDeath: %d instances destroyed."),
		InstancesToDestroy.Num());
}

// ========================================================================
// 实例生命周期
// ========================================================================

void UApecoxInventoryComponent::RegisterInstance(UApecoxInventoryItemInstance* Instance)
{
	if (!Instance || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// 防止重复注册
	if (RegisteredInstances.Contains(Instance))
	{
		return;
	}

	RegisteredInstances.Add(Instance);

	// AddReplicatedSubObject：引擎创建 NetGUID 和复制通道
	if (IsUsingRegisteredSubObjectList())
	{
		AddReplicatedSubObject(Instance);
	}
}

void UApecoxInventoryComponent::DestroyInstance(UApecoxInventoryItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	// 从所有武器槽中清除——只在找到真实槽位时操作，避免将 None 传入 ClearSlot
	if (UApecoxWeaponInstance* WeaponInst = Cast<UApecoxWeaponInstance>(Instance))
	{
		const EApecoxWeaponSlot FoundSlot = WeaponSlotState.FindSlotForInstance(WeaponInst);
		if (FoundSlot != EApecoxWeaponSlot::None)
		{
			WeaponSlotState.ClearSlot(FoundSlot);
			MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxInventoryComponent, WeaponSlotState, this);
		}
	}

	// 从 Registered Subobject 列表注销
	if (IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(Instance);
	}
	RegisteredInstances.Remove(Instance);

	// 从 FastArray 条目移除——RemoveEntry 内部调用 MarkArrayDirty
	InventoryList.RemoveEntry(Instance);

	// 解除全部强引用和复制注册后让 GC 自然回收——不调用 MarkAsGarbage
}

// ========================================================================
// 复制生命周期
// ========================================================================

void UApecoxInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// 注册已存在的实例——确保晚加入的客户端获得完整 Subobject 状态
	if (IsUsingRegisteredSubObjectList())
	{
		for (UApecoxInventoryItemInstance* Instance : RegisteredInstances)
		{
			if (Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

bool UApecoxInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
	FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// 传统兼容路径：如果未使用 Registered Subobject List，则在此逐个序列化实例
	for (UApecoxInventoryItemInstance* Instance : RegisteredInstances)
	{
		if (Instance)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

// ========================================================================
// 复制注册
// ========================================================================

void UApecoxInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 库存仅复制到 Owner——其他玩家不需要知道私有持有物
	DOREPLIFETIME_CONDITION(UApecoxInventoryComponent, InventoryList, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UApecoxInventoryComponent, WeaponSlotState, COND_OwnerOnly);
}
