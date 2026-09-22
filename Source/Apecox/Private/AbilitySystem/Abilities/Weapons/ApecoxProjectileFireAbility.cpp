// Copyright Apecox. All Rights Reserved.
#include "AbilitySystem/Abilities/Weapons/ApecoxProjectileFireAbility.h"
#include "AbilitySystem/TargetData/ApecoxRangedShotTargetData.h"
#include "AbilitySystem/Effects/ApecoxWeaponDamageEffect.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "Weapons/ApecoxWeaponProjectile.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

bool UApecoxProjectileFireAbility::IsFireConfigValid(const UApecoxRangedWeaponInstance* Weapon) const
{
    const auto* Config = Weapon ? Weapon->GetProjectileFireConfig() : nullptr;
    return Config && Config->IsValidProjectileConfig();
}

void UApecoxProjectileFireAbility::ExecuteValidatedShot(const FApecoxRangedShotTargetData& ShotData,
    UApecoxRangedWeaponInstance* Weapon)
{
    const FApecoxProjectileFireConfig& Config = *Weapon->GetProjectileFireConfig();
    AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
    APawn* Pawn = Cast<APawn>(Avatar);
    UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
    const FVector View = Pawn ? Pawn->GetPawnViewLocation() : Avatar->GetActorLocation();
    const FRotator Aim = Pawn ? Pawn->GetBaseAimRotation() : Avatar->GetActorRotation();
    const FVector Direction = ShotData.AimDirection.GetSafeNormal();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ApecoxProjectileAim), false, Avatar);
    Query.bReturnPhysicalMaterial = true;
    FHitResult SightHit;
    const FVector SightEnd = View + Direction * Config.MaxRange;
    GetWorld()->LineTraceSingleByChannel(SightHit, View, SightEnd, Config.TraceChannel, Query);
    const FVector Intent = SightHit.bBlockingHit ? SightHit.ImpactPoint : SightEnd;
    FVector Origin = View + Aim.RotateVector(Config.GameplayFireOriginOffset);
    FHitResult Obstruction;
    GetWorld()->SweepSingleByChannel(Obstruction, View, Origin, FQuat::Identity, Config.TraceChannel,
        FCollisionShape::MakeSphere(Config.CollisionRadius), Query);
    if (Obstruction.bBlockingHit) { Origin = Obstruction.Location; }

    const FVector FireDirection = (Intent - Origin).GetSafeNormal(SMALL_NUMBER, Direction);
    const FTransform SpawnTransform(FireDirection.Rotation(), Origin);
    AApecoxWeaponProjectile* Projectile = GetWorld()->SpawnActorDeferred<AApecoxWeaponProjectile>(
        Config.ProjectileClass, SpawnTransform, Avatar, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Projectile)
    {
        SendShotConfirmation(ShotData.ShotId, EApecoxShotConfirmation::Rejected);
        return;
    }
    // Allocate first, commit second, then enable collision. A failed spawn consumes no round;
    // BeginPlay/contact cannot deal damage before the shot transaction has committed.
	if (!Weapon->CommitServerShot(ShotData.ShotId, GetWorld()->GetTimeSeconds(),
		ShotData.BurstId, ShotData.BurstShotIndex))
    {
        Projectile->Destroy();
        SendShotConfirmation(ShotData.ShotId, EApecoxShotConfirmation::Rejected);
        return;
    }
	auto Context = ASC->MakeEffectContext();
	Context.AddInstigator(Avatar, Projectile);
	Context.AddSourceObject(Weapon);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		UApecoxWeaponDamageEffect::StaticClass(), 1, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(ApecoxGameplayTags::SetByCaller_Damage, -Config.BaseDamage);
	}
	Projectile->FinishSpawning(SpawnTransform);
	ExecuteWeaponFireCue(CurrentActorInfo, Weapon);
	SendShotConfirmation(ShotData.ShotId, EApecoxShotConfirmation::Launched);
	const float ProjectileSpeed = FMath::FRandRange(Config.MinSpeed, Config.MaxSpeed);
	Projectile->Launch(Config, FireDirection * ProjectileSpeed,
		Spec, ASC, GetWeaponStateComponent(CurrentActorInfo), ShotData.ShotId, Obstruction);
	if (const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(Avatar))
	{
		if (UApecoxEquipmentComponent* Equipment = Character->GetEquipmentComponent())
		{
			const float TracerDistance = Obstruction.bBlockingHit
				? FVector::Distance(Origin, Obstruction.ImpactPoint)
				: FVector::Distance(Origin, Intent);
			Equipment->BroadcastProjectileTracer(
				Origin, FireDirection, ProjectileSpeed, Config.GravityScale, TracerDistance);
		}
	}
}
