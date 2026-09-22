// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ApecoxInventoryComponent.generated.h"

class UApecoxInventoryItemInstance;
class UApecoxWeaponInstance;
class AApecoxWeaponPickup;
class AApecoxPlayerController;
class UApecoxInventoryComponent;

// ============================================================================
// EApecoxWeaponSlot
// ============================================================================

/** 武器槽位——使用枚举而非 GameplayTag，因为这是封闭、类型稳定的槽位集合，不参与 GAS 关系查询。 */
UENUM(BlueprintType)
enum class EApecoxWeaponSlot : uint8
{
	/** 空手/无当前槽位——不是可存储槽，是哨兵值 */
	None		UMETA(DisplayName = "None"),

	/** 第一普通武器槽（主武器） */
	Primary		UMETA(DisplayName = "Primary"),

	/** 第二普通武器槽（副武器） */
	Secondary	UMETA(DisplayName = "Secondary")
};

// ============================================================================
// FApecoxInventoryEntry — FastArray Item
// ============================================================================

/** 单个库存条目——FastArray 复制的基本单元，武器 StackCount 固定为 1 */
USTRUCT()
struct FApecoxInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	/** 运行时物品实例（Registered Subobject） */
	UPROPERTY()
	TObjectPtr<UApecoxInventoryItemInstance> ItemInstance = nullptr;

	/** 当前堆叠数量——武器固定为 1，为未来弹药/消耗品预留 */
	UPROPERTY()
	int32 StackCount = 0;
};

// ============================================================================
// FApecoxInventoryList — FastArray Container
// ============================================================================

/** 库存条目 FastArray 容器——负责 Delta 复制、增删和只读查询 */
USTRUCT()
struct FApecoxInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FApecoxInventoryList() : OwnerComponent(nullptr) {}
	explicit FApecoxInventoryList(UApecoxInventoryComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

	/** 条目列表——通过 FastArray Delta Serialization 同步到 Owner */
	UPROPERTY()
	TArray<FApecoxInventoryEntry> Entries;

	// --- FFastArraySerializer 接口 ---
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms);
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);

	// --- 库存查询 ---
	const FApecoxInventoryEntry* FindEntryByInstance(const UApecoxInventoryItemInstance* Instance) const;
	int32 FindEntryIndex(const UApecoxInventoryItemInstance* Instance) const;

	// --- 服务器增删（Authority Only） ---
	void AddEntry(UApecoxInventoryItemInstance* Instance, int32 Stack);
	void RemoveEntry(UApecoxInventoryItemInstance* Instance);
	void ClearAllEntries();

private:
	/** 非复制回指针——仅服务器端用于调用 MarkItemDirty/MarkArrayDirty */
	UPROPERTY(NotReplicated)
	TObjectPtr<UApecoxInventoryComponent> OwnerComponent = nullptr;
};

/** FastArray Delta Serialization 必须通过 trait 声明 */
template<>
struct TStructOpsTypeTraits<FApecoxInventoryList> : public TStructOpsTypeTraitsBase2<FApecoxInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

// ============================================================================
// FApecoxWeaponSlotState
// ============================================================================

/** 武器槽映射——与通用库存条目分离，避免弹药/电池被迫拥有武器槽字段 */
USTRUCT()
struct FApecoxWeaponSlotState
{
	GENERATED_BODY()

	/** 主武器槽中的实例——Authority Only 写入 */
	UPROPERTY()
	TObjectPtr<UApecoxWeaponInstance> PrimaryWeapon = nullptr;

	/** 副武器槽中的实例——Authority Only 写入 */
	UPROPERTY()
	TObjectPtr<UApecoxWeaponInstance> SecondaryWeapon = nullptr;

	/** 查找给定实例的槽位——未找到返回 None */
	EApecoxWeaponSlot FindSlotForInstance(const UApecoxWeaponInstance* Instance) const;

	/** 将实例放入指定槽位——Authority Only */
	void SetSlot(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* Instance);

	/** 从槽位中移除（不清除实例本身）——Authority Only */
	void ClearSlot(EApecoxWeaponSlot Slot);

	/** 所有槽位清空 */
	void ClearAllSlots();
};

// ============================================================================
// UApecoxInventoryComponent
// ============================================================================

