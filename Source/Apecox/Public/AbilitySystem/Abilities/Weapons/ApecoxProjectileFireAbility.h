// Copyright Apecox. All Rights Reserved.
#pragma once
#include "AbilitySystem/Abilities/Weapons/ApecoxRangedFireAbility.h"
#include "ApecoxProjectileFireAbility.generated.h"

UCLASS()
class APECOX_API UApecoxProjectileFireAbility : public UApecoxRangedFireAbility
{
    GENERATED_BODY()
protected:
    virtual bool IsFireConfigValid(const UApecoxRangedWeaponInstance* Weapon) const override;
    virtual void ExecuteValidatedShot(const FApecoxRangedShotTargetData& ShotData, UApecoxRangedWeaponInstance* Weapon) override;
};
