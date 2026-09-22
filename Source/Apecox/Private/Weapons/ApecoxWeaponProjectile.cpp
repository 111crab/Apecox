// Copyright Apecox. All Rights Reserved.
#include "Weapons/ApecoxWeaponProjectile.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxWeaponStateComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "Character/ApecoxBotCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable<int32> CVarApecoxProjectileDebug(
    TEXT("apecox.Projectile.Debug"), 0, TEXT("Draw authoritative projectile flight segments and impact points."));

// Playable score-attack defaults to lethal AI. The cvar remains available for non-lethal
// presentation/debug sessions without changing weapon or shield rules.
static TAutoConsoleVariable<int32> CVarApecoxAIDamageEnabled(
    TEXT("apecox.AI.DamageEnabled"), 1,
    TEXT("Whether projectiles fired by AApecoxBotCharacter apply their damage GameplayEffect (0/1)."));

AApecoxWeaponProjectile::AApecoxWeaponProjectile()
{
    bReplicates = true;
    SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(1.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetGenerateOverlapEvents(false);
    Collision->bReturnMaterialOnMove = true;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetCastShadow(false);
    Visual->SetReceivesDecals(false);
    Visual->SetCanEverAffectNavigation(false);
    // Gameplay projectile remains authoritative and replicated, but its high-speed rendering is
    // handled by the exact-path multicast tracer. Hiding this mesh avoids a delayed duplicate on clients.
    Visual->SetHiddenInGame(true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ProjectileMesh(
        TEXT("/Game/InfimaGames/ArtCore/Weapons/Guns/Meshes/SM_IG_Projectile_Bullet.SM_IG_Projectile_Bullet"));
    if (ProjectileMesh.Succeeded())
    {
        // 网格自身引用RAR的MI_IG_Bullet_Glow，不额外复制材质或Niagara系统。
        Visual->SetStaticMesh(ProjectileMesh.Object);
    }
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision);
    Movement->bAutoActivate = false;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = false;
    Movement->bForceSubStepping = true;
    Movement->MaxSimulationTimeStep = 1.0f / 120.0f;
    Movement->MaxSimulationIterations = 16;
    Movement->OnProjectileStop.AddDynamic(this, &ThisClass::OnProjectileStop);
}

void AApecoxWeaponProjectile::BeginPlay()
{
    Super::BeginPlay();
    if (!HasAuthority())
    {
        // No client collision or independently simulated damage; movement comes from Authority.
        Movement->Deactivate();
    }
}

void AApecoxWeaponProjectile::Launch(const FApecoxProjectileFireConfig& Config, const FVector& Velocity,
    const FGameplayEffectSpecHandle& InDamageSpec, UAbilitySystemComponent* InSourceASC,
    UApecoxWeaponStateComponent* InWeaponState, uint32 InShotId, const FHitResult& InitialObstruction)
{
    if (!HasAuthority() || bLaunched || bResolved) { return; }
    bLaunched = true;
    ShotId = InShotId;
    DamageSpec = InDamageSpec;
    SourceASC = InSourceASC;
    WeaponState = InWeaponState;
    PreviousLocation = GetActorLocation();
    Collision->SetSphereRadius(Config.CollisionRadius);
    // Use the configured query channel as this moving body's filter. Target response to
    // Weapon then controls blocking, just as it does for the aiming trace.
    Collision->SetCollisionObjectType(Config.TraceChannel);
    Collision->SetCollisionResponseToAllChannels(ECR_Block);
    // Bullets must not collide with other bullets using this same channel.
    Collision->SetCollisionResponseToChannel(Config.TraceChannel, ECR_Ignore);
    Collision->IgnoreActorWhenMoving(GetOwner(), true);
    Collision->IgnoreActorWhenMoving(GetInstigator(), true);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Movement->ProjectileGravityScale = Config.GravityScale;
    Movement->Velocity = Velocity;
    Movement->Activate();
    SetLifeSpan(Config.LifeSeconds);
    UE_LOG(LogTemp, Log, TEXT("[Apecox] Projectile launched ShotId=%u Speed=%.1f Gravity=%.2f"),
        ShotId, Velocity.Size(), Config.GravityScale);
    if (InitialObstruction.bBlockingHit)
    {
        // Origin path already intersects a wall: resolve this wall, never spawn behind it.
        OnProjectileStop(InitialObstruction);
    }
}

void AApecoxWeaponProjectile::OnProjectileStop(const FHitResult& Hit)
{
    if (!HasAuthority() || !bLaunched || bResolved) { return; }
    bResolved = true; // Set BEFORE GE/Cue callbacks, which may cause nested gameplay events.
    Movement->StopMovementImmediately();
    Movement->Deactivate();
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EApecoxShotConfirmation Result = EApecoxShotConfirmation::Miss;
    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Hit.GetActor());
    const bool bBotDamageSuppressed = IsValid(GetInstigator())
        && GetInstigator()->IsA<AApecoxBotCharacter>()
        && CVarApecoxAIDamageEnabled.GetValueOnGameThread() == 0;
    if (TargetASC && DamageSpec.IsValid() && !bBotDamageSuppressed)
    {
        const FGameplayAttribute Health = UApecoxVitalAttributeSet::GetHealthAttribute();
        const FGameplayAttribute Shield = UApecoxVitalAttributeSet::GetShieldAttribute();
        const bool bHasHealth = TargetASC->HasAttributeSetForAttribute(Health);
        const bool bHasShield = TargetASC->HasAttributeSetForAttribute(Shield);
        const float BeforeDurability = (bHasHealth ? TargetASC->GetNumericAttribute(Health) : 0.0f)
            + (bHasShield ? TargetASC->GetNumericAttribute(Shield) : 0.0f);
        DamageSpec.Data->GetContext().AddHitResult(Hit, true);
        TargetASC->ApplyGameplayEffectSpecToSelf(*DamageSpec.Data);
        const float AfterDurability = (bHasHealth ? TargetASC->GetNumericAttribute(Health) : 0.0f)
            + (bHasShield ? TargetASC->GetNumericAttribute(Shield) : 0.0f);
        if ((bHasHealth || bHasShield) && AfterDurability < BeforeDurability)
        {
            Result = EApecoxShotConfirmation::ConfirmedHit;
        }
    }
    if (UAbilitySystemComponent* ASC = SourceASC.Get())
    {
        FGameplayCueParameters Params;
        Params.EffectContext = ASC->MakeEffectContext();
        Params.EffectContext.AddHitResult(Hit);
        Params.EffectContext.AddInstigator(GetInstigator(), this);
        Params.Instigator = GetInstigator();
        Params.EffectCauser = this;
        Params.Location = Hit.ImpactPoint;
        Params.Normal = Hit.ImpactNormal;
        Params.PhysicalMaterial = Hit.PhysMaterial;
        ASC->ExecuteGameplayCue(ApecoxGameplayTags::GameplayCue_Weapon_Impact, Params);
    }
	if (WeaponState.IsValid())
	{
		WeaponState->SendProjectileResult(
			ShotId, Result,
			Result == EApecoxShotConfirmation::ConfirmedHit ? Hit.GetActor() : nullptr);
	}
    UE_LOG(LogTemp, Log, TEXT("[Apecox] Projectile impact ShotId=%u Target=%s Damaged=%s Location=%s"),
        ShotId, *GetNameSafe(Hit.GetActor()), Result == EApecoxShotConfirmation::ConfirmedHit ? TEXT("yes") : TEXT("no"), *Hit.ImpactPoint.ToString());
    if (CVarApecoxProjectileDebug.GetValueOnGameThread())
    {
        DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 12, FColor::Red, false, 3);
    }
    Destroy();
}

void AApecoxWeaponProjectile::LifeSpanExpired()
{
    if (HasAuthority() && bLaunched && !bResolved)
    {
        bResolved = true;
        if (WeaponState.IsValid()) { WeaponState->SendProjectileResult(ShotId, EApecoxShotConfirmation::Miss); }
        UE_LOG(LogTemp, Log, TEXT("[Apecox] Projectile expired ShotId=%u"), ShotId);
    }
    Super::LifeSpanExpired();
}

void AApecoxWeaponProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority() && bLaunched && !bResolved && CVarApecoxProjectileDebug.GetValueOnGameThread())
    {
        DrawDebugLine(GetWorld(), PreviousLocation, GetActorLocation(), FColor::Yellow, false, 3, 0, 1);
        DrawDebugPoint(GetWorld(), GetActorLocation(), 5, FColor::Yellow, false, 0.04f);
    }
    PreviousLocation = GetActorLocation();
}
