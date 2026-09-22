// Copyright Apecox. All Rights Reserved.

#include "Player/ApecoxPlayerController.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Camera/ApecoxPlayerCameraManager.h"
#include "Inventory/ApecoxInventoryComponent.h"
#include "Weapons/ApecoxWeaponStateComponent.h"

AApecoxPlayerController::AApecoxPlayerController()
{
	PlayerCameraManagerClass = AApecoxPlayerCameraManager::StaticClass();

	// 私有库存——跨 Pawn 保留（本轮死亡时由 GameMode 显式清空）
	InventoryComponent = CreateDefaultSubobject<UApecoxInventoryComponent>(TEXT("InventoryComponent"));

	// 武器状态组件——射击确认与命中反馈，默认复制
	WeaponStateComponent = CreateDefaultSubobject<UApecoxWeaponStateComponent>(TEXT("WeaponStateComponent"));
}

// ========================================================================
// 鼠标观察（本地表现）
// ========================================================================

void AApecoxPlayerController::AddMouseLookInput(const FVector2D& LookInput,
	const FVector2D& ContextSensitivityMultiplier)
{
	// 只处理本地 Controller——非本地不操作 ControlRotation
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 应用灵敏度统一缩放 X/Y，不乘 DeltaTime（鼠标增量已是每帧量）
	const FVector2D SafeContextMultiplier(
		FMath::Max(0.0, static_cast<double>(ContextSensitivityMultiplier.X)),
		FMath::Max(0.0, static_cast<double>(ContextSensitivityMultiplier.Y)));
	const FVector2D ScaledInput = LookInput * SafeContextMultiplier * MouseLookSensitivity;

	if (ScaledInput.X != 0.0f)
	{
		AddYawInput(ScaledInput.X);
	}

	if (ScaledInput.Y != 0.0f)
	{
		AddPitchInput(ScaledInput.Y);
	}
}

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
