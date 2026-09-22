// Copyright Apecox. All Rights Reserved.

#include "Pickups/ApecoxCombatPickup.h"

#include "Character/ApecoxHealthComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/ApecoxPlayerState.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AApecoxCombatPickup::AApecoxCombatPickup()
{
	bReplicates = true;
	SetReplicateMovement(false);
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->InitSphereRadius(70.0f);
	Trigger->SetCollisionObjectType(ECC_WorldDynamic);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(Trigger);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetCanEverAffectNavigation(false);
	PickupMesh->SetRelativeScale3D(FVector(0.38f, 0.38f, 0.22f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaceholderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (PlaceholderMesh.Succeeded())
	{
		PickupMesh->SetStaticMesh(PlaceholderMesh.Object);
	}

	PickupLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
	PickupLabel->SetupAttachment(Trigger);
	PickupLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
	PickupLabel->SetHorizontalAlignment(EHTA_Center);
	PickupLabel->SetWorldSize(24.0f);
	PickupLabel->SetCastShadow(false);

	AccentLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AccentLight"));
	AccentLight->SetupAttachment(Trigger);
	AccentLight->SetRelativeLocation(FVector(0.0f, 0.0f, 25.0f));
	AccentLight->SetIntensity(1800.0f);
	AccentLight->SetAttenuationRadius(240.0f);
	AccentLight->SetCastShadows(false);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 55.0f, 0.0f);
}

void AApecoxCombatPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPresentation();
}

void AApecoxCombatPickup::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleOverlap);
	RefreshPresentation();
}

void AApecoxCombatPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AApecoxCombatPickup, bPickupAvailable);
}

void AApecoxCombatPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bPickupAvailable)
	{
		return;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(OtherActor);
	AApecoxPlayerState* PlayerState = Character ? Character->GetPlayerState<AApecoxPlayerState>() : nullptr;
	const UApecoxHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
	if (!Character || !PlayerState || !Health || Health->IsDeadOrDying()
		|| PlayerState->GetCombatTeam() != EApecoxCombatTeam::Players)
	{
		return;
	}

	const bool bConsumed = PickupKind == EApecoxCombatPickupKind::HealthPack
		? PlayerState->TryApplyHealthPickup(HealthRestoreAmount)
		: PlayerState->TryApplyShieldBattery(ShieldEvolutionPoints);
	if (!bConsumed)
	{
		return;
	}

	SetPickupAvailable(false);
	if (RespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			RespawnTimerHandle, this, &ThisClass::RespawnPickup, RespawnDelay, false);
	}
}

void AApecoxCombatPickup::SetPickupAvailable(bool bNewAvailable)
{
	if (!HasAuthority() || bPickupAvailable == bNewAvailable)
	{
		return;
	}
	bPickupAvailable = bNewAvailable;
	RefreshPresentation();
	ForceNetUpdate();
}

void AApecoxCombatPickup::RespawnPickup()
{
	SetPickupAvailable(true);
}

void AApecoxCombatPickup::OnRep_PickupAvailable()
{
	RefreshPresentation();
}

void AApecoxCombatPickup::RefreshPresentation()
{
	const bool bHealth = PickupKind == EApecoxCombatPickupKind::HealthPack;
	const FLinearColor VisualColor = bHealth ? HealthVisualColor : ShieldVisualColor;

	if (Trigger)
	{
		Trigger->SetCollisionEnabled(bPickupAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
	if (PickupMesh)
	{
		// SciFi_Props/M_Master 暴露这两个参数。组件会使用自己的动态材质实例，
		// 因此红色血包和蓝色护盾可以继续复用同一个原始 Material Instance。
		PickupMesh->SetVectorParameterValueOnMaterials(
			TEXT("Emissive_Color"), FVector(VisualColor.R, VisualColor.G, VisualColor.B));
		PickupMesh->SetScalarParameterValueOnMaterials(TEXT("Emissive_Power"), EmissivePower);
		PickupMesh->SetVisibility(bPickupAvailable, true);
	}
	if (PickupLabel)
	{
		PickupLabel->SetText(FText::FromString(bHealth ? TEXT("HEALTH") : TEXT("SHIELD +300")));
		PickupLabel->SetTextRenderColor(VisualColor.ToFColor(true));
		PickupLabel->SetVisibility(bPickupAvailable, true);
	}
	if (AccentLight)
	{
		AccentLight->SetLightColor(VisualColor);
		AccentLight->SetVisibility(bPickupAvailable, true);
	}
	if (RotatingMovement)
	{
		RotatingMovement->SetComponentTickEnabled(bPickupAvailable);
	}
}
