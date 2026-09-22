// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxWeaponPresentationActor.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Character/ApecoxHealthComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"

namespace ApecoxWeaponPresentation
{
	static const FName LoadedMagazineComponentName(TEXT("MagazineDefault"));
	static const FName ReserveMagazineSocketName(TEXT("SOCKET_Magazine_Reserve"));
	static const FName DefaultLaserSocketName(TEXT("SOCKET_Laser"));
}

AApecoxWeaponPresentationActor::AApecoxWeaponPresentationActor()
{
	// 纯表现 Actor：不开 Tick、不复制、无碰撞。
	// 它只承载本地视觉层，不参与网络同步、物理或拾取判定。
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetReplicates(false);
	// 显式关闭 Actor 级碰撞：纯表现对象从 Actor 层面即不参与世界碰撞/Overlap，
	// 即使 Blueprint 子类后续新增组件，生成前也先被隔离。
	SetActorEnableCollision(false);

	// Prefab 根节点——完整枪械相对 Arms 的统一调整基准
	PresentationRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PresentationRoot"));
	SetRootComponent(PresentationRoot);

	// 枪体 SkeletalMesh——枪械机械动作 Montage 的播放目标
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(PresentationRoot);
	// 武器表现永远不参与世界物理或 Overlap 判定
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	// 显式枪口锚点——FP 枪口 VFX/SFX 的附着位置，不依赖供应商 Socket 名
	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(WeaponMesh);

	// 显式左手握持锚点——由具体武器表现 Blueprint 放到护木的实际握持位置
	LeftHandGripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandGripPoint"));
	LeftHandGripPoint->SetupAttachment(WeaponMesh);

	// RAR 的手持弹匣不是附着到手骨骼，而是附着到武器骨架上的动画 Socket。
	// Weapon Reload Sequence 驱动 SOCKET_Magazine_Reserve，与 Arms 动画共同形成取出/插入动作。
	MagazineReserve = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineReserve"));
	MagazineReserve->SetupAttachment(WeaponMesh, ApecoxWeaponPresentation::ReserveMagazineSocketName);
	MagazineReserve->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MagazineReserve->SetGenerateOverlapEvents(false);
	MagazineReserve->SetHiddenInGame(true);
	MagazineReserve->SetVisibility(false);

	// 镭射实体挂件固定安装在枪械骨架SOCKET_Laser；具体美术由Presentation Data Asset注入。
	LaserAttachment = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserAttachment"));
	LaserAttachment->SetupAttachment(WeaponMesh, ApecoxWeaponPresentation::DefaultLaserSocketName);
	LaserAttachment->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserAttachment->SetGenerateOverlapEvents(false);
	LaserAttachment->SetVisibility(false, true);
	LaserAttachment->SetHiddenInGame(true, true);

	// RAR的SM_IG_Laser_Beam沿局部X轴展开，挂到实体挂件自己的SOCKET_Laser。
	LaserBeam = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeam"));
	LaserBeam->SetupAttachment(LaserAttachment, ApecoxWeaponPresentation::DefaultLaserSocketName);
	LaserBeam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LaserBeam->SetGenerateOverlapEvents(false);
	LaserBeam->SetCastShadow(false);
	LaserBeam->SetVisibility(false, true);
	LaserBeam->SetHiddenInGame(true, true);

	// 光点是世界表面Decal。使用绝对Transform，避免枪械父级移动把上一帧命中点带走。
	LaserDot = CreateDefaultSubobject<UDecalComponent>(TEXT("LaserDot"));
	LaserDot->SetupAttachment(PresentationRoot);
	LaserDot->SetAbsolute(true, true, true);
	LaserDot->DecalSize = FVector(5.0f);
	LaserDot->SetVisibility(false, true);
	LaserDot->SetHiddenInGame(true, true);

	LaserDotLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LaserDotLight"));
	LaserDotLight->SetupAttachment(LaserDot);
	LaserDotLight->SetIntensity(1.5f);
	LaserDotLight->SetAttenuationRadius(75.0f);
	LaserDotLight->SetLightColor(FLinearColor(1.0f, 0.015f, 0.0f));
	LaserDotLight->SetCastShadows(false);
	LaserDotLight->SetVisibility(false, true);
	LaserDotLight->SetHiddenInGame(true, true);
}

void AApecoxWeaponPresentationActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDestroyWhenOwnerUnavailable)
	{
		AActor* OwningActor = GetOwner();
		const AApecoxPlayerCharacter* OwningCharacter = Cast<AApecoxPlayerCharacter>(OwningActor);
		const bool bOwnerDead = OwningCharacter && OwningCharacter->GetHealthComponent()
			&& OwningCharacter->GetHealthComponent()->IsDeadOrDying();
		if (!IsValid(OwningActor) || OwningActor->IsActorBeingDestroyed()
			|| OwningActor->IsHidden() || bOwnerDead)
		{
			ShutdownPresentation();
			return;
		}
	}
	if (bLaserEnabled)
	{
		UpdateLaserTrace();
	}
}

UStaticMeshComponent* AApecoxWeaponPresentationActor::FindLoadedMagazineComponent() const
{
	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	for (UStaticMeshComponent* Component : StaticMeshComponents)
	{
		if (Component && Component != MagazineReserve
			&& (Component->GetFName() == ApecoxWeaponPresentation::LoadedMagazineComponentName
				|| Component->GetName().StartsWith(ApecoxWeaponPresentation::LoadedMagazineComponentName.ToString())))
		{
			return Component;
		}
	}
	return nullptr;
}

bool AApecoxWeaponPresentationActor::PrepareReloadMagazineVisuals()
{
	UStaticMeshComponent* LoadedMagazine = FindLoadedMagazineComponent();
	if (!LoadedMagazine || !MagazineReserve || !LoadedMagazine->GetStaticMesh())
	{
		ResetReloadMagazineVisuals();
		return false;
	}

	MagazineReserve->SetStaticMesh(LoadedMagazine->GetStaticMesh());
	for (int32 MaterialIndex = 0; MaterialIndex < LoadedMagazine->GetNumMaterials(); ++MaterialIndex)
	{
		MagazineReserve->SetMaterial(MaterialIndex, LoadedMagazine->GetMaterial(MaterialIndex));
	}
	MagazineReserve->SetRelativeTransform(FTransform::Identity);
	ResetReloadMagazineVisuals();
	return true;
}

void AApecoxWeaponPresentationActor::SetReloadMagazineVisualState(
	bool bLoadedMagazineVisible, bool bReserveMagazineVisible)
{
	if (UStaticMeshComponent* LoadedMagazine = FindLoadedMagazineComponent())
	{
		LoadedMagazine->SetVisibility(bLoadedMagazineVisible, true);
		LoadedMagazine->SetHiddenInGame(!bLoadedMagazineVisible, true);
	}
	if (MagazineReserve)
	{
		MagazineReserve->SetVisibility(bReserveMagazineVisible, true);
		MagazineReserve->SetHiddenInGame(!bReserveMagazineVisible, true);
	}
}

void AApecoxWeaponPresentationActor::ResetReloadMagazineVisuals()
{
	SetReloadMagazineVisualState(true, false);
}

