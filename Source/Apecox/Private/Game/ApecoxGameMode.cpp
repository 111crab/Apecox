// Copyright Apecox. All Rights Reserved.

#include "Game/ApecoxGameMode.h"
#include "Game/ApecoxGameState.h"
#include "Player/ApecoxPlayerController.h"
#include "Player/ApecoxPlayerState.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Inventory/ApecoxInventoryComponent.h"
#include "UI/ApecoxHUD.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/ApecoxBotCharacter.h"

namespace
{
	AApecoxPlayerState* ResolveApecoxPlayerState(AActor* Actor)
	{
		if (AApecoxPlayerState* DirectPlayerState = Cast<AApecoxPlayerState>(Actor))
		{
			return DirectPlayerState;
		}
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetPlayerState<AApecoxPlayerState>();
		}
		if (const AController* Controller = Cast<AController>(Actor))
		{
			return Controller->GetPlayerState<AApecoxPlayerState>();
		}
		return nullptr;
	}
}

AApecoxGameMode::AApecoxGameMode()
{
	// 绑定 Apecox 自有类型，使 GameMode 在生成各角色时使用项目子类
	GameStateClass = AApecoxGameState::StaticClass();
	PlayerControllerClass = AApecoxPlayerController::StaticClass();
	PlayerStateClass = AApecoxPlayerState::StaticClass();
	DefaultPawnClass = AApecoxPlayerCharacter::StaticClass();
	HUDClass = AApecoxHUD::StaticClass();
}

void AApecoxGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (AApecoxGameState* ApecoxGameState = GetGameState<AApecoxGameState>())
		{
			ApecoxGameState->StartScoreAttack(TargetScore);
		}
	}
}

bool AApecoxGameMode::IsScoreAttackInProgress() const
{
	const AApecoxGameState* ApecoxGameState = GetGameState<AApecoxGameState>();
	return ApecoxGameState && ApecoxGameState->IsMatchInProgress();
}

void AApecoxGameMode::HandleCombatantKilled(AActor* Victim, AActor* Killer)
{
	if (!HasAuthority() || !IsValid(Victim) || !IsScoreAttackInProgress())
	{
		return;
	}

	AApecoxPlayerState* VictimPlayerState = ResolveApecoxPlayerState(Victim);
	AApecoxPlayerState* KillerPlayerState = ResolveApecoxPlayerState(Killer);
	if (VictimPlayerState)
	{
		VictimPlayerState->AddDeath();
	}

	// Self damage and environment deaths count as deaths but do not award team score.
	if (!IsValid(Killer) || Killer == Victim || KillerPlayerState == VictimPlayerState)
	{
		return;
	}

	// PlayerState is the stable team truth across Pawn death and future Controller/Effect
	// instigator changes. The actor-class fallback keeps map-placed bots safe during the
	// very short window before their AI PlayerState has initialized.
	const bool bVictimIsBot = VictimPlayerState
		? VictimPlayerState->GetCombatTeam() == EApecoxCombatTeam::AI
		: Victim->IsA<AApecoxBotCharacter>();
	const bool bKillerIsBot = KillerPlayerState
		? KillerPlayerState->GetCombatTeam() == EApecoxCombatTeam::AI
		: Killer->IsA<AApecoxBotCharacter>();
	AApecoxGameState* ApecoxGameState = GetGameState<AApecoxGameState>();
	if (!ApecoxGameState)
	{
		return;
	}

	if (bVictimIsBot && !bKillerIsBot)
	{
		if (KillerPlayerState)
		{
			KillerPlayerState->AddKill();
		}
		if (ApecoxGameState->AddPlayerTeamScore() >= ApecoxGameState->GetTargetScore())
		{
			ApecoxGameState->FinishMatch(EApecoxMatchWinner::Players);
		}
	}
	else if (!bVictimIsBot && bKillerIsBot)
	{
		if (KillerPlayerState)
		{
			KillerPlayerState->AddKill();
		}
		if (ApecoxGameState->AddAITeamScore() >= ApecoxGameState->GetTargetScore())
		{
			ApecoxGameState->FinishMatch(EApecoxMatchWinner::AI);
		}
	}
}

