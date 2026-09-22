// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Inventory/ApecoxInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/World.h"

// ========================================================================
// 服务端验证常量——集中定义，不加入每武器配置
// ========================================================================

/** 射速比较允许的浮点容差（秒）——防止因精度问题误拒合法射击 */
static constexpr float FireRateTolerance = 0.001f;

// ========================================================================
// 初始化
// ========================================================================

void UApecoxRangedWeaponInstance::Initialize(const UApecoxInventoryItemDefinition* InDefinition)
{
	// 必须先调用 Super 设置 ItemDefinition——后续 GetFireConfig 依赖 Definition 已设置
	Super::Initialize(InDefinition);

	// 只在 Authority 创建路径从有效 FireConfig 初始化弹匣
	// 客户端通过复制接收 CurrentMagazineAmmo，不自行读取 Definition 重建权威状态
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
	{
		const UApecoxWeaponDefinition* WeaponDef = GetRangedWeaponDefinition();
		if (WeaponDef)
		{
			if (const FApecoxWeaponFireConfig* FireConfig = WeaponDef->GetFireConfig())
			{
				if (FireConfig->IsValidFireConfig())
				{
					CurrentMagazineAmmo = FireConfig->MagazineCapacity;
					ReserveAmmo = FMath::Max(WeaponDef->ReloadConfig.InitialReserveAmmo, 0);
				}
				else
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[Apecox] RangedWeaponInstance::Initialize: FireConfig on '%s' is invalid "
							"(MagazineCapacity=%d, RPM=%.1f). Ammo set to 0."),
						*WeaponDef->GetName(), FireConfig->MagazineCapacity, FireConfig->RoundsPerMinute);
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Apecox] RangedWeaponInstance::Initialize: No FireConfig on '%s'. Ammo remains 0."),
					*WeaponDef->GetName());
			}
		}
	}
}

// ========================================================================
// 类型安全配置访问
// ========================================================================

const UApecoxWeaponDefinition* UApecoxRangedWeaponInstance::GetRangedWeaponDefinition() const
{
	return Cast<UApecoxWeaponDefinition>(GetItemDefinition());
}

// ========================================================================
// 客户端本地射击节奏
// ========================================================================

bool UApecoxRangedWeaponInstance::CanStartLocalShot(float WorldTimeSeconds) const
{
	if (IsReloading())
	{
		return false;
	}
	// 弹匣为空无法射击
	if (CurrentMagazineAmmo <= 0)
	{
		return false;
	}

	// 从 Definition 获取 RPM 计算射速间隔
	const UApecoxWeaponDefinition* WeaponDef = GetRangedWeaponDefinition();
	if (!WeaponDef)
	{
		return false;
	}

	const FApecoxWeaponFireConfig* FireConfig = WeaponDef->GetFireConfig();
	if (!FireConfig || !FireConfig->IsValidFireConfig())
	{
		return false;
	}

	// 客户端射速门控——使用本地时间与本地计时
	const float ShotInterval = 60.0f / FireConfig->RoundsPerMinute;
	const float TimeSinceLastShot = WorldTimeSeconds - LastLocalFireTimeSeconds;

	// 首次射击（LastLocalFireTimeSeconds == 0）直接允许
	if (LastLocalFireTimeSeconds <= 0.0f)
	{
		return true;
	}

	return TimeSinceLastShot >= (ShotInterval - FireRateTolerance);
}

uint32 UApecoxRangedWeaponInstance::StartLocalShot(float WorldTimeSeconds)
{
	// 更新本地射速计时
	LastLocalFireTimeSeconds = WorldTimeSeconds;

	// 生成非零 Shot ID——处理 uint32 回绕
	if (NextLocalShotId == 0)
	{
		// 首次射击：从 1 开始
		NextLocalShotId = 1;
	}

	const uint32 ShotId = NextLocalShotId;
	++NextLocalShotId;

	// 回绕时跳过 0（0 是哨兵值）
	if (NextLocalShotId == 0)
	{
		NextLocalShotId = 1;
	}
	++LocalBurstShotCount;

	return ShotId;
}

void UApecoxRangedWeaponInstance::EndLocalFireBurst()
{
	LocalBurstShotCount = 0;
	++LocalBurstId;
	if (LocalBurstId == 0)
	{
		LocalBurstId = 1;
	}
}

// ========================================================================
// 服务端权威射击验证与结算
// ========================================================================

