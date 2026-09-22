// Copyright Apecox. All Rights Reserved.
#include "Weapons/ApecoxProjectileTracer.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AApecoxProjectileTracer::AApecoxProjectileTracer()
{
	bReplicates = false;
	SetActorEnableCollision(false);
	PrimaryActorTick.bCanEverTick = true;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	SetRootComponent(Visual);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetGenerateOverlapEvents(false);
	Visual->SetCastShadow(false);
	Visual->SetReceivesDecals(false);
	Visual->SetCanEverAffectNavigation(false);
	Visual->SetRelativeScale3D(TracerVisualScale);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TracerMesh(
		TEXT("/Game/InfimaGames/ArtCore/Weapons/Guns/Meshes/SM_IG_Projectile_Bullet.SM_IG_Projectile_Bullet"));
	if (TracerMesh.Succeeded())
	{
		Visual->SetStaticMesh(TracerMesh.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Visual);
	Movement->bAutoActivate = false;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->InitialSpeed = 0.0f;
	Movement->MaxSpeed = 0.0f;
}

void AApecoxProjectileTracer::InitializeTracer(
	const FVector& Direction, float Speed, float GravityScale, float MaxDistance)
{
	const FVector SafeDirection = Direction.GetSafeNormal();
	if (SafeDirection.IsNearlyZero() || !FMath::IsFinite(Speed) || Speed <= 0.0f
		|| !FMath::IsFinite(GravityScale) || GravityScale < 0.0f
		|| !FMath::IsFinite(MaxDistance) || MaxDistance <= 0.0f)
	{
		Destroy();
		return;
	}

	SetActorRotation(SafeDirection.Rotation());
	TravelLimit = MaxDistance;
	DistanceTravelled = 0.0f;
	PreviousLocation = GetActorLocation();
	Movement->ProjectileGravityScale = GravityScale;
	Movement->Velocity = SafeDirection * Speed;
	Movement->Activate(true);

	const float RangeLife = MaxDistance / Speed;
	SetLifeSpan(FMath::Clamp(RangeLife + 0.03f, 0.03f, MaximumLifeSeconds));

	if (UMaterialInstanceDynamic* Material = Visual->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetScalarParameterValue(TEXT("Intensity Emissive"), EmissiveIntensity);
	}
}

void AApecoxProjectileTracer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector CurrentLocation = GetActorLocation();
	DistanceTravelled += FVector::Distance(PreviousLocation, CurrentLocation);
	PreviousLocation = CurrentLocation;
	if (TravelLimit > 0.0f && DistanceTravelled >= TravelLimit)
	{
		Destroy();
	}
}
