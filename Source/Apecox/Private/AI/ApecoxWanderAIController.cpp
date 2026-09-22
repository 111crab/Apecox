// Copyright Apecox. All Rights Reserved.

#include "AI/ApecoxWanderAIController.h"

#include "AI/ApecoxAIObjectivePoint.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Character/ApecoxBotCharacter.h"
#include "Character/ApecoxHealthComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Game/ApecoxGameState.h"
#include "Player/ApecoxPlayerState.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"

AApecoxWanderAIController::AApecoxWanderAIController()
{
	// Apecox 的 ASC/属性集由 PlayerState 持有。AI 同样需要 PlayerState，才能直接复用角色的
	// InitializeAbilitySystem、受伤和死亡能力链路。
	bWantsPlayerState = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AApecoxWanderAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CombatRandom.Initialize(HashCombine(GetUniqueID(), InPawn ? InPawn->GetUniqueID() : 0));
	DecisionTimeRemaining = 0.0f;
	CombatRepositionTimeRemaining = 0.0f;
	ReactionTimeRemaining = 0.0f;
	FireBurstTimeRemaining = 0.0f;
	FirePauseTimeRemaining = 0.0f;
	LostSightTime = 0.0f;
	ObjectiveSearchTimeRemaining = 0.0f;
	ObjectiveLookTimeRemaining = 0.0f;
	bFireBurstActive = false;
	bSearchingObjective = false;
	CurrentAimOffset = FVector::ZeroVector;
	CombatTarget = nullptr;
	CurrentObjective = nullptr;
	if (AApecoxPlayerState* ApecoxPlayerState = GetPlayerState<AApecoxPlayerState>())
	{
		ApecoxPlayerState->SetCombatTeam(EApecoxCombatTeam::AI);
	}
}

void AApecoxWanderAIController::OnUnPossess()
{
	StopCombat();
	Super::OnUnPossess();
}

void AApecoxWanderAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AApecoxBotCharacter* Bot = Cast<AApecoxBotCharacter>(GetPawn());
	if (!HasAuthority() || !Bot)
	{
		return;
	}

	if (UApecoxAbilitySystemComponent* ASC = Bot->GetApecoxAbilitySystemComponent())
	{
		ASC->ProcessAbilityInput(DeltaSeconds, false);
	}
	CombatRepositionTimeRemaining = FMath::Max(CombatRepositionTimeRemaining - DeltaSeconds, 0.0f);
	ReactionTimeRemaining = FMath::Max(ReactionTimeRemaining - DeltaSeconds, 0.0f);
	ObjectiveSearchTimeRemaining = FMath::Max(ObjectiveSearchTimeRemaining - DeltaSeconds, 0.0f);
	ObjectiveLookTimeRemaining = FMath::Max(ObjectiveLookTimeRemaining - DeltaSeconds, 0.0f);
	if (bFireBurstActive)
	{
		FireBurstTimeRemaining -= DeltaSeconds;
		if (FireBurstTimeRemaining <= 0.0f)
		{
			EndFireBurst(Bot, true);
		}
	}
	else
	{
		FirePauseTimeRemaining = FMath::Max(FirePauseTimeRemaining - DeltaSeconds, 0.0f);
	}

	DecisionTimeRemaining -= DeltaSeconds;
	if (DecisionTimeRemaining <= 0.0f)
	{
		DecisionTimeRemaining = DecisionInterval;
		UpdateCombatDecision();
	}
}

AApecoxPlayerCharacter* AApecoxWanderAIController::FindNearestLivingPlayer()
{
	AApecoxPlayerCharacter* BestTarget = nullptr;
	float BestDistanceSquared = FMath::Square(AcquireRange);
	const APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<AApecoxPlayerCharacter> It(GetWorld()); It; ++It)
	{
		AApecoxPlayerCharacter* Candidate = *It;
		if (!IsValid(Candidate) || Candidate->IsA<AApecoxBotCharacter>() || Candidate->IsHidden())
		{
			continue;
		}
		const UApecoxHealthComponent* Health = Candidate->GetHealthComponent();
		if (!Health || Health->IsDeadOrDying())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared && LineOfSightTo(Candidate))
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}
	return BestTarget;
}

