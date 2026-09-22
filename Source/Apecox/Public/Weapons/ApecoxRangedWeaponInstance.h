// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/ApecoxWeaponInstance.h"
#include "ApecoxRangedWeaponInstance.generated.h"

struct FApecoxWeaponFireConfig;
struct FApecoxRangedFireConfig;
struct FApecoxProjectileFireConfig;

UENUM(BlueprintType)
enum class EApecoxWeaponReloadState : uint8
{
	None,
	Tactical,
	Empty
};

/**
 * UApecoxRangedWeaponInstance
 * - 远程武器运行时实例——保存单把武器的可变运行时状态。
 * - 弹匣（CurrentMagazineAmmo）只对 Owner 可见复制；
 *   客户端预测射击只更新本地射速时间和 Shot ID，不权威修改弹匣。
 *
 * 为什么可变状态放在此处而非 WeaponDefinition：
 * - Definition 是跨所有同款实例共享的不可变配置（PrimaryDataAsset）；
 * - 弹匣剩余、射速计时和 Shot ID 账本是单把武器实例的私有可变状态，
 *   随拾取创建、随丢弃/死亡销毁——必须属于 UObject 实例。
 *
 * 为什么要区分 LastLocalFireTimeSeconds 和 LastServerFireTimeSeconds：
 * - 客户端根据本地时间控制射速节奏（预测开火不需要等待服务器确认）；
 * - 服务器根据权威时间验证射速——客户端可能因网络延迟而高频发送请求；
 * - Listen Server Host 通过 CanCommitServerShot 避免本地预测和 Authority 双扣。
 */
UCLASS()
class APECOX_API UApecoxRangedWeaponInstance : public UApecoxWeaponInstance
{
	GENERATED_BODY()

	friend class FApecoxWeaponReloadTest;

public:
	// ========================================================================
	// 初始化
	// ========================================================================

	/**
	 * Authority 创建路径 override：
	 * 1. 调用 Super::Initialize() 设置 ItemDefinition。
	 * 2. 从有效 FireConfig 初始化 CurrentMagazineAmmo。
	 * 客户端通过复制接收弹匣，不是自行读取 Definition 重建权威状态。
	 */
	virtual void Initialize(const UApecoxInventoryItemDefinition* InDefinition) override;

	// ========================================================================
	// 类型安全配置访问
	// ========================================================================

	/** 只读访问远程武器定义——等价于 Cast<UApecoxWeaponDefinition>(GetItemDefinition()) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	const UApecoxWeaponDefinition* GetRangedWeaponDefinition() const;

	const FApecoxRangedFireConfig* GetRangedFireConfig() const;
	const FApecoxProjectileFireConfig* GetProjectileFireConfig() const;

	// ========================================================================
	// 客户端本地射击节奏（不权威扣弹）
	// ========================================================================

	/**
	 * 根据本地时间、RPM 和已复制弹匣判断是否可以发起预测射击。
	 * 不修改任何状态——只是检查门控条件。
	 */
	bool CanStartLocalShot(float WorldTimeSeconds) const;

	/**
	 * 更新本地射速时间并生成非零 Shot ID——不权威扣弹。
	 * 必须在 CanStartLocalShot 返回 true 之后调用。
	 * @param WorldTimeSeconds 当前世界时间（客户端本地）
	 * @return 新生成的唯一 Shot ID
	 */
	uint32 StartLocalShot(float WorldTimeSeconds);

	/** 写入下一发TargetData的本地连发身份与刚刚发起的索引。 */
	uint32 GetLocalBurstId() const { return LocalBurstId; }
	int32 GetLocalBurstShotCount() const { return LocalBurstShotCount; }
	int32 GetLastStartedLocalBurstShotIndex() const { return FMath::Max(LocalBurstShotCount - 1, 0); }

	/** 开火键松开：下一次射击从新Burst的索引0开始。 */
	void EndLocalFireBurst();

	// ========================================================================
	// 服务端权威射击验证与结算
	// ========================================================================