/**
 * UApecoxInventoryComponent
 * - 由 AApecoxPlayerController 创建和拥有的私有库存。
 * - 为什么 Inventory 在 PlayerController 而非 Character：
 *   - 库存是玩家持久持有状态，不应随 Pawn 销毁而丢失。
 *   - 死亡/重生时 Character 被销毁重建，但 PlayerController 保持不变，
 *     库存可以在此跨 Pawn 保留或在死亡时显式清空。
 * - 使用 FastArray 复制条目，使用 Registered Subobject 管理实例生命周期。
 * - 客户端不能直接修改数组、槽位或实例——所有写操作通过 Server RPC。
 */
UCLASS(ClassGroup = (Apecox), meta = (BlueprintSpawnableComponent))
class APECOX_API UApecoxInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApecoxInventoryComponent();

	// ========================================================================
	// 初始化
	// ========================================================================

	/** 验证 Owner 必须是 AApecoxPlayerController，否则确保失败 */
	void InitializeComponent() override;

	// ========================================================================
	// 客户端拾取入口
	// ========================================================================

	/**
	 * 本地输入入口——客户端提交候选拾取物，由 Server RPC 重新验证。
	 * 为什么拾取必须由拥有客户端的组件发 RPC：
	 * - 世界拾取 Actor 不属于客户端，客户端不能在其上声明 Server RPC。
	 * - InventoryComponent 的 Outer 是 PlayerController（客户端拥有的 Actor），
	 *   因此 Server RPC 可以被正确路由。
	 */
	void RequestPickupWeapon(AApecoxWeaponPickup* Pickup);

	// ========================================================================
	// Server RPC
	// ========================================================================

	/**
	 * 服务器拾取验证——重新检查所有条件，不信任客户端。
	 * 事务流程：TryClaim -> 创建实例 -> 放入槽位 -> 通知装备 -> Consume。
	 * 任一步失败都回滚实例、槽位、Registered Subobject 和 Claim。
	 */
	UFUNCTION(Server, Reliable)
	void ServerRequestPickupWeapon(AApecoxWeaponPickup* Pickup);

	// ========================================================================
	// 库存查询（Authority 和 Owner 可访问）
	// ========================================================================

	/** 查找第一个空闲武器槽——Primary 优先，没有空槽返回 None */
	EApecoxWeaponSlot FindFirstFreeWeaponSlot() const;

	/** 获取指定槽位中的武器实例 */
	UApecoxWeaponInstance* GetWeaponInSlot(EApecoxWeaponSlot Slot) const;

	/** 获取 Primary 槽位实例——便捷访问 */
	UApecoxWeaponInstance* GetPrimaryWeapon() const { return WeaponSlotState.PrimaryWeapon; }

	// ========================================================================
	// 服务器武器管理
	// ========================================================================

	/**
	 * Authority 原子事务：验证、创建实例、放入槽位、通知装备。
	 * 失败时回滚所有副作用。成功返回实例，失败返回 nullptr。
	 */
	UApecoxWeaponInstance* TryAddAndEquipWeapon(
		const class UApecoxWeaponDefinition* WeaponDef, AApecoxWeaponPickup* Pickup);

	/**
	 * 死亡时清空所有武器槽位和库存实例。
	 * 当前规则是删除库存、空手重生——不生成世界掉落 Actor。
	 */
	void ClearInventoryForDeath();

	// ========================================================================
	// 复制与 Subobject 生命周期
	// ========================================================================

	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch,
		FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;

	// ========================================================================
	// 委托
	// ========================================================================

	/** 当装备的武器可能发生改变时广播——驱动 EquipmentComponent 更新 */
	DECLARE_DELEGATE_TwoParams(FOnWeaponAdded, EApecoxWeaponSlot /*Slot*/,
		UApecoxWeaponInstance* /*Instance*/);
	FOnWeaponAdded OnWeaponAdded;

private:
	/** 添加 Registered Subobject 并开始复制——创建设备后调用 */
	void RegisterInstance(UApecoxInventoryItemInstance* Instance);

	/** 注销并销毁实例——从所有槽位、库存条目和 Subobject 列表中移除 */
	void DestroyInstance(UApecoxInventoryItemInstance* Instance);

	// ========================================================================
	// 复制成员
	// ========================================================================

	/** 库存条目的 Delta 复制——仅复制到 Owner */
	UPROPERTY(Replicated)
	FApecoxInventoryList InventoryList;

	/** 武器槽映射——仅复制到 Owner */
	UPROPERTY(Replicated)
	FApecoxWeaponSlotState WeaponSlotState;

	/** 已注册 Subobject 的实例集合——Authority 创建时加入，删除时移除 */
	UPROPERTY()
	TArray<TObjectPtr<UApecoxInventoryItemInstance>> RegisteredInstances;
};
