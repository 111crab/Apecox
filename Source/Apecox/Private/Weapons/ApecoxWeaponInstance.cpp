// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"

const UApecoxWeaponDefinition* UApecoxWeaponInstance::GetWeaponDefinition() const
{
	return Cast<UApecoxWeaponDefinition>(GetItemDefinition());
}
