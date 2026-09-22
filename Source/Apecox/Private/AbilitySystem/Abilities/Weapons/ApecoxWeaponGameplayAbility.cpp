// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Weapons/ApecoxWeaponInstance.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "GameplayAbilitySpec.h"

UApecoxWeaponGameplayAbility::UApecoxWeaponGameplayAbility()
{
	// 武器 GA 默认 OnInputTriggered——连续射击子类覆盖为 WhileInputActive
	ActivationPolicy = EApecoxAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EApecoxAbilityActivationGroup::Independent;
}

// ========================================================================
// 来源武器验证
// ========================================================================

const UApecoxWeaponInstance* UApecoxWeaponGameplayAbility::GetWeaponInstanceFromSpec(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return nullptr;
	}

	// 使用传入 Handle 通过 ASC 查找 Spec——禁止激活前依赖 GetCurrentAbilitySpec()
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		return nullptr;
	}

	// SourceObject 是装备时由 UApecoxAbilitySet::GrantToAbilitySystem 传入的 WeaponInstance
	return Cast<UApecoxWeaponInstance>(Spec->SourceObject.Get());
}

const UApecoxRangedWeaponInstance* UApecoxWeaponGameplayAbility::GetRangedWeaponInstanceFromSpec(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Cast<UApecoxRangedWeaponInstance>(GetWeaponInstanceFromSpec(Handle, ActorInfo));
}

bool UApecoxWeaponGameplayAbility::IsSourceWeaponCurrentlyEquipped(
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return false;
	}

	const UApecoxWeaponInstance* SourceWeapon = GetWeaponInstanceFromSpec(Handle, ActorInfo);
	if (!SourceWeapon)
	{
		return false;
	}

	// 从 Avatar 获取 EquipmentComponent 检查当前装备
	const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	const UApecoxEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	if (!Equipment)
	{
		return false;
	}

	// SourceObject 必须正是当前装备的实例——武器已卸下或切换到其他武器时拒绝
	return Equipment->GetCurrentWeaponInstance() == SourceWeapon;
}

// ========================================================================
// CanActivateAbility
// ========================================================================

bool UApecoxWeaponGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 先执行父类检查（ActivationBlockedTags、并发组等）
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 验证 SourceObject：
	// 1. 不是空的
	// 2. 是有效的 WeaponInstance
	// 3. 与当前装备的实例一致（武器未被卸下或切换）
	if (!IsSourceWeaponCurrentlyEquipped(Handle, ActorInfo))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] WeaponGameplayAbility::CanActivateAbility: Source weapon not "
				"currently equipped — activation rejected."));
		return false;
	}

	return true;
}