bool UApecoxRangedWeaponInstance::CanCommitServerShot(uint32 ShotId, float WorldTimeSeconds,
	uint32 BurstId, int32 BurstShotIndex) const
{
	if (IsReloading())
	{
		return false;
	}
	// Shot ID 必须非零（0 是哨兵/无效值）
	if (ShotId == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Apecox] CanCommitServerShot: Shot ID is 0 — rejected."));
		return false;
	}
	if (BurstId == 0 || BurstShotIndex < 0)
	{
		return false;
	}
	if (LastAcceptedBurstId == 0)
	{
		if (BurstShotIndex != 0) { return false; }
	}
	else
	{
		const bool bBurstWrapped = LastAcceptedBurstId > 0xFFFF0000u && BurstId < 0x0000FFFFu;
		if (BurstId == LastAcceptedBurstId)
		{
			// A rejected request may leave a gap, but an accepted index may never move backwards.
			if (BurstShotIndex < ServerBurstShotCount) { return false; }
		}
		else if ((!bBurstWrapped && BurstId < LastAcceptedBurstId) || BurstShotIndex != 0)
		{
			return false;
		}
	}

	// 弹匣为空
	if (CurrentMagazineAmmo <= 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Apecox] CanCommitServerShot: Magazine empty — rejected."));
		return false;
	}

	// 从 Definition 获取 RPM
	const UApecoxWeaponDefinition* WeaponDef = GetRangedWeaponDefinition();
	if (!WeaponDef)
	{
		return false;
	}

	const FApecoxWeaponFireConfig* FireConfig = WeaponDef->GetFireConfig();
	if (!FireConfig || !FireConfig->IsValidFireConfig())
	{
		return false;
	}

	// Shot ID 回绕检测——新 ID 必须严格大于已接受的 Shot ID
	// uint32 回绕处理：如果 LastAcceptedShotId 接近 UINT32_MAX 且新 ID 很小（回绕后），允许通过
	if (LastAcceptedShotId > 0)
	{
		const bool bIsWrapAround = (LastAcceptedShotId > 0xFFFF0000) && (ShotId < 0x0000FFFF);
		if (!bIsWrapAround && ShotId <= LastAcceptedShotId)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[Apecox] CanCommitServerShot: Shot ID %u is out-of-order (last accepted=%u) — rejected."),
				ShotId, LastAcceptedShotId);
			return false;
		}
	}

	// 服务端权威射速验证——使用服务器世界时间
	const float ShotInterval = 60.0f / FireConfig->RoundsPerMinute;
	if (LastServerFireTimeSeconds > 0.0f)
	{
		const float TimeSinceLastShot = WorldTimeSeconds - LastServerFireTimeSeconds;
		if (TimeSinceLastShot < (ShotInterval - FireRateTolerance))
		{
			UE_LOG(LogTemp, Log,
				TEXT("[Apecox] CanCommitServerShot: RPM violation — %.4fs since last shot, "
					"requires >= %.4fs (ShotId=%u) — rejected."),
				TimeSinceLastShot, ShotInterval, ShotId);
			return false;
		}
	}

	return true;
}

bool UApecoxRangedWeaponInstance::CommitServerShot(uint32 ShotId, float WorldTimeSeconds,
	uint32 BurstId, int32 BurstShotIndex)
{
	// Authority guard——非 Authority 永远不得提交弹药扣除
	AActor* Owner = GetTypedOuter<AActor>();
	if (!Owner || !Owner->HasAuthority())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] CommitServerShot: Called on non-authority (Outer='%s') — rejected."),
			*GetNameSafe(Owner));
		return false;
	}

	// 在修改状态前再次执行必要条件检查——防止调用方跳过 CanCommit 直接调用 Commit
	if (!CanCommitServerShot(ShotId, WorldTimeSeconds, BurstId, BurstShotIndex))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] CommitServerShot: CanCommitServerShot return false in Commit guard — "
				"ShotId=%u rejected."),
			ShotId);
		return false;
	}

	// 更新服务端权威账本
	LastServerFireTimeSeconds = WorldTimeSeconds;
	LastAcceptedShotId = ShotId;
	LastAcceptedBurstId = BurstId;
	ServerBurstShotCount = BurstShotIndex + 1;

	// 扣除一发弹药——合法 Miss 也消耗
	--CurrentMagazineAmmo;

	// 通知复制系统——确保 Owner 客户端收到最新的弹匣值
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxRangedWeaponInstance, CurrentMagazineAmmo, this);

	UE_LOG(LogTemp, Log,
		TEXT("[Apecox] CommitServerShot: ShotId=%u accepted. Ammo=%d/%d."),
		ShotId,
		CurrentMagazineAmmo,
		GetRangedWeaponDefinition() ? GetRangedWeaponDefinition()->GetFireConfig()->MagazineCapacity : 0);

	return true;
}