void AApecoxWanderAIController::UpdateCombatDecision()
{
	AApecoxBotCharacter* Bot = Cast<AApecoxBotCharacter>(GetPawn());
	const AApecoxGameState* ApecoxGameState = GetWorld() ? GetWorld()->GetGameState<AApecoxGameState>() : nullptr;
	if (!Bot || !ApecoxGameState || !ApecoxGameState->IsMatchInProgress())
	{
		StopCombat();
		return;
	}

	const UApecoxHealthComponent* BotHealth = Bot->GetHealthComponent();
	if (!BotHealth || BotHealth->IsDeadOrDying())
	{
		StopCombat();
		return;
	}

	AApecoxPlayerCharacter* PreviousTarget = CombatTarget;
	if (CombatTarget)
	{
		const bool bTargetObjectValid = IsValid(CombatTarget);
		const UApecoxHealthComponent* TargetHealth = bTargetObjectValid
			? CombatTarget->GetHealthComponent() : nullptr;
		const bool bTargetInvalid = !bTargetObjectValid || !TargetHealth
			|| TargetHealth->IsDeadOrDying() || CombatTarget->IsHidden();
		const bool bOutsideLeash = !bTargetInvalid
			&& FVector::DistSquared(Bot->GetActorLocation(), CombatTarget->GetActorLocation())
				> FMath::Square(LoseTargetRange);
		if (bTargetInvalid || bOutsideLeash)
		{
			ClearCombatTarget(Bot);
		}
		else if (LineOfSightTo(CombatTarget))
		{
			LostSightTime = 0.0f;
		}
		else
		{
			LostSightTime += DecisionInterval;
			if (LostSightTime >= LostSightGraceSeconds)
			{
				ClearCombatTarget(Bot);
			}
		}
	}
	if (!CombatTarget)
	{
		CombatTarget = FindNearestLivingPlayer();
	}
	if (CombatTarget != PreviousTarget)
	{
		EndFireBurst(Bot, false);
		ReactionTimeRemaining = CombatTarget
			? CombatRandom.FRandRange(ReactionDelayMin, FMath::Max(ReactionDelayMin, ReactionDelayMax))
			: 0.0f;
		FirePauseTimeRemaining = 0.0f;
		LostSightTime = 0.0f;
		CurrentAimOffset = FVector::ZeroVector;
	}

	if (!CombatTarget)
	{
		EndFireBurst(Bot, false);
		UpdateObjectiveMovement(Bot);
		return;
	}

	UpdateAimFocus(Bot, CombatTarget);
	const float Distance = FVector::Dist(Bot->GetActorLocation(), CombatTarget->GetActorLocation());
	const bool bCanSeeTarget = LineOfSightTo(CombatTarget);
	if (Distance <= FireRange && bCanSeeTarget)
	{
		if (Bot->IsAIWeaponMagazineEmpty())
		{
			EndFireBurst(Bot, true);
			StopMovement();
			Bot->TryStartAIWeaponReload();
		}
		else if (Bot->IsReloading())
		{
			EndFireBurst(Bot, true);
			StopMovement();
		}
		else
		{
			UpdateCombatMovement(Bot, CombatTarget);
			if (!bFireBurstActive && ReactionTimeRemaining <= 0.0f
				&& FirePauseTimeRemaining <= 0.0f)
			{
				BeginFireBurst(Bot, CombatTarget);
			}
		}
	}
	else
	{
		EndFireBurst(Bot, false);
		CurrentAimOffset = FVector::ZeroVector;
		UpdateAimFocus(Bot, CombatTarget);
		MoveToActor(CombatTarget, FireRange * 0.75f, true, true, true, nullptr, true);
	}
}

