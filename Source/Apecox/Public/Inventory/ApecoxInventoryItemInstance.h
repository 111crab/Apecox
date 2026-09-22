// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ApecoxInventoryItemInstance.generated.h"

class UApecoxInventoryItemDefinition;

/**
 * UApecoxInventoryItemInstance
 * - 运行时物品身份，Outer 固定为 AApecoxPlayerController。
 * - 使用 Registered Subobject 生命周期完成网络复制和 GC 安全。
 * - 只负责保存 Definition 和提供类型安全访问；武器可变状态（弹匣、散布等）归于子类。
 *
 * 为什么 UObject 需要 Registered Subobject 生命周期：
 * - 动态 UObject 必须通过 AActorComponent::AddReplicatedSubObject 注册，
 *   否则 ReplicationGraph 不知道它的存在，不会为它分配复制通道。
 * - 删除时必须调用 RemoveReplicatedSubObject 注销并标记 GC 可回收，
 *   否则在客户端可能残留悬空的 NetGUID 引用。
 */
UCLASS()
class APECOX_API UApecoxInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Authority 创建后只初始化一次——重复调用安全返回。
	 * 改为 virtual 允许 UApecoxRangedWeaponInstance override 并在弹匣初始化后调用 Super。
	 */
	virtual void Initialize(const UApecoxInventoryItemDefinition* InDefinition);

	/** 只读访问不可变定义 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	const UApecoxInventoryItemDefinition* GetItemDefinition() const { return ItemDefinition; }

	// 支持作为 Subobject 网络复制
	virtual bool IsSupportedForNetworking() const override { return true; }

	// 从 Outer Actor 安全获取世界——CDO 或 Outer 无效时返回 nullptr
	virtual UWorld* GetWorld() const override;

protected:
	/** 不可变物品定义——创建时设置，从不改变 */
	UPROPERTY(Replicated)
	TObjectPtr<const UApecoxInventoryItemDefinition> ItemDefinition;
};