void AApecoxGameMode::RequestPlayerRespawn(APlayerController* Controller)
{
	// 仅在服务器处理重生请求
	if (!HasAuthority())
	{
		return;
	}

	if (!Controller)
	{
		return;
	}

	if (!IsScoreAttackInProgress())
	{
		return;
	}

	// 防重入：同一 Controller 只加入一次 Pending 集合，阻止重复 Timer
	TWeakObjectPtr<AController> WeakController(Controller);
	if (PendingRespawnControllers.Contains(WeakController))
	{
		return;
	}

	// 清理死亡玩家的私有库存——当前规则是死亡后清空、空手重生
	// 必须在安排 Timer 前清理，确保 Equipment 先卸下再清空 Controller 库存
	// GameMode 只决定死亡库存策略，不操作 WeaponInstance 内部字段或 ASC
	if (AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(Controller))
	{
		if (UApecoxInventoryComponent* Inventory = PC->GetInventoryComponent())
		{
			Inventory->ClearInventoryForDeath();
		}
	}

	PendingRespawnControllers.Add(WeakController);

	// 每个 Controller 创建独立的延迟 Timer——晚死亡的玩家不会共享早死亡玩家的倒计时
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
		this,
		&AApecoxGameMode::RestartPlayerAfterDelay,
		WeakController);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		RespawnDelegate,
		RespawnDelaySeconds,
		false // 不循环
	);
}

void AApecoxGameMode::RequestBotRespawn(AController* Controller, TSubclassOf<APawn> BotPawnClass,
	const FTransform& SpawnTransform)
{
	if (!HasAuthority() || !IsValid(Controller) || !BotPawnClass)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(Controller))
	{
		AIController->StopMovement();
	}

	if (!IsScoreAttackInProgress())
	{
		Controller->Destroy();
		return;
	}

	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
		this,
		&AApecoxGameMode::RespawnBotAfterDelay,
		BotPawnClass,
		SpawnTransform);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		RespawnDelegate,
		RespawnDelaySeconds,
		false);

	// AI 不需要保留玩家输入、库存等 Controller 状态。销毁旧 Controller 可同时清理旧 PlayerState/ASC，
	// 新 Pawn 的 AutoPossessAI 会创建全新的 Controller、PlayerState 和满生命 ASC。
	Controller->Destroy();
}

void AApecoxGameMode::RespawnBotAfterDelay(TSubclassOf<APawn> BotPawnClass, FTransform SpawnTransform)
{
	if (!HasAuthority() || !BotPawnClass || !GetWorld() || !IsScoreAttackInProgress())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<APawn>(BotPawnClass, SpawnTransform, SpawnParameters);
}

void AApecoxGameMode::RestartPlayerAfterDelay(TWeakObjectPtr<AController> Controller)
{
	// 先移除 Pending 标记，无论后续验证是否通过
	PendingRespawnControllers.Remove(Controller);

	if (!HasAuthority())
	{
		return;
	}

	if (!IsScoreAttackInProgress())
	{
		return;
	}

	AController* Ctrl = Controller.Get();
	if (!IsValid(Ctrl))
	{
		return;
	}

	// 到期时 Controller 必须没有 Pawn——Character 的 DestroyDueToDeath
	// 已通过 DetachFromControllerPendingDestroy 解除了占有
	if (Ctrl->GetPawn() != nullptr)
	{
		ensureMsgf(false,
			TEXT("[Apecox] RestartPlayerAfterDelay: Controller '%s' still has Pawn '%s' "
				"after respawn delay. This is a lifecycle error."),
			*GetNameSafe(Ctrl),
			*GetNameSafe(Ctrl->GetPawn()));
		return;
	}

	// 调用引擎 RestartPlayer：FindPlayerStart + SpawnDefaultPawn + Possess
	// 新 Pawn 通过 PossessedBy → InitializeAbilitySystem 绑定到原 PlayerState/ASC
	RestartPlayer(Ctrl);
}
