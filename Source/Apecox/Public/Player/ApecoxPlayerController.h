// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ApecoxPlayerController.generated.h"

/**
 * AApecoxPlayerController
 * - 建立 Apecox 项目级 PlayerController 类型边界
 * - 本批不添加输入绑定、UI、库存或 RPC
 */
UCLASS()
class APECOX_API AApecoxPlayerController : public APlayerController
{
	GENERATED_BODY()
};
