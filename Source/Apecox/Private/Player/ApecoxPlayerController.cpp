// Copyright Apecox. All Rights Reserved.

#include "Player/ApecoxPlayerController.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"

void AApecoxPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	// 从当前 PlayerState 获取 ASC 并调度输入处理
	if (AApecoxPlayerState* ApecoxPS = GetPlayerState<AApecoxPlayerState>())
	{
		if (UApecoxAbilitySystemComponent* ASC = ApecoxPS->GetApecoxAbilitySystemComponent())
		{
			ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
		}
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}
