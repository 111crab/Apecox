// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/ApecoxAbilitySet.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/ApecoxGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AttributeSet.h"

// --- FApecoxAbilitySetGrantedHandles ---

void FApecoxAbilitySetGrantedHandles::RemoveFromAbilitySystem(UApecoxAbilitySystemComponent* ASC)
{
	// 修复 7：ASC 无效或非 Authority 时安全返回
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	for (const FActiveGameplayEffectHandle& Handle : ActiveGameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	for (UAttributeSet* AttrSet : GrantedAttributeSets)
	{
		if (AttrSet)
		{
			ASC->RemoveSpawnedAttribute(AttrSet);
		}
	}

	AbilitySpecHandles.Reset();
	ActiveGameplayEffectHandles.Reset();
	GrantedAttributeSets.Reset();
}

// --- UApecoxAbilitySet ---

void UApecoxAbilitySet::GrantToAbilitySystem(UApecoxAbilitySystemComponent* ASC,
	FApecoxAbilitySetGrantedHandles& OutGrantedHandles,
	UObject* SourceObject) const
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// --- 授予 Ability ---
	for (const FApecoxAbilitySetAbility& AbilityEntry : GrantedAbilities)
	{
		if (!AbilityEntry.Ability)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' has null Ability entry"), *GetName());
			continue;
		}

		// 修复 7：AbilityLevel < 1 时 ensure 并跳过
		if (AbilityEntry.AbilityLevel < 1)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' has Ability '%s' with invalid level %d"),
				*GetName(), *AbilityEntry.Ability->GetName(), AbilityEntry.AbilityLevel);
			continue;
		}

		// 修复 7：在 GiveAbility 前先将 InputTag 写入本地 Spec，
		// 确保 OnGive 和初次复制都能看到完整的 Spec
		FGameplayAbilitySpec Spec(AbilityEntry.Ability, AbilityEntry.AbilityLevel, INDEX_NONE, SourceObject);
		if (AbilityEntry.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityEntry.InputTag);
		}

		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
		if (Handle.IsValid())
		{
			OutGrantedHandles.AbilitySpecHandles.Add(Handle);
		}
	}

	// --- 授予 GameplayEffect ---
	for (const FApecoxAbilitySetGameplayEffect& EffectEntry : GrantedGameplayEffects)
	{
		if (!EffectEntry.GameplayEffect)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' has null GameplayEffect entry"), *GetName());
			continue;
		}

		// 修复 7：EffectLevel <= 0 时 ensure 并跳过
		if (EffectEntry.EffectLevel <= 0.0f)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' has GE '%s' with invalid level %f"),
				*GetName(), *EffectEntry.GameplayEffect->GetName(), EffectEntry.EffectLevel);
			continue;
		}

		UGameplayEffect* EffectCDO = EffectEntry.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		if (EffectCDO->DurationPolicy == EGameplayEffectDurationType::Instant)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' contains Instant GE '%s'."),
				*GetName(), *EffectEntry.GameplayEffect->GetName());
			continue;
		}

		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		if (SourceObject)
		{
			EffectContext.AddSourceObject(SourceObject);
		}

		FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectToSelf(
			EffectCDO, EffectEntry.EffectLevel, EffectContext);
		if (ActiveGEHandle.IsValid())
		{
			OutGrantedHandles.ActiveGameplayEffectHandles.Add(ActiveGEHandle);
		}
	}

	// --- 授予 AttributeSet ---
	for (const FApecoxAbilitySetAttributeSet& AttrEntry : GrantedAttributeSets)
	{
		if (!AttrEntry.AttributeSet)
		{
			ensureMsgf(false, TEXT("[Apecox] AbilitySet '%s' has null AttributeSet entry"), *GetName());
			continue;
		}

		UObject* Outer = ASC->GetOwner();
		UAttributeSet* AttrSet = NewObject<UAttributeSet>(Outer ? Outer : ASC, AttrEntry.AttributeSet);
		ASC->AddAttributeSetSubobject(AttrSet);
		OutGrantedHandles.GrantedAttributeSets.Add(AttrSet);
	}
}