AApecoxAIObjectivePoint* AApecoxWanderAIController::ChooseObjectivePoint()
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<AApecoxAIObjectivePoint*> Candidates;
	for (TActorIterator<AApecoxAIObjectivePoint> It(GetWorld()); It; ++It)
	{
		AApecoxAIObjectivePoint* Point = *It;
		if (IsValid(Point) && Point->bEnabled && Point->SelectionWeight > 0.0f)
		{
			Candidates.Add(Point);
		}
	}
	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	// 多个点存在时避免原地重复选择；单点地图则允许在该点持续搜索。
	if (Candidates.Num() > 1 && CurrentObjective)
	{
		Candidates.Remove(CurrentObjective);
	}

	float TotalWeight = 0.0f;
	for (const AApecoxAIObjectivePoint* Point : Candidates)
	{
		TotalWeight += Point->SelectionWeight;
	}
	float Selection = CombatRandom.FRandRange(0.0f, TotalWeight);
	for (AApecoxAIObjectivePoint* Point : Candidates)
	{
		Selection -= Point->SelectionWeight;
		if (Selection <= 0.0f)
		{
			return Point;
		}
	}
	return Candidates.Last();
}

void AApecoxWanderAIController::UpdateObjectiveMovement(AApecoxBotCharacter* Bot)
{
	if (!Bot)
	{
		return;
	}

	if (!IsValid(CurrentObjective) || !CurrentObjective->bEnabled)
	{
		CurrentObjective = ChooseObjectivePoint();
		bSearchingObjective = false;
		ObjectiveSearchTimeRemaining = 0.0f;
		ObjectiveLookTimeRemaining = 0.0f;
	}
	if (!CurrentObjective)
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	const float DistanceSquared = FVector::DistSquared2D(
		Bot->GetActorLocation(), CurrentObjective->GetActorLocation());
	if (DistanceSquared > FMath::Square(CurrentObjective->AcceptanceRadius))
	{
		bSearchingObjective = false;
		ClearFocus(EAIFocusPriority::Gameplay);
		MoveToActor(CurrentObjective, CurrentObjective->AcceptanceRadius,
			true, true, true, nullptr, true);
		return;
	}

	StopMovement();
	if (!bSearchingObjective)
	{
		bSearchingObjective = true;
		ObjectiveSearchTimeRemaining = CombatRandom.FRandRange(
			CurrentObjective->SearchDurationMin,
			FMath::Max(CurrentObjective->SearchDurationMin, CurrentObjective->SearchDurationMax));
		ObjectiveLookTimeRemaining = 0.0f;
	}
	else if (ObjectiveSearchTimeRemaining <= 0.0f)
	{
		CurrentObjective = ChooseObjectivePoint();
		bSearchingObjective = false;
		ObjectiveLookTimeRemaining = 0.0f;
		return;
	}

	if (ObjectiveLookTimeRemaining <= 0.0f)
	{
		ObjectiveLookTimeRemaining = ObjectiveLookInterval;
		const float Yaw = CombatRandom.FRandRange(-180.0f, 180.0f);
		const FVector LookDirection = FRotator(0.0f, Yaw, 0.0f).Vector();
		SetFocalPoint(Bot->GetPawnViewLocation() + LookDirection * ObjectiveLookDistance,
			EAIFocusPriority::Gameplay);
	}
}