bool AApecoxWeaponPresentationActor::ConfigureLaser(
	const UApecoxWeaponPresentationDefinition* Presentation,
	USceneComponent* ExternalMountParent,
	FName ExternalMountSocket,
	FName ExternalStableAttachmentName,
	const FTransform& ExternalMountTransform,
	bool bConvergeToViewAim)
{
	SetLaserEnabled(false);
	bLaserConfigured = false;
	bLaserConvergesToViewAim = bConvergeToViewAim;
	bDestroyWhenOwnerUnavailable = false;
	bShutdownStarted = false;

	if (!LaserAttachment || !LaserBeam || !LaserDot || !LaserDotLight)
	{
		return false;
	}

	LaserAttachment->SetStaticMesh(Presentation ? Presentation->LaserAttachmentMesh.Get() : nullptr);
	LaserBeam->SetStaticMesh(Presentation ? Presentation->LaserBeamMesh.Get() : nullptr);
	LaserDot->SetDecalMaterial(Presentation ? Presentation->LaserDotMaterial.Get() : nullptr);
	LaserAttachment->SetVisibility(false, false);
	LaserAttachment->SetHiddenInGame(true, false);

	const bool bHasRequiredAssets = Presentation && Presentation->bSupportsLaser
		&& Presentation->LaserAttachmentMesh;
	if (!bHasRequiredAssets || !Presentation->LaserBeamMesh || !Presentation->LaserDotMaterial)
	{
		return false;
	}

	const FName AttachmentSocket = ExternalMountParent
		? ExternalMountSocket
		: (Presentation->LaserAttachmentSocketName.IsNone()
			? ApecoxWeaponPresentation::DefaultLaserSocketName
			: Presentation->LaserAttachmentSocketName);
	LaserEmitterSocketName = Presentation->LaserEmitterSocketName.IsNone()
		? ApecoxWeaponPresentation::DefaultLaserSocketName
		: Presentation->LaserEmitterSocketName;

	// RAR并非把镭射直接挂到枪械SkeletalMesh。它先把Socket Laser锚点挂到
	// 当前Forestock的SOCKET_Laser；Apecox的固定Forestock同样是Prefab中的子组件。
	// 因此先尝试WeaponMesh，再在完整Prefab中寻找真正提供该Socket的固定部件。
	USceneComponent* LaserMountParent = ExternalMountParent;
	if (LaserMountParent && (AttachmentSocket.IsNone() || !LaserMountParent->DoesSocketExist(AttachmentSocket)))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ConfigureLaser: external mount '%s' has no socket '%s'."),
			*GetNameSafe(LaserMountParent), *AttachmentSocket.ToString());
		return false;
	}
	if (!LaserMountParent && WeaponMesh && WeaponMesh->DoesSocketExist(AttachmentSocket))
	{
		LaserMountParent = WeaponMesh;
	}
	else if (!LaserMountParent)
	{
		TArray<UStaticMeshComponent*> StaticMeshComponents;
		GetComponents<UStaticMeshComponent>(StaticMeshComponents);
		for (UStaticMeshComponent* Component : StaticMeshComponents)
		{
			if (Component && Component != LaserAttachment && Component != LaserBeam
				&& Component->DoesSocketExist(AttachmentSocket))
			{
				LaserMountParent = Component;
				break;
			}
		}
	}

	if (!LaserMountParent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ConfigureLaser: no component in '%s' provides mount socket '%s'."),
			*GetName(), *AttachmentSocket.ToString());
		return false;
	}
	if (!LaserAttachment->DoesSocketExist(LaserEmitterSocketName))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ConfigureLaser: attachment mesh '%s' has no emitter socket '%s'."),
			*GetNameSafe(Presentation->LaserAttachmentMesh), *LaserEmitterSocketName.ToString());
		return false;
	}

	LaserAttachment->AttachToComponent(LaserMountParent,
		FAttachmentTransformRules::SnapToTargetIncludingScale, AttachmentSocket);
	LaserAttachment->SetRelativeTransform(ExternalMountParent
		? ExternalMountTransform
		: FTransform::Identity);
	if (ExternalMountParent && !ExternalStableAttachmentName.IsNone())
	{
		if (!ExternalMountParent->DoesSocketExist(ExternalStableAttachmentName))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Apecox] ConfigureLaser: external mount '%s' has no stable attachment '%s'."),
				*GetNameSafe(ExternalMountParent), *ExternalStableAttachmentName.ToString());
			return false;
		}

		// Muzzle 只作为一次性的美术定位参考。捕获摆放结果后转挂到稳定 Root，
		// 使挂件跟随整把枪的组件/角色手部运动，但不再消费 Barrel/Muzzle 的机械回弹。
		LaserAttachment->UpdateComponentToWorld();
		const FTransform CalibratedWorldTransform = LaserAttachment->GetComponentTransform();
		LaserAttachment->AttachToComponent(ExternalMountParent,
			FAttachmentTransformRules::KeepWorldTransform, ExternalStableAttachmentName);
		LaserAttachment->SetWorldTransform(CalibratedWorldTransform);
	}
	LaserBeam->AttachToComponent(LaserAttachment,
		FAttachmentTransformRules::SnapToTargetIncludingScale, LaserEmitterSocketName);
	LaserBeam->SetRelativeTransform(FTransform::Identity);
	// 只显示实体挂件自身；不能向子组件传播，否则会在镭射关闭时误打开Beam。
	LaserAttachment->SetVisibility(true, false);
	LaserAttachment->SetHiddenInGame(false, false);

	// 第三人称表现 Actor 需要在宿主死亡/隐藏后自行收口，不能只依赖死亡委托的网络到达顺序。
	if (ExternalMountParent)
	{
		bDestroyWhenOwnerUnavailable = true;
		SetActorTickEnabled(true);
	}

	LaserTraceChannel = Presentation->LaserTraceChannel;
	LaserMaxDistance = FMath::Max(Presentation->LaserMaxDistance, 1.0f);
	LaserBeamMeshLength = FMath::Max(Presentation->LaserBeamMeshLength, 0.01f);
	LaserBeamThickness = FMath::Max(Presentation->LaserBeamThickness, 0.01f);
	LaserDotSizeBase = FMath::Max(Presentation->LaserDotSizeBase, 0.01f);
	LaserDotSizeMultiplier = FMath::Max(Presentation->LaserDotSizeMultiplier, 0.0f);
	bLaserConfigured = true;
	return true;
}

