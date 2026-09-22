// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxWeaponPickup.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

AApecoxWeaponPickup::AApecoxWeaponPickup()
{
	SetReplicates(true);
	PrimaryActorTick.bCanEverTick = false;

	// 根组件
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// 交互球体——仅 Query，不模拟物理
	// 小半径查询体，能被本地 Visibility Trace 命中即可；
	// 交互距离（InteractionDistance）用于服务器距离/视线校验，
	// 不要把交互距离直接当作球体半径
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionSphere->SetSphereRadius(40.0f);

	// 地面武器 Mesh
	PickupMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(SceneRoot);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AApecoxWeaponPickup::BeginPlay()
{
	Super::BeginPlay();
	RefreshPickupPresentation();
}

// ========================================================================
// 验证
// ========================================================================

bool AApecoxWeaponPickup::CanBePickedUpBy(const APawn* Pawn) const
{
	// 仅服务器可验证
	if (!HasAuthority())
	{
		return false;
	}

	if (!Pawn)
	{
		return false;
	}

	// 已经被 Claim 或已消耗
	if (bClaimed)
	{
		return false;
	}

	// 正在被销毁
	if (!IsValid(this) || IsActorBeingDestroyed())
	{
		return false;
	}

	// 需要有效武器定义
	if (!WeaponDefinition)
	{
		return false;
	}

	// 距离检测
	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation());
	if (DistSq > FMath::Square(InteractionDistance))
	{
		return false;
	}

	// 视线检测——使用 Pawn 视点位置作为起点，模拟真实玩家视角
	// 已忽略 Pawn 和 Pickup 自身，任何 BlockingHit 都表示被障碍阻挡
	if (GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Pawn);
		Params.AddIgnoredActor(this);

		const FVector TraceStart = Pawn->GetPawnViewLocation();
		const FVector TraceEnd = GetActorLocation();

		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd,
			ECC_Visibility, Params);

		if (Hit.bBlockingHit)
		{
			// 被障碍物阻挡
			return false;
		}
	}

	return true;
}

// ========================================================================
// 事务
// ========================================================================

bool AApecoxWeaponPickup::TryClaim()
{
	if (!HasAuthority())
	{
		return false;
	}

	if (bClaimed)
	{
		return false;
	}

	bClaimed = true;
	return true;
}

void AApecoxWeaponPickup::ReleaseClaim()
{
	if (HasAuthority())
	{
		bClaimed = false;
	}
}

void AApecoxWeaponPickup::Consume()
{
	if (!HasAuthority())
	{
		return;
	}

	// 客户端不销毁权威 Pickup——只有服务器可以
	Destroy();
}

// ========================================================================
// 复制与表现
// ========================================================================

void AApecoxWeaponPickup::OnRep_WeaponDefinition()
{
	RefreshPickupPresentation();
}

void AApecoxWeaponPickup::RefreshPickupPresentation()
{
	USkeletalMesh* MeshToUse = nullptr;

	if (WeaponDefinition && WeaponDefinition->PresentationDefinition)
	{
		const UApecoxWeaponPresentationDefinition* Pres = WeaponDefinition->PresentationDefinition;

		// WorldPickupMesh 优先，为空则回退到 ThirdPersonWeaponMesh
		MeshToUse = Pres->WorldPickupMesh;
		if (!MeshToUse)
		{
			MeshToUse = Pres->ThirdPersonWeaponMesh;
		}
	}

	if (PickupMesh)
	{
		PickupMesh->SetSkeletalMesh(MeshToUse);
	}
}

// ========================================================================
// 复制注册
// ========================================================================

void AApecoxWeaponPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApecoxWeaponPickup, WeaponDefinition);
}
