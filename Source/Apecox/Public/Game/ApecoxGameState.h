// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ApecoxGameState.generated.h"

UENUM(BlueprintType)
enum class EApecoxMatchPhase : uint8
{
	WaitingToStart,
	InProgress,
	PostMatch
};

UENUM(BlueprintType)
enum class EApecoxMatchWinner : uint8
{
	None,
	Players,
	AI
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApecoxMatchStateChanged);

/**
 * AApecoxGameState
 * - 建立 Apecox 项目级 GameState 类型边界
 * - 复制 Score Attack 的公共比分、目标分、阶段和胜方
 * - UMG 只观察这些状态；只有 Authority 可以修改
 */
UCLASS()
class APECOX_API AApecoxGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AApecoxGameState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	EApecoxMatchPhase GetApecoxMatchPhase() const { return MatchPhase; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	EApecoxMatchWinner GetMatchWinner() const { return MatchWinner; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	int32 GetPlayerTeamScore() const { return PlayerTeamScore; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	int32 GetAITeamScore() const { return AITeamScore; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	int32 GetTargetScore() const { return TargetScore; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	bool IsMatchInProgress() const { return MatchPhase == EApecoxMatchPhase::InProgress; }

	/** UMG can bind one stable event instead of polling replicated fields every frame. */
	UPROPERTY(BlueprintAssignable, Category = "Apecox|Match")
	FApecoxMatchStateChanged OnMatchStateChanged;

	// Authority-only mutation surface used by GameMode.
	void StartScoreAttack(int32 InTargetScore);
	int32 AddPlayerTeamScore(int32 Delta = 1);
	int32 AddAITeamScore(int32 Delta = 1);
	void FinishMatch(EApecoxMatchWinner InWinner);

private:
	UFUNCTION()
	void OnRep_MatchState();

	void NotifyMatchStateChanged();

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	EApecoxMatchPhase MatchPhase = EApecoxMatchPhase::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	EApecoxMatchWinner MatchWinner = EApecoxMatchWinner::None;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 PlayerTeamScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 AITeamScore = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	int32 TargetScore = 15;
};
