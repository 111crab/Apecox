// Copyright Apecox. All Rights Reserved.

#include "Game/ApecoxGameState.h"

#include "Net/UnrealNetwork.h"

AApecoxGameState::AApecoxGameState()
{
	bReplicates = true;
}

void AApecoxGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApecoxGameState, MatchPhase);
	DOREPLIFETIME(AApecoxGameState, MatchWinner);
	DOREPLIFETIME(AApecoxGameState, PlayerTeamScore);
	DOREPLIFETIME(AApecoxGameState, AITeamScore);
	DOREPLIFETIME(AApecoxGameState, TargetScore);
}

void AApecoxGameState::StartScoreAttack(int32 InTargetScore)
{
	if (!HasAuthority())
	{
		return;
	}

	TargetScore = FMath::Max(1, InTargetScore);
	PlayerTeamScore = 0;
	AITeamScore = 0;
	MatchWinner = EApecoxMatchWinner::None;
	MatchPhase = EApecoxMatchPhase::InProgress;
	ForceNetUpdate();
	NotifyMatchStateChanged();
}

int32 AApecoxGameState::AddPlayerTeamScore(int32 Delta)
{
	if (!HasAuthority() || !IsMatchInProgress())
	{
		return PlayerTeamScore;
	}

	PlayerTeamScore = FMath::Max(0, PlayerTeamScore + Delta);
	ForceNetUpdate();
	NotifyMatchStateChanged();
	return PlayerTeamScore;
}

int32 AApecoxGameState::AddAITeamScore(int32 Delta)
{
	if (!HasAuthority() || !IsMatchInProgress())
	{
		return AITeamScore;
	}

	AITeamScore = FMath::Max(0, AITeamScore + Delta);
	ForceNetUpdate();
	NotifyMatchStateChanged();
	return AITeamScore;
}

void AApecoxGameState::FinishMatch(EApecoxMatchWinner InWinner)
{
	if (!HasAuthority() || !IsMatchInProgress() || InWinner == EApecoxMatchWinner::None)
	{
		return;
	}

	MatchWinner = InWinner;
	MatchPhase = EApecoxMatchPhase::PostMatch;
	ForceNetUpdate();
	NotifyMatchStateChanged();
}

void AApecoxGameState::OnRep_MatchState()
{
	NotifyMatchStateChanged();
}

void AApecoxGameState::NotifyMatchStateChanged()
{
	OnMatchStateChanged.Broadcast();
}