	/**
	 * 检查 Shot ID、RPM 和当前弹匣——不修改状态。
	 * 执行顺序：先 CanCommit 所有请求合法性检查，通过后再 Commit。
	 * 避免扣弹后发现请求非法、或先应用伤害后发现 Shot ID 重复。
	 */
	bool CanCommitServerShot(uint32 ShotId, float WorldTimeSeconds, uint32 BurstId, int32 BurstShotIndex) const;

	/**
	 * 仅 Authority 调用——原子更新服务器射速账本、Shot ID 账本并扣一发弹药。
	 * 必须在 CanCommitServerShot 返回 true 之后调用。
	 * 合法 Miss 也扣弹；Rejected 不消耗弹药。
	 * @return true 弹药成功扣除；false 表示非 Authority、内部重检失败或条件不满足
	 */
	bool CommitServerShot(uint32 ShotId, float WorldTimeSeconds, uint32 BurstId, int32 BurstShotIndex);

	int32 GetServerBurstShotCount() const { return ServerBurstShotCount; }

	// ========================================================================
	// 弹匣访问
	// ========================================================================

	/** 只读弹匣访问——为未来 HUD 保留入口 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetCurrentMagazineAmmo() const { return CurrentMagazineAmmo; }

	/** 弹匣是否为空 */
	bool IsMagazineEmpty() const { return CurrentMagazineAmmo <= 0; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	EApecoxWeaponReloadState GetReloadState() const { return ReloadState; }

	bool IsReloading() const { return ReloadState != EApecoxWeaponReloadState::None; }
	bool CanReload() const;

	/** Authority 开始换弹事务；根据开始时弹匣是否为空冻结普通/空仓分支。 */
	bool BeginReload();

	/** Authority 在配置提交点原子转移弹药；同一事务重复调用不会重复加弹。 */
	bool CommitReload();

	/** Authority 结束或取消换弹；已提交的弹药不回滚。 */
	void FinishReload();

protected:
	/** 弹匣复制回调——为未来 HUD 更新保留入口 */
	UFUNCTION()
	void OnRep_CurrentMagazineAmmo();

	UFUNCTION()
	void OnRep_ReserveAmmo();

	UFUNCTION()
	void OnRep_ReloadState();

	// ========================================================================
	// 复制成员
	// ========================================================================

	/**
	 * 当前弹匣剩余——只对 Owner 可见，由服务端权威扣除。
	 * 客户端通过复制接收，不是从 Definition 自行计算。
	 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentMagazineAmmo)
	int32 CurrentMagazineAmmo = 0;

	/** 当前单武器阶段的备用弹药；未来完整 Ammo Type 系统可替换其来源。 */
	UPROPERTY(ReplicatedUsing = OnRep_ReserveAmmo)
	int32 ReserveAmmo = 0;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadState)
	EApecoxWeaponReloadState ReloadState = EApecoxWeaponReloadState::None;

	/** 仅 Authority 使用，防止同一换弹定时器重复提交。 */
	bool bReloadAmmoCommitted = false;

	// ========================================================================
	// 非复制成员
	// ========================================================================

	/** Owning Client/Listen Host 的本地射速节奏——不复制 */
	float LastLocalFireTimeSeconds = 0.0f;

	/** Authority 对该武器的权威射速账本——不复制 */
	float LastServerFireTimeSeconds = 0.0f;

	/** 客户端生成的下一 Shot ID——不复制，非零递增 */
	uint32 NextLocalShotId = 0;

	/** Authority 最后接受的 Shot ID——预防重复/倒序射击事务重复结算 */
	uint32 LastAcceptedShotId = 0;

	/** 本地预测Burst从1开始；0保留为无效网络值。 */
	uint32 LocalBurstId = 1;
	int32 LocalBurstShotCount = 0;

	/** Authority只接受同一Burst不倒退的索引；新Burst必须从0开始。 */
	uint32 LastAcceptedBurstId = 0;
	int32 ServerBurstShotCount = 0;
};
