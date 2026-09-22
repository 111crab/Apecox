// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Effects/ApecoxWeaponDamageEffect.h"

#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "GameplayTags/ApecoxGameplayTags.h"

UApecoxWeaponDamageEffect::UApecoxWeaponDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo& Modifier = Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = UApecoxVitalAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat DamageMagnitude;
	DamageMagnitude.DataTag = ApecoxGameplayTags::SetByCaller_Damage;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(DamageMagnitude);
}
