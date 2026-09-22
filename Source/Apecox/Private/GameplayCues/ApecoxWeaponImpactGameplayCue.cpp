// Copyright Apecox. All Rights Reserved.

#include "GameplayCues/ApecoxWeaponImpactGameplayCue.h"

#include "Components/DecalComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UApecoxWeaponImpactGameplayCue::UApecoxWeaponImpactGameplayCue()
{
	// Leave GameplayCueTag at None in this native presentation base. The derived
	// GCN_Weapon_Impact Blueprint must serialize GameplayCue.Weapon.Impact itself;
	// GameplayCueManager discovers Blueprint notifies from that Asset Registry value.
	IsOverride = true;
}

bool UApecoxWeaponImpactGameplayCue::OnExecute_Implementation(
	AActor* MyTarget,
	const FGameplayCueParameters& Parameters) const
{
	const FHitResult* Hit = Parameters.EffectContext.GetHitResult();
	const FVector ImpactPoint = Hit && Hit->bBlockingHit
		? FVector(Hit->ImpactPoint)
		: FVector(Parameters.Location);
	const FVector ImpactNormal = Hit && Hit->bBlockingHit
		? Hit->ImpactNormal.GetSafeNormal()
		: FVector(Parameters.Normal).GetSafeNormal();
	const FRotator ImpactRotation = ImpactNormal.IsNearlyZero()
		? FRotator::ZeroRotator
		: ImpactNormal.Rotation();

	UE_LOG(LogTemp, Log,
		TEXT("[Apecox] Impact Cue executed Target=%s Location=%s VFX=%s Sound=%s Decals=%d"),
		*GetNameSafe(MyTarget),
		*ImpactPoint.ToString(),
		*GetNameSafe(ImpactSystem),
		*GetNameSafe(ImpactSound),
		ImpactDecalMaterials.Num());

	if (ImpactSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			MyTarget,
			ImpactSystem,
			ImpactPoint,
			ImpactRotation,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::AutoRelease,
			true);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(MyTarget, ImpactSound, ImpactPoint);
	}

	if (!ImpactDecalMaterials.IsEmpty())
	{
		const int32 MaterialIndex = FMath::RandRange(0, ImpactDecalMaterials.Num() - 1);
		UMaterialInterface* DecalMaterial = ImpactDecalMaterials[MaterialIndex];
		if (DecalMaterial)
		{
			FRotator DecalRotation = ImpactRotation;
			DecalRotation.Roll = FMath::FRandRange(0.0f, 360.0f);
			UDecalComponent* Decal = nullptr;
			if (Hit && Hit->GetComponent())
			{
				Decal = UGameplayStatics::SpawnDecalAttached(
					DecalMaterial,
					ImpactDecalSize,
					Hit->GetComponent(),
					Hit->BoneName,
					ImpactPoint,
					DecalRotation,
					EAttachLocation::KeepWorldPosition,
					DecalFadeStartDelay + DecalFadeDuration);
			}
			else
			{
				Decal = UGameplayStatics::SpawnDecalAtLocation(
					MyTarget,
					DecalMaterial,
					ImpactDecalSize,
					ImpactPoint,
					DecalRotation,
					DecalFadeStartDelay + DecalFadeDuration);
			}

			if (Decal)
			{
				Decal->SetFadeOut(DecalFadeStartDelay, DecalFadeDuration, false);
			}
		}
	}

	return ImpactSystem || ImpactSound || !ImpactDecalMaterials.IsEmpty();
}
