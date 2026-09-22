// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ApecoxWeaponDamageEffect.generated.h"

/**
 * Weapon damage is one stable native Instant GE. The weapon definition supplies the
 * signed magnitude through SetByCaller.Damage, so each weapon can tune damage without
 * duplicating or mutating GameplayEffect Blueprint assets.
 */
UCLASS()
class APECOX_API UApecoxWeaponDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UApecoxWeaponDamageEffect();
};
