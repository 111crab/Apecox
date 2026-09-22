// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "ApecoxWeaponStateComponent.generated.h"

/**
 * EApecoxShotConfirmation
 * - 服务器对客户端的一次射击请求的最终判定。
 * - Rejected：请求被拒绝（非法 Shot ID、RPM 违规、弹药不足、武器已卸下等），未消耗弹药。
 * - Miss：请求合法、弹药已消耗，但未能命中有效目标。
 * - ConfirmedHit：请求合法、弹药已消耗、且成功向有效目标（有 ASC）应用了 Damage GE。
 * - Launched：实体子弹已生成且弹药已消耗；最终飞行结果另走OnProjectileResolved。
 *   Projectile的ConfirmedHit还要求Health确实下降，不能把空GE或免疫当作受伤。
 */
UENUM(BlueprintType)
enum class EApecoxShotConfirmation : uint8
{
	Rejected		UMETA(DisplayName = "Rejected"),
	Miss			UMETA(DisplayName = "Miss"),
	ConfirmedHit	UMETA(DisplayName = "ConfirmedHit"),
	Launched        UMETA(DisplayName = "Launched")
};

/** 单次未确认射击的本地记录 */
USTRUCT()
struct FApecoxUnconfirmedShot
{
	GENERATED_BODY()

	/** 射击标识——与 TargetData 中的 ShotId 配对 */
	UPROPERTY()
	uint32 ShotId = 0;

	/** 客户端候选是否命中——预测表现用途 */
	bool bCandidateWasHit = false;

	bool operator==(uint32 InShotId) const { return ShotId == InShotId; }
};

/** 射击确认结果委托——未来 HUD 可订阅 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FApecoxShotConfirmedDelegate, uint32, ShotId, EApecoxShotConfirmation, Result);

/** 实体弹丸最终结算；DamagedActor 仅在实际造成伤害时有效。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FApecoxProjectileResolvedDelegate, uint32, ShotId,
	EApecoxShotConfirmation, Result, AActor*, DamagedActor);

/**
 * UApecoxWeaponStateComponent
 * - 由 AApecoxPlayerController 创建并默认复制。
 * - 本地保存未确认 Shot ID 和候选命中；不能无限增长。
 * - Server 通过 Client RPC ClientConfirmShot 返回该发的最终结果。
 * - Client 移除未确认记录并广播确认结果。
 *
 * 它不执行 Trace、不扣弹、不应用 GE、不保存 Weapon Definition——
 * 这些职责属于 UApecoxRangedFireAbility、其弹道实现和 UApecoxRangedWeaponInstance。
 *
 * 与 Lyra 的区别：
 * - Lyra 的 ULyraWeaponStateComponent 负责命中标记 UI、伤害时间追踪和 Tick 驱动武器散热；
 * - Apecox V1 只保留 Shot 确认/清理核心功能，不实现命中标记 UI 和武器散热。
 */
UCLASS(ClassGroup = (Apecox), meta = (BlueprintSpawnableComponent))
class APECOX_API UApecoxWeaponStateComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	explicit UApecoxWeaponStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ========================================================================
	// 客户端：记录未确认射击
	// ========================================================================

	/**
	 * Owning Client 发起预测射击时调用——记录 Shot ID 和候选结果。
	 * 清理超量旧记录（保留最多 MaxUnconfirmedShots 条最早未确认的记录）。
	 */
	void RecordUnconfirmedShot(uint32 ShotId, bool bCandidateWasHit);

	// ========================================================================
	// 服务器：发送确认
	// ========================================================================

	/**
	 * Authority 验证并结算射击后调用——通过 Client RPC 向 Owning Client 返回最终结果。
	 * @param ShotId 被确认的射击标识
	 * @param Result Rejected / Miss / ConfirmedHit
	 */
	void SendShotConfirmation(uint32 ShotId, EApecoxShotConfirmation Result);

    /** Flight result is separate: it must not remove a newer weapon's same-number prediction. */
	void SendProjectileResult(uint32 ShotId, EApecoxShotConfirmation Result,
		AActor* DamagedActor = nullptr);
    UPROPERTY(BlueprintAssignable, Category="Apecox|Weapon")
	FApecoxProjectileResolvedDelegate OnProjectileResolved;

	// ========================================================================
	// 访问器
	// ========================================================================

	/** 射击确认委托——未来 HUD 可订阅以显示命中标记或弹药 UI */
	UPROPERTY(BlueprintAssignable, Category = "Apecox|Weapon")
	FApecoxShotConfirmedDelegate OnShotConfirmed;

protected:
	/** Client RPC——服务器向 Owning Client 返回射击最终结果 */
	UFUNCTION(Client, Reliable)
	void ClientConfirmShot(uint32 ShotId, EApecoxShotConfirmation Result);

    UFUNCTION(Client, Reliable)
	void ClientResolveProjectile(uint32 ShotId, EApecoxShotConfirmation Result,
		AActor* DamagedActor);

	/** 清理所有未确认记录 */
	void ClearAllUnconfirmed();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * 未确认射击记录——不能无限增长。
	 * 确认时移除匹配 Shot ID；超量时清理最早的记录。
	 */
	UPROPERTY()
	TArray<FApecoxUnconfirmedShot> UnconfirmedShots;

	/** 最大未确认射击数——防止无限增长 */
	static constexpr int32 MaxUnconfirmedShots = 32;
};
