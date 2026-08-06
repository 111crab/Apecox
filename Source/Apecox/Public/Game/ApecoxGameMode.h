// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ApecoxGameMode.generated.h"

/**
 * AApecoxGameMode
 * - 建立 Apecox 项目自有 Gameplay Framework 类型边界
 * - 在构造函数中指定 GameState、PlayerController、PlayerState 和默认 Pawn 类型
 * - 本批不实现胜利、计分、死亡或复活逻辑
 */
UCLASS()
class APECOX_API AApecoxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AApecoxGameMode();
};
