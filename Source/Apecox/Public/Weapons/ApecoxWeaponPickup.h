// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ApecoxWeaponPickup.generated.h"

class USphereComponent;
class USkeletalMeshComponent;
class UApecoxWeaponDefinition;

/**
 * AApecoxWeaponPickup
 * - 世界中可拾取的武器 Actor。
 * - Server Authority 管理 Claim 事务，确保两个客户端不能同时拾取同一个 Actor。
 * - 客户端仅提交候选——Server RPC 重新验证所有条件。
 *
 * 世界拾取 Actor 为什么不声明客户端 Server RPC：
 * - Pickup 是独立 Actor，不属于任何客户端，因此引擎不会将客户端 RPC 路由给它。
 * - 拾取请求由 PlayerController 拥有的 InventoryComponent 发起 Server RPC，
 *   在服务器完成验证后操作此 Actor。
 */
UCLASS()
class APECOX_API AApecoxWeaponPickup : public AActor
{
	GENERATED_BODY()

public:
	AApecoxWeaponPickup();

	// ========================================================================
	// 验证和事务
	// ========================================================================

	/** 服务器验证此 Pawn 是否可以拾取——距离、视线、Pawn 存活、未被 Claim */
	bool CanBePickedUpBy(const APawn* Pawn) const;

	/** 原子占用此拾取物——如果已被 Claim 返回 false */
	bool TryClaim();

	/** 释放占用（回滚时使用） */
	void ReleaseClaim();

	/** 事务成功后服务器消耗此 Actor——销毁拾取物 */
	void Consume();

	// ========================================================================
	// 访问器
	// ========================================================================

	const UApecoxWeaponDefinition* GetWeaponDefinition() const { return WeaponDefinition; }

protected:
	virtual void BeginPlay() override;

	/** WeaponDefinition 复制回调——刷新地面 Mesh 表现 */
	UFUNCTION()
	void OnRep_WeaponDefinition();

	/** 根据 PresentationDefinition 刷新 PickupMesh 的 SkeletalMesh */
	void RefreshPickupPresentation();

	// ========================================================================
	// 组件
	// ========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 交互球体——仅 Query，可被 Visibility Trace 命中 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 地面武器 Mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<USkeletalMeshComponent> PickupMesh;

	// ========================================================================
	// 数据
	// ========================================================================

	/** 拾取物代表的武器定义——复制并通过 RepNotify 刷新 Mesh */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponDefinition, EditDefaultsOnly, Category = "Pickup")
	TObjectPtr<const UApecoxWeaponDefinition> WeaponDefinition;

	/** 服务器交互验证距离——可在蓝图中按武器大小微调 */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float InteractionDistance = 250.0f;

	/** 瞬时事务状态——服务器标记此拾取物正在被某个客户端拾取 */
	bool bClaimed = false;
};
