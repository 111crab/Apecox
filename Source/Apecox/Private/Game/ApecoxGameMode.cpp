// Copyright Apecox. All Rights Reserved.

#include "Game/ApecoxGameMode.h"
#include "Game/ApecoxGameState.h"
#include "Player/ApecoxPlayerController.h"
#include "Player/ApecoxPlayerState.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"

AApecoxGameMode::AApecoxGameMode()
{
	// 绑定 Apecox 自有类型，使 GameMode 在生成各角色时使用项目子类
	GameStateClass = AApecoxGameState::StaticClass();
	PlayerControllerClass = AApecoxPlayerController::StaticClass();
	PlayerStateClass = AApecoxPlayerState::StaticClass();
	DefaultPawnClass = AApecoxPlayerCharacter::StaticClass();
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

	// 防重入：同一 Controller 只加入一次 Pending 集合，阻止重复 Timer
	TWeakObjectPtr<AController> WeakController(Controller);
	if (PendingRespawnControllers.Contains(WeakController))
	{
		return;
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

void AApecoxGameMode::RestartPlayerAfterDelay(TWeakObjectPtr<AController> Controller)
{
	// 先移除 Pending 标记，无论后续验证是否通过
	PendingRespawnControllers.Remove(Controller);

	if (!HasAuthority())
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
