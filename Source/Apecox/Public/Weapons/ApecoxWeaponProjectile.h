// Copyright Apecox. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ApecoxWeaponProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UApecoxWeaponStateComponent;
struct FApecoxProjectileFireConfig;

/** Authority owns flight and damage. Proxies render replicated movement only.
 * The damage spec outlives the fire ability and equipped weapon state.
 */
UCLASS(Blueprintable)
class APECOX_API AApecoxWeaponProjectile : public AActor
{
    GENERATED_BODY()
public:
    AApecoxWeaponProjectile();
    void Launch(const FApecoxProjectileFireConfig& Config, const FVector& Velocity,
        const FGameplayEffectSpecHandle& InDamageSpec, UAbilitySystemComponent* InSourceASC,
        UApecoxWeaponStateComponent* InWeaponState, uint32 InShotId, const FHitResult& InitialObstruction);
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
    TObjectPtr<USphereComponent> Collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
    TObjectPtr<UStaticMeshComponent> Visual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
    TObjectPtr<UProjectileMovementComponent> Movement;

protected:
    virtual void BeginPlay() override;
    virtual void LifeSpanExpired() override;
    UFUNCTION()
    void OnProjectileStop(const FHitResult& Hit);

private:
    UPROPERTY()
    FGameplayEffectSpecHandle DamageSpec;
    TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
    TWeakObjectPtr<UApecoxWeaponStateComponent> WeaponState;
    uint32 ShotId = 0;
    bool bLaunched = false;
    bool bResolved = false;
    FVector PreviousLocation = FVector::ZeroVector;
};
