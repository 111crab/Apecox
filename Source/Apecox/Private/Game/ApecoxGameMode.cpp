// Copyright Apecox. All Rights Reserved.

#include "Game/ApecoxGameMode.h"
#include "Game/ApecoxGameState.h"
#include "Player/ApecoxPlayerController.h"
#include "Player/ApecoxPlayerState.h"
#include "Character/ApecoxPlayerCharacter.h"

AApecoxGameMode::AApecoxGameMode()
{
	// 绑定 Apecox 自有类型，使 GameMode 在生成各角色时使用项目子类
	GameStateClass = AApecoxGameState::StaticClass();
	PlayerControllerClass = AApecoxPlayerController::StaticClass();
	PlayerStateClass = AApecoxPlayerState::StaticClass();
	DefaultPawnClass = AApecoxPlayerCharacter::StaticClass();
}
