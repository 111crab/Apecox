// Copyright Apecox. All Rights Reserved.

#include "Character/ApecoxBotCharacter.h"

#include "AI/ApecoxWanderAIController.h"
#include "Game/ApecoxGameMode.h"
#include "Engine/World.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponInstance.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Inventory/ApecoxInventoryItemInstance.h"
#include "GameFramework/CharacterMovementComponent.h"

AApecoxBotCharacter::AApecoxBotCharacter()
{
	AIControllerClass = AApecoxWanderAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = true;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}
}

void AApecoxBotCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	TryEquipBotStartingWeapon();
}

void AApecoxBotCharacter::TryEquipBotStartingWeapon()
{
	if (!HasAuthority() || !StartingWeaponDefinition || !EquipmentComponent || EquipmentComponent->IsArmed())
	{
		return;
	}

	const TSubclassOf<UApecoxInventoryItemInstance> InstanceClass = StartingWeaponDefinition->InstanceClass;
	if (!InstanceClass || !InstanceClass->IsChildOf(UApecoxWeaponInstance::StaticClass()))
	{
		return;
	}

	UApecoxWeaponInstance* WeaponInstance = NewObject<UApecoxWeaponInstance>(this, InstanceClass);
	if (!WeaponInstance)
	{
		return;
	}

	WeaponInstance->Initialize(StartingWeaponDefinition);
	EquipmentComponent->EquipWeapon(EApecoxWeaponSlot::Primary, WeaponInstance);
}

void AApecoxBotCharacter::SetAIWeaponFireActive(bool bActive)
{
	if (!HasAuthority() || bAIWeaponFireActive == bActive)
	{
		return;
	}

	bAIWeaponFireActive = bActive;
	if (UApecoxAbilitySystemComponent* ASC = GetApecoxAbilitySystemComponent())
	{
		if (bActive)
		{
			ASC->AbilityInputTagPressed(ApecoxGameplayTags::InputTag_Weapon_Fire);
		}
		else
		{
			ASC->AbilityInputTagReleased(ApecoxGameplayTags::InputTag_Weapon_Fire);
		}
	}
}

bool AApecoxBotCharacter::TryStartAIWeaponReload()
{
	if (!HasAuthority())
	{
		return false;
	}

	SetAIWeaponFireActive(false);
	return TryStartWeaponReloadAuthority();
}

bool AApecoxBotCharacter::IsAIWeaponMagazineEmpty() const
{
	const UApecoxRangedWeaponInstance* Weapon = EquipmentComponent
		? Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()) : nullptr;
	return Weapon && Weapon->IsMagazineEmpty();
}

void AApecoxBotCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		BotSpawnTransform = GetActorTransform();
	}
}

void AApecoxBotCharacter::RequestRespawnFromGameMode(AController* RespawnController)
{
	if (AApecoxGameMode* GameMode = GetWorld()->GetAuthGameMode<AApecoxGameMode>())
	{
		GameMode->RequestBotRespawn(RespawnController, GetClass(), BotSpawnTransform);
	}
}
