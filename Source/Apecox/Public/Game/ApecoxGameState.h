// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ApecoxGameState.generated.h"

/**
 * AApecoxGameState
 * - 建立 Apecox 项目级 GameState 类型边界
 * - 本批不添加复制字段、比赛阶段、计分等公共状态
 */
UCLASS()
class APECOX_API AApecoxGameState : public AGameStateBase
{
	GENERATED_BODY()
};