// ========================================================================
// 弹匣复制
// ========================================================================

void UApecoxRangedWeaponInstance::OnRep_CurrentMagazineAmmo()
{
	// 为未来 HUD 更新保留入口——当前仅日志
	UE_LOG(LogTemp, Verbose,
		TEXT("[Apecox] RangedWeaponInstance: OnRep_CurrentMagazineAmmo = %d"), CurrentMagazineAmmo);
}

void UApecoxRangedWeaponInstance::OnRep_ReserveAmmo()
{
	UE_LOG(LogTemp, Verbose,
		TEXT("[Apecox] RangedWeaponInstance: OnRep_ReserveAmmo = %d"), ReserveAmmo);
}

void UApecoxRangedWeaponInstance::OnRep_ReloadState()
{
	UE_LOG(LogTemp, Verbose,
		TEXT("[Apecox] RangedWeaponInstance: OnRep_ReloadState = %d"),
		static_cast<int32>(ReloadState));
}

bool UApecoxRangedWeaponInstance::CanReload() const
{
	const UApecoxWeaponDefinition* WeaponDef = GetRangedWeaponDefinition();
	const FApecoxWeaponFireConfig* FireConfig = WeaponDef ? WeaponDef->GetFireConfig() : nullptr;
	return WeaponDef && WeaponDef->ReloadConfig.IsValid() && FireConfig
		&& CurrentMagazineAmmo >= 0 && CurrentMagazineAmmo < FireConfig->MagazineCapacity
		&& ReserveAmmo > 0 && !IsReloading();
}

bool UApecoxRangedWeaponInstance::BeginReload()
{
	AActor* Owner = GetTypedOuter<AActor>();
	if (!Owner || !Owner->HasAuthority() || !CanReload())
	{
		return false;
	}

	ReloadState = IsMagazineEmpty()
		? EApecoxWeaponReloadState::Empty : EApecoxWeaponReloadState::Tactical;
	bReloadAmmoCommitted = false;
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxRangedWeaponInstance, ReloadState, this);
	return true;
}

bool UApecoxRangedWeaponInstance::CommitReload()
{
	AActor* Owner = GetTypedOuter<AActor>();
	const UApecoxWeaponDefinition* WeaponDef = GetRangedWeaponDefinition();
	const FApecoxWeaponFireConfig* FireConfig = WeaponDef ? WeaponDef->GetFireConfig() : nullptr;
	if (!Owner || !Owner->HasAuthority() || !IsReloading() || bReloadAmmoCommitted || !FireConfig)
	{
		return false;
	}

	const int32 MissingAmmo = FMath::Max(FireConfig->MagazineCapacity - CurrentMagazineAmmo, 0);
	const int32 TransferAmmo = FMath::Min(MissingAmmo, ReserveAmmo);
	if (TransferAmmo <= 0)
	{
		return false;
	}

	CurrentMagazineAmmo += TransferAmmo;
	ReserveAmmo -= TransferAmmo;
	bReloadAmmoCommitted = true;
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxRangedWeaponInstance, CurrentMagazineAmmo, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxRangedWeaponInstance, ReserveAmmo, this);
	return true;
}

void UApecoxRangedWeaponInstance::FinishReload()
{
	AActor* Owner = GetTypedOuter<AActor>();
	if (!Owner || !Owner->HasAuthority() || !IsReloading())
	{
		return;
	}

	ReloadState = EApecoxWeaponReloadState::None;
	bReloadAmmoCommitted = false;
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxRangedWeaponInstance, ReloadState, this);
}

// ========================================================================
// 复制注册
// ========================================================================

void UApecoxRangedWeaponInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 弹匣只对 Owner 可见——远端表现由 EquippedWeaponState 摘要驱动
	DOREPLIFETIME_CONDITION(UApecoxRangedWeaponInstance, CurrentMagazineAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UApecoxRangedWeaponInstance, ReserveAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UApecoxRangedWeaponInstance, ReloadState, COND_OwnerOnly);
}

const FApecoxRangedFireConfig* UApecoxRangedWeaponInstance::GetRangedFireConfig() const
{
    const auto* Def = GetRangedWeaponDefinition();
    return Def ? Def->GetFireConfig<FApecoxRangedFireConfig>() : nullptr;
}

const FApecoxProjectileFireConfig* UApecoxRangedWeaponInstance::GetProjectileFireConfig() const
{
    const auto* Def = GetRangedWeaponDefinition();
    return Def ? Def->GetFireConfig<FApecoxProjectileFireConfig>() : nullptr;
}