void AApecoxWeaponPresentationActor::SetLaserEnabled(bool bEnabled)
{
	bLaserEnabled = bEnabled && bLaserConfigured;
	SetActorTickEnabled(bLaserEnabled || bDestroyWhenOwnerUnavailable);

	if (LaserBeam)
	{
		LaserBeam->SetVisibility(bLaserEnabled, true);
		LaserBeam->SetHiddenInGame(!bLaserEnabled, true);
	}
	if (LaserDot)
	{
		LaserDot->SetVisibility(false, true);
		LaserDot->SetHiddenInGame(true, true);
	}
	if (LaserDotLight)
	{
		LaserDotLight->SetVisibility(false, true);
		LaserDotLight->SetHiddenInGame(true, true);
	}

	if (bLaserEnabled)
	{
		UpdateLaserTrace();
	}
}

void AApecoxWeaponPresentationActor::ShutdownPresentation()
{
	if (bShutdownStarted)
	{
		return;
	}
	bShutdownStarted = true;
	bDestroyWhenOwnerUnavailable = false;
	SetLaserEnabled(false);
	SetActorTickEnabled(false);

	// LaserAttachment 在 TP 路径会跨 Actor 附着到外部武器组件。显式先销毁这些组件，
	// 不依赖延迟 Actor Destroy/GC 处理外部 Attachment，避免死亡后留下世界空间残影。
	if (LaserDotLight)
	{
		LaserDotLight->DestroyComponent();
	}
	if (LaserDot)
	{
		LaserDot->DestroyComponent();
	}
	if (LaserBeam)
	{
		LaserBeam->DestroyComponent();
	}
	if (LaserAttachment)
	{
		LaserAttachment->DestroyComponent();
	}
	Destroy();
}

