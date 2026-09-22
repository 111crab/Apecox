// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxWeaponStateComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UApecoxWeaponStateComponent::UApecoxWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

// ========================================================================
// 客户端：记录未确认射击
// ========================================================================

void UApecoxWeaponStateComponent::RecordUnconfirmedShot(uint32 ShotId, bool bCandidateWasHit)
{
	if (ShotId == 0)
	{
		return;
	}

	// 容量管理：超过上限时移除最早的记录
	if (UnconfirmedShots.Num() >= MaxUnconfirmedShots)
	{
		const uint32 RemovedShotId = UnconfirmedShots[0].ShotId;
		UnconfirmedShots.RemoveAt(0);

		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] WeaponStateComponent: Unconfirmed shots full (%d), "
				"removed oldest ShotId=%u."),
			MaxUnconfirmedShots, RemovedShotId);
	}

	FApecoxUnconfirmedShot& NewShot = UnconfirmedShots.Emplace_GetRef();
	NewShot.ShotId = ShotId;
	NewShot.bCandidateWasHit = bCandidateWasHit;

	UE_LOG(LogTemp, Log,
		TEXT("[Apecox] WeaponStateComponent [Client]: Predicted ShotId=%u (candidate hit=%s). "
			"Unconfirmed count=%d."),
		ShotId,
		bCandidateWasHit ? TEXT("yes") : TEXT("no"),
		UnconfirmedShots.Num());
}

// ========================================================================
// 服务器：发送确认
// ========================================================================

void UApecoxWeaponStateComponent::SendShotConfirmation(uint32 ShotId, EApecoxShotConfirmation Result)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	const TCHAR* ResultStr = TEXT("???");
	switch (Result)
	{
	case EApecoxShotConfirmation::Rejected:		ResultStr = TEXT("Rejected"); break;
	case EApecoxShotConfirmation::Launched:       ResultStr = TEXT("Launched"); break;
	case EApecoxShotConfirmation::Miss:			ResultStr = TEXT("Miss"); break;
	case EApecoxShotConfirmation::ConfirmedHit:	ResultStr = TEXT("ConfirmedHit"); break;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[Apecox] WeaponStateComponent [Authority]: ShotId=%u result=%s."),
		ShotId, ResultStr);

	ClientConfirmShot(ShotId, Result);
}

// ========================================================================
// Client RPC
// ========================================================================

void UApecoxWeaponStateComponent::ClientConfirmShot_Implementation(uint32 ShotId, EApecoxShotConfirmation Result)
{
	const TCHAR* ResultStr = TEXT("???");
	switch (Result)
	{
	case EApecoxShotConfirmation::Rejected:		ResultStr = TEXT("Rejected"); break;
	case EApecoxShotConfirmation::Launched:       ResultStr = TEXT("Launched"); break;
	case EApecoxShotConfirmation::Miss:			ResultStr = TEXT("Miss"); break;
	case EApecoxShotConfirmation::ConfirmedHit:	ResultStr = TEXT("ConfirmedHit"); break;
	}

	// 从未确认列表中移除匹配的 Shot ID
	const int32 FoundIndex = UnconfirmedShots.IndexOfByPredicate(
		[ShotId](const FApecoxUnconfirmedShot& Shot) { return Shot.ShotId == ShotId; });

	if (FoundIndex != INDEX_NONE)
	{
		UnconfirmedShots.RemoveAt(FoundIndex);
		UE_LOG(LogTemp, Log,
			TEXT("[Apecox] WeaponStateComponent [Client]: Confirmed ShotId=%u result=%s. "
				"Remaining unconfirmed=%d."),
			ShotId, ResultStr, UnconfirmedShots.Num());
	}
	else
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] WeaponStateComponent [Client]: Received confirmation for "
				"unknown ShotId=%u result=%s — may already be cleaned up or was rejected pre-prediction."),
			ShotId, ResultStr);
	}

	// 广播确认结果——未来 HUD 通过此委托显示命中标记
	OnShotConfirmed.Broadcast(ShotId, Result);
}

// ========================================================================
// 生命周期
// ========================================================================

void UApecoxWeaponStateComponent::ClearAllUnconfirmed()
{
	if (UnconfirmedShots.Num() > 0)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] WeaponStateComponent: Clearing %d unconfirmed shots."),
			UnconfirmedShots.Num());
		UnconfirmedShots.Empty();
	}
}

void UApecoxWeaponStateComponent::SendProjectileResult(
	uint32 ShotId, EApecoxShotConfirmation Result, AActor* DamagedActor)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		ClientResolveProjectile(ShotId, Result, DamagedActor);
	}
}

void UApecoxWeaponStateComponent::ClientResolveProjectile_Implementation(
	uint32 ShotId, EApecoxShotConfirmation Result, AActor* DamagedActor)
{
	OnProjectileResolved.Broadcast(ShotId, Result, DamagedActor);
}

void UApecoxWeaponStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllUnconfirmed();
	Super::EndPlay(EndPlayReason);
}
