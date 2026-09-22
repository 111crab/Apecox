// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ApecoxAnimationTypes.generated.h"

/**
 * EApecoxCharacterAnimationFamily
 * - 表现层有限选择：装备后角色与 FP Arms 应使用的动画族。
 * - 这是纯表现选择，不是 GameplayTag——动画族数量有限且稳定，不进入玩法逻辑，
 *   因此用枚举即可，避免引入状态 Tag 语义。
 * - 不提前增加尚未接入资产的 Pistol、Shotgun、Bow、Launcher 等占位值。
 */
UENUM(BlueprintType)
enum class EApecoxCharacterAnimationFamily : uint8
{
	/** 空手——未装备任何武器的默认动画族（安全中性值） */
	Unarmed UMETA(DisplayName = "空手"),

	/** 步枪——装备步枪后角色与 FP Arms 使用的动画族 */
	Rifle UMETA(DisplayName = "步枪")
};