void AApecoxWeaponPresentationActor::UpdateLaserTrace()
{
	if (!bLaserEnabled || !bLaserConfigured || !LaserAttachment || !LaserBeam
		|| !LaserDot || !LaserDotLight || !GetWorld())
	{
		return;
	}

	const FTransform EmitterTransform = LaserAttachment->GetSocketTransform(
		LaserEmitterSocketName, RTS_World);
	const FVector TraceStart = EmitterTransform.GetLocation();
	// 射线的物理方向必须来自实体镭射发射 Socket。用户已经把挂件朝向校准到枪管；
	// 因此换弹、冲刺或其他动画把枪转开时，光束也随枪转动，而不会继续指向鼠标/镜头方向。
	// ADS 会在下方按 AimAlpha 收敛到玩法瞄准点，以保留一倍镜单红点表现。
	const FVector PhysicalDirection = EmitterTransform.GetUnitAxis(EAxis::X).GetSafeNormal();
	if (PhysicalDirection.IsNearlyZero())
	{
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ApecoxLaserTrace), false, this);
	QueryParams.AddIgnoredActor(this);
	AActor* OwningActor = GetOwner();
	if (OwningActor)
	{
		QueryParams.AddIgnoredActor(OwningActor);
	}

	// 腰射保持实体镭射器的真实朝向。进入ADS时，把光束平滑归零到与玩法射击一致的
	// 相机中心目标，避免侧挂镭射的平行视差在瞄具中形成第二个明显红点。
	FVector TraceDirection = PhysicalDirection;
	if (bLaserConvergesToViewAim)
	{
		if (const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(OwningActor))
		{
			const float AimWeight = FMath::Clamp(Character->GetAimAlpha(), 0.0f, 1.0f);
			if (AimWeight > UE_SMALL_NUMBER)
			{
				const FVector ViewStart = Character->GetPawnViewLocation();
				const FVector ViewDirection = Character->GetBaseAimRotation().Vector().GetSafeNormal();
				const FVector ViewEnd = ViewStart + ViewDirection * LaserMaxDistance;
				FHitResult ViewHit;
				const bool bViewBlockingHit = GetWorld()->LineTraceSingleByChannel(
					ViewHit, ViewStart, ViewEnd, LaserTraceChannel, QueryParams);
				const FVector AimPoint = bViewBlockingHit ? ViewHit.Location : ViewEnd;
				const FVector ConvergedDirection = (AimPoint - TraceStart).GetSafeNormal();
				if (!ConvergedDirection.IsNearlyZero())
				{
					TraceDirection = FMath::Lerp(
						PhysicalDirection, ConvergedDirection, AimWeight).GetSafeNormal();
				}
			}
		}
	}

	const FVector TraceEnd = TraceStart + TraceDirection * LaserMaxDistance;

	FHitResult Hit;
	const bool bBlockingHit = GetWorld()->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, LaserTraceChannel, QueryParams);
	const float BeamDistance = bBlockingHit
		? FMath::Clamp(Hit.Distance, 0.0f, LaserMaxDistance)
		: LaserMaxDistance;

	LaserBeam->SetRelativeScale3D(FVector(
		BeamDistance / LaserBeamMeshLength,
		LaserBeamThickness,
		LaserBeamThickness));
	LaserBeam->SetWorldRotation(TraceDirection.Rotation());

	LaserDot->SetVisibility(bBlockingHit, true);
	LaserDot->SetHiddenInGame(!bBlockingHit, true);
	LaserDotLight->SetVisibility(bBlockingHit, true);
	LaserDotLight->SetHiddenInGame(!bBlockingHit, true);
	if (bBlockingHit)
	{
		LaserDot->SetWorldLocation(Hit.Location);
		LaserDot->SetWorldRotation(Hit.Normal.Rotation());
		const float DotScale = LaserDotSizeBase + Hit.Distance * LaserDotSizeMultiplier;
		LaserDot->SetWorldScale3D(FVector(FMath::Max(DotScale, 0.01f)));
	}
}