void AApecoxWanderAIController::ClearCombatTarget(AApecoxBotCharacter* Bot)
{
	EndFireBurst(Bot, false);
	CombatTarget = nullptr;
	LostSightTime = 0.0f;
	ReactionTimeRemaining = 0.0f;
	FirePauseTimeRemaining = 0.0f;
	CurrentAimOffset = FVector::ZeroVector;
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AApecoxWanderAIController::UpdateCombatMovement(
	AApecoxBotCharacter* Bot, AApecoxPlayerCharacter* Target)
{
	if (!Bot || !Target || CombatRepositionTimeRemaining > 0.0f || !GetWorld())
	{
		return;
	}

	CombatRepositionTimeRemaining = CombatRandom.FRandRange(
		CombatRepositionIntervalMin,
		FMath::Max(CombatRepositionIntervalMin, CombatRepositionIntervalMax));
	const FVector ToTarget = (Target->GetActorLocation() - Bot->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();
	const float Distance = FVector::Dist2D(Bot->GetActorLocation(), Target->GetActorLocation());
	const float LateralSign = CombatRandom.RandRange(0, 1) == 0 ? -1.0f : 1.0f;
	const float LateralDistance = CombatRandom.FRandRange(
		CombatMoveDistanceMin, FMath::Max(CombatMoveDistanceMin, CombatMoveDistanceMax));
	float RadialDistance = CombatRandom.FRandRange(-150.0f, 150.0f);
	if (Distance < PreferredCombatDistanceMin)
	{
		RadialDistance = -CombatRandom.FRandRange(150.0f, 350.0f);
	}
	else if (Distance > PreferredCombatDistanceMax)
	{
		RadialDistance = CombatRandom.FRandRange(150.0f, 350.0f);
	}
	const FVector DesiredLocation = Bot->GetActorLocation()
		+ Right * LateralSign * LateralDistance
		+ ToTarget * RadialDistance;

	FNavLocation ProjectedLocation;
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (Navigation && Navigation->ProjectPointToNavigation(DesiredLocation, ProjectedLocation))
	{
		MoveToLocation(ProjectedLocation.Location, 60.0f, true, true, true, true, nullptr, true);
	}
}

void AApecoxWanderAIController::UpdateAimFocus(
	const AApecoxBotCharacter* Bot, const AApecoxPlayerCharacter* Target)
{
	if (!Bot || !Target)
	{
		return;
	}
	SetFocalPoint(Target->GetPawnViewLocation() + CurrentAimOffset, EAIFocusPriority::Gameplay);
}

void AApecoxWanderAIController::BeginFireBurst(
	AApecoxBotCharacter* Bot, AApecoxPlayerCharacter* Target)
{
	if (!Bot || !Target || bFireBurstActive)
	{
		return;
	}

	const FVector ToTarget = (Target->GetPawnViewLocation() - Bot->GetPawnViewLocation()).GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();
	CurrentAimOffset = Right * CombatRandom.FRandRange(-AimErrorHorizontal, AimErrorHorizontal)
		+ FVector::UpVector * CombatRandom.FRandRange(-AimErrorVertical, AimErrorVertical);
	UpdateAimFocus(Bot, Target);
	bFireBurstActive = true;
	FireBurstTimeRemaining = CombatRandom.FRandRange(
		FireBurstDurationMin, FMath::Max(FireBurstDurationMin, FireBurstDurationMax));
	Bot->SetAIWeaponFireActive(true);
}

void AApecoxWanderAIController::EndFireBurst(AApecoxBotCharacter* Bot, bool bStartPause)
{
	if (Bot)
	{
		Bot->SetAIWeaponFireActive(false);
	}
	bFireBurstActive = false;
	FireBurstTimeRemaining = 0.0f;
	if (bStartPause)
	{
		FirePauseTimeRemaining = CombatRandom.FRandRange(
			FirePauseMin, FMath::Max(FirePauseMin, FirePauseMax));
	}
}

void AApecoxWanderAIController::StopCombat()
{
	if (AApecoxBotCharacter* Bot = Cast<AApecoxBotCharacter>(GetPawn()))
	{
		EndFireBurst(Bot, false);
	}
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	CombatTarget = nullptr;
	CurrentObjective = nullptr;
	CombatRepositionTimeRemaining = 0.0f;
	ReactionTimeRemaining = 0.0f;
	FirePauseTimeRemaining = 0.0f;
	LostSightTime = 0.0f;
	ObjectiveSearchTimeRemaining = 0.0f;
	ObjectiveLookTimeRemaining = 0.0f;
	bSearchingObjective = false;
	CurrentAimOffset = FVector::ZeroVector;
}
