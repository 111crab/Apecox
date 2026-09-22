// Copyright Apecox. All Rights Reserved.

#include "Equipment/ApecoxEquipmentComponent.h"
#include "Weapons/ApecoxWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "Weapons/ApecoxWeaponPresentationActor.h"
#include "Weapons/ApecoxProjectileTracer.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/ApecoxAbilitySet.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Character/ApecoxHealthComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	bool IsOwningPlayerFirstPerson(const AApecoxPlayerCharacter* Character)
	{
		// Server AI controllers are local too, but bots must always use the visible third-person
		// presentation path. Only an actual locally controlled player owns the FP arms/camera.
		return Character && Character->IsPlayerControlled() && Character->IsLocallyControlled();
	}
}
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/AudioComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/SkeletalMesh.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"

UApecoxEquipmentComponent::UApecoxEquipmentComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UApecoxEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	// 蓝图预览仍可显示手臂；进入游戏后按实际装备表现决定显隐。
	RefreshWeaponPresentation();
}

void UApecoxEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyWeaponPresentation();
	Super::EndPlay(EndPlayReason);
}

USceneComponent* UApecoxEquipmentComponent::GetFirstPersonLeftHandGripPoint() const
{
	return FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetLeftHandGripPoint()
		: nullptr;
}

// ========================================================================
// ASC 生命周期
// ========================================================================

void UApecoxEquipmentComponent::InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	// 幂等：同一 ASC 不重复初始化
	if (AbilitySystemComponent == InASC)
	{
		return;
	}

	// 如果有旧的 ASC，先反初始化
	if (AbilitySystemComponent)
	{
		UninitializeFromAbilitySystem();
	}

	AbilitySystemComponent = InASC;
}

void UApecoxEquipmentComponent::UninitializeFromAbilitySystem()
{
	// 先卸下当前武器——撤销 AbilitySet、销毁表现、清空摘要
	UnequipCurrentWeapon();

	AbilitySystemComponent = nullptr;
}

// ========================================================================
// 装备（Authority Only）
// ========================================================================

bool UApecoxEquipmentComponent::EquipWeapon(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* WeaponInstance)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon called on non-authority. Ignored."));
		return false;
	}

	// --- 所有验证必须在 UnequipCurrentWeapon 之前完成 ---
	// 一旦卸下旧装备，撤销句柄就丢失了；验证失败必须保证旧装备不受影响

	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon: No ASC initialized."));
		return false;
	}

	if (!WeaponInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon: Null WeaponInstance."));
		return false;
	}

	const UApecoxWeaponDefinition* WeaponDef = WeaponInstance->GetWeaponDefinition();
	if (!WeaponDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon: WeaponInstance has no definition."));
		return false;
	}

	if (Slot == EApecoxWeaponSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Apecox] EquipWeapon: Cannot equip to None slot."));
		return false;
	}

	// --- 当前阶段 EquippedAbilitySets 为空仍算合法装备成功 ---
	// 武器 GA 在 Phase 2B 实现，当前基础设施已满足装备闭环

	// 1. 先卸下旧装备——全部验证在之前完成，失败不会走到这里
	UnequipCurrentWeapon();

	// 2. 使用 WeaponInstance 作为 SourceObject 授予 EquippedAbilitySets
	//    SourceObject 让武器 GA 能区分自己来自哪个武器实例
	FApecoxAbilitySetGrantedHandles Handles;
	for (const TObjectPtr<const UApecoxAbilitySet>& AbilitySet : WeaponDef->EquippedAbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GrantToAbilitySystem(AbilitySystemComponent, Handles,
				WeaponInstance // SourceObject 指向 WeaponInstance
			);
		}
	}
	WeaponAbilitySetHandles = Handles;

	// 3. 写入运行时引用和复制摘要
	CurrentWeaponInstance = WeaponInstance;
	EquippedWeaponState.Slot = Slot;
	EquippedWeaponState.WeaponDefinition = WeaponDef;

	// 通知复制系统——Server 不会自动触发 OnRep
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxEquipmentComponent, CurrentWeaponInstance, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxEquipmentComponent, EquippedWeaponState, this);

	// 4. 服务器主动刷新本地表现——Dedicated Server 内部跳过 Mesh 创建
	if (GetNetMode() != NM_DedicatedServer)
	{
		RefreshWeaponPresentation();
	}

	UE_LOG(LogTemp, Log, TEXT("[Apecox] EquipWeapon: Slot=%d, Weapon='%s', Instance='%s'."),
		static_cast<int32>(Slot),
		*WeaponDef->GetName(),
		*GetNameSafe(WeaponInstance));

	return true;
}

// ========================================================================
// 卸下（Authority Only，幂等）
// ========================================================================

void UApecoxEquipmentComponent::UnequipCurrentWeapon()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	// 幂等——空手时安全返回
	if (EquippedWeaponState.IsUnarmed() && !CurrentWeaponInstance)
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		// 1. 撤销武器 AbilitySet——只使用保存的句柄，不误伤英雄技能
		//    被移除的武器 Spec 会通过 ASC::OnRemoveAbility 精确清理
		//    该 SpecHandle 在 InputPressed/Held/Released 三组中的条目，
		//    不会调用全局 ClearAbilityInput()。
		WeaponAbilitySetHandles.RemoveFromAbilitySystem(AbilitySystemComponent);
	}

	// 2. 先结束公开换弹表现，再清空运行时引用和装备摘要。
	SetReloadPresentationState(0);
	SetLaserPresentationEnabledAuthority(false);
	CurrentWeaponInstance = nullptr;
	EquippedWeaponState.Slot = EApecoxWeaponSlot::None;
	EquippedWeaponState.WeaponDefinition = nullptr;

	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxEquipmentComponent, CurrentWeaponInstance, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxEquipmentComponent, EquippedWeaponState, this);

	// 3. 销毁表现
	DestroyWeaponPresentation();

	UE_LOG(LogTemp, Log, TEXT("[Apecox] UnequipCurrentWeapon: Complete."));
}

// ========================================================================
// 表现刷新
// ========================================================================

void UApecoxEquipmentComponent::OnRep_EquippedWeaponState()
{
	// RepNotify 只刷新表现，不授予能力、不修改库存
	DestroyWeaponPresentation();
	RefreshWeaponPresentation();
}

void UApecoxEquipmentComponent::OnRep_ReloadPresentationState()
{
	ApplyReloadPresentationState();
}

void UApecoxEquipmentComponent::OnRep_LaserPresentationEnabled()
{
	ApplyThirdPersonLaserPresentation();
}

void UApecoxEquipmentComponent::RebuildPresentationAfterClientRestart()
{
	// ClientRestart may happen before or after EquippedWeaponState replication. Rebuilding is
	// deliberately idempotent so either ordering converges on the same owner-specific visuals.
	DestroyWeaponPresentation();
	RefreshWeaponPresentation();
}

void UApecoxEquipmentComponent::RefreshWeaponPresentation()
{
	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	if (const UApecoxHealthComponent* Health = Character->GetHealthComponent();
		Health && Health->IsDeadOrDying())
	{
		// 死亡 Pawn 的装备摘要可能比 DeathState 更早或更晚到达。
		// 无论 RepNotify 顺序如何，都不得为死亡宿主重新生成本地 FP/TP 表现。
		DestroyWeaponPresentation();
		return;
	}

	// 手臂与完整 FP 武器共同显示；空手或表现配置不可用时不露出空手姿势。
	if (USkeletalMeshComponent* FPMesh = Character->GetFirstPersonMesh())
	{
		FPMesh->SetHiddenInGame(true, false);
	}

	// Dedicated Server 不创建视觉组件——没有渲染、没有本地玩家
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	// 空手时不需要武器表现
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	if (!WeaponDef || EquippedWeaponState.Slot == EApecoxWeaponSlot::None)
	{
		bLaserRequestedOn = false;
		bLastSubmittedLaserPresentationEnabled = false;
		return;
	}

	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef->PresentationDefinition;
	if (!Pres)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RefreshWeaponPresentation: No PresentationDefinition on '%s'."),
			*WeaponDef->GetName());
		return;
	}

	// ========================================================================
	// 第一人称完整武器表现 Prefab
	// 为什么 FP 武器改用 AApecoxWeaponPresentationActor 而不是单个 Mesh Component：
	// - SK_RAR_AssaultRifle 只是枪体，默认弹匣、护木和机械瞄具是供应商拆分的独立 Mesh，
	//   单字段无法组成完整枪械，也无法在 Blueprint Viewport 统一预览和调节。
	// - Presentation Actor 是本地纯表现对象：不复制、不承载弹药/命中/配件等玩法真相，
	//   只把一把固定组装好的完整武器交给 EquipmentComponent 作为视觉反馈。
	// - 战斗真相仍在服务器 WeaponInstance + EquippedWeaponState 摘要，表现层可自由解释。
	// ========================================================================

	// 只有本地控制的 Pawn 才生成 FP 表现；远端角色不生成不可见的 FP 枪械。
	// 空 Class 时安全跳过，不产生 FP 表现。
	if (IsOwningPlayerFirstPerson(Character) && Pres->FirstPersonWeaponPresentationClass)
	{
		bool bCreatedFirstPersonPresentation = false;
		if (!FirstPersonWeaponPresentationActor)
		{
			// 用当前 World 生成非复制的表现 Actor，Owner 设为 Character。
			// 该 Actor 是无碰撞纯表现对象，显式 AlwaysSpawn，避免被生成碰撞策略拒绝。
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Character;
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			FirstPersonWeaponPresentationActor = GetWorld()->SpawnActor<AApecoxWeaponPresentationActor>(
				Pres->FirstPersonWeaponPresentationClass, FTransform::Identity, SpawnParams);
			bCreatedFirstPersonPresentation = FirstPersonWeaponPresentationActor != nullptr;

			if (!FirstPersonWeaponPresentationActor)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Apecox] RefreshWeaponPresentation: Failed to spawn FP presentation actor "
						"from class '%s' for '%s'. Gameplay state unaffected."),
					*Pres->FirstPersonWeaponPresentationClass->GetName(),
					*GetNameSafe(Character));
			}
		}

		if (FirstPersonWeaponPresentationActor)
		{
			// 遍历 Actor 内全部 UPrimitiveComponent 统一配置 FP 可见性与碰撞。
			// 不能只配置 WeaponMesh：Blueprint 子类添加的弹匣、护木、机械瞄具同样必须遵守 FP 规则。
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			FirstPersonWeaponPresentationActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
			for (UPrimitiveComponent* Prim : PrimitiveComponents)
			{
				Prim->SetOnlyOwnerSee(true);
				Prim->SetOwnerNoSee(false);
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Prim->SetGenerateOverlapEvents(false);
				// UE 5.8 第一人称渲染类型——获得与手臂一致的 FOV 和近裁剪处理
				Prim->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
			}

			// 只注入本项目需要的纯美术资产；RAR玩法蓝图和接口不进入Apecox运行时。
			FirstPersonWeaponPresentationActor->ConfigureLaser(Pres);

			// Blueprint 中的 MagazineDefault 是常态枪槽弹匣；运行时复制其外观到
			// 继承组件 MagazineReserve，供 RAR 武器换弹动画的备用弹匣 Socket 驱动。
			if (!FirstPersonWeaponPresentationActor->PrepareReloadMagazineVisuals())
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Apecox] Reload magazine visuals unavailable on '%s': "
						"expected a StaticMeshComponent named MagazineDefault with a mesh."),
					*GetNameSafe(FirstPersonWeaponPresentationActor));
			}

			// 附着整把完整武器到第一人称 Mesh 的 Socket，再应用微调 Transform
			if (USkeletalMeshComponent* FPMesh = Character->GetFirstPersonMesh())
			{
				const bool bAttached = FirstPersonWeaponPresentationActor->AttachToComponent(FPMesh,
					FAttachmentTransformRules::SnapToTargetIncludingScale,
					Pres->FirstPersonAttachSocket);

				FirstPersonWeaponPresentationActor->SetActorRelativeTransform(
					Pres->FirstPersonAttachTransform);
				FPMesh->SetHiddenInGame(!bAttached, false);

				// 只在真实的新建+附着上启动一组完整装备表现；普通刷新不会重播。
				if (bCreatedFirstPersonPresentation && bAttached)
				{
					PlayEquipPresentation(Pres);
				}
				RefreshLaserPresentation(Character->CanUseWeaponLaser());
			}
		}
	}
	else if (!Pres->FirstPersonWeaponPresentationClass)
	{
		// 空 Class 安全跳过，不生成 FP 表现
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] RefreshWeaponPresentation: No FirstPersonWeaponPresentationClass on '%s' — skipping FP presentation."),
			*WeaponDef->GetName());
	}

	// ========================================================================
	// 第三人称武器 Mesh
	// ========================================================================

	if (!ThirdPersonWeaponMeshComponent)
	{
		ThirdPersonWeaponMeshComponent = NewObject<USkeletalMeshComponent>(
			Character, USkeletalMeshComponent::StaticClass(),
			TEXT("ThirdPersonWeaponMesh"));
		ThirdPersonWeaponMeshComponent->RegisterComponent();
	}

	ThirdPersonWeaponMeshComponent->SetSkeletalMesh(Pres->ThirdPersonWeaponMesh);
	ThirdPersonWeaponMeshComponent->SetOwnerNoSee(true);
	ThirdPersonWeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// UE 5.8 世界空间渲染类型——与第三人称角色 Mesh 保持一致
	ThirdPersonWeaponMeshComponent->SetFirstPersonPrimitiveType(
		EFirstPersonPrimitiveType::WorldSpaceRepresentation);

	// 附着到第三人称 Mesh
	if (USkeletalMeshComponent* TPMesh = Character->GetMesh())
	{
		ThirdPersonWeaponMeshComponent->AttachToComponent(TPMesh,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			Pres->ThirdPersonAttachSocket);

		ThirdPersonWeaponMeshComponent->SetRelativeTransform(
			Pres->ThirdPersonAttachTransform);
	}

	// 观察者为同一把 TP Weapon 创建本地纯表现镭射宿主。该 Actor 不复制；服务器只复制
	// 一位布尔状态，所有观察端各自从相同 Socket 做光束 Trace，晚加入客户端也能恢复现状。
	if (!IsOwningPlayerFirstPerson(Character) && !ThirdPersonLaserPresentationActor
		&& Pres->bSupportsLaser
		&& Pres->ThirdPersonWeaponMesh && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ThirdPersonLaserPresentationActor = GetWorld()->SpawnActor<AApecoxWeaponPresentationActor>(
			AApecoxWeaponPresentationActor::StaticClass(), FTransform::Identity, SpawnParams);

		const bool bConfigured = ThirdPersonLaserPresentationActor
			&& ThirdPersonLaserPresentationActor->ConfigureLaser(
				Pres,
				ThirdPersonWeaponMeshComponent,
				Pres->ThirdPersonLaserAttachmentSocketName,
				Pres->ThirdPersonLaserStableAttachmentName,
				Pres->ThirdPersonLaserAttachmentTransform,
				false);
		if (!bConfigured && ThirdPersonLaserPresentationActor)
		{
			ThirdPersonLaserPresentationActor->Destroy();
			ThirdPersonLaserPresentationActor = nullptr;
		}
		ApplyThirdPersonLaserPresentation();
	}
}

void UApecoxEquipmentComponent::DestroyWeaponPresentation()
{
	// 装备与检视都依赖当前 FP Arms/Weapon；销毁表现前先结束动作和声音。
	FinishEquipPresentation(true, true, 0.0f);
	// 检视依赖当前 FP Arms/Weapon；销毁任一表现前先原子结束整组动作与声音。
	FinishInspectPresentation(true, true, 0.0f);
	FinishReloadPresentation(true, true, 0.0f);

	// 幂等——多次调用安全
	if (AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* FPMesh = Character->GetFirstPersonMesh())
		{
			FPMesh->SetHiddenInGame(true, false);
		}
	}

	// FP 表现 Actor：使用 AActor::Destroy() 走引擎正常生命周期，不手工 delete/GC。
	// 立即置空，后续 Refresh 才能安全重建。
	if (FirstPersonWeaponPresentationActor)
	{
		FirstPersonWeaponPresentationActor->ShutdownPresentation();
		FirstPersonWeaponPresentationActor = nullptr;
	}

	if (ThirdPersonLaserPresentationActor)
	{
		ThirdPersonLaserPresentationActor->ShutdownPresentation();
		ThirdPersonLaserPresentationActor = nullptr;
	}

	if (ThirdPersonWeaponMeshComponent)
	{
		ThirdPersonWeaponMeshComponent->DestroyComponent();
		ThirdPersonWeaponMeshComponent = nullptr;
	}
}

// ========================================================================
// 开火表现
// ========================================================================

void UApecoxEquipmentComponent::PlayFirePresentation()
{
	// Dedicated Server 跳过所有视觉资源——没有渲染、没有本地玩家。
	// 射击真相仍由服务器 GA 结算，这里只负责本地/远端表现。
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	if (!WeaponDef)
	{
		return;
	}

	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef->PresentationDefinition;
	if (!Pres)
	{
		return;
	}

	// 通过本地控制关系选择表现通道——本地 Pawn 走 FP，远端 Proxy 走 TP，绝不同时播放两条路径
	if (IsOwningPlayerFirstPerson(Character))
	{
		// 开火输入优先：先收口可能仍在播放的拾枪动作和声音，再播放本发反馈。
		const float EquipBlendOut = Pres->EquipInterruptBlendOutTime;
		StopEquipPresentation(EquipBlendOut);

		// Owning Player：FP Arms + FP Weapon + FP 枪口表现
		PlayMontageOnMesh(Character->GetFirstPersonMesh(), Pres->FirstPersonArmsFireMontage);

		// FP 武器 Montage 目标改为 Presentation Actor 的 WeaponMesh
		USkeletalMeshComponent* FPWeaponMesh = FirstPersonWeaponPresentationActor
			? FirstPersonWeaponPresentationActor->GetWeaponMesh()
			: nullptr;
		PlayMontageOnMesh(FPWeaponMesh, Pres->FirstPersonWeaponFireMontage);

		// FP 枪口表现：显式 MuzzlePoint 组件，AttachPointName = None，使用组件自身 Transform
		USceneComponent* FPMuzzlePoint = FirstPersonWeaponPresentationActor
			? FirstPersonWeaponPresentationActor->GetMuzzlePoint()
			: nullptr;
		PlayMuzzlePresentation(FPMuzzlePoint, NAME_None, Pres);
	}
	else
	{
		// Simulated Proxy：TP Character + 可选 TP Weapon + TP 枪口表现
		PlayMontageOnMesh(Character->GetMesh(), Pres->ThirdPersonCharacterFireMontage);
		PlayMontageOnMesh(ThirdPersonWeaponMeshComponent, Pres->ThirdPersonWeaponFireMontage);
		PlayMuzzlePresentation(ThirdPersonWeaponMeshComponent, Pres->MuzzleSocketName, Pres);
	}
}

void UApecoxEquipmentComponent::BroadcastProjectileTracer(
	const FVector& Origin, const FVector& Direction,
	float Speed, float GravityScale, float MaxDistance)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		MulticastPlayProjectileTracer(Origin, Direction, Speed, GravityScale, MaxDistance);
	}
}

void UApecoxEquipmentComponent::MulticastPlayProjectileTracer_Implementation(
	FVector_NetQuantize10 Origin, FVector_NetQuantizeNormal Direction,
	float Speed, float GravityScale, float MaxDistance)
{
	if (!GetWorld() || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = Cast<APawn>(GetOwner());
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AApecoxProjectileTracer* Tracer = GetWorld()->SpawnActor<AApecoxProjectileTracer>(
		Origin, Direction.Rotation(), SpawnParameters);
	if (Tracer)
	{
		Tracer->InitializeTracer(Direction, Speed, GravityScale, MaxDistance);
	}
}

bool UApecoxEquipmentComponent::PlayEquipPresentation(
	const UApecoxWeaponPresentationDefinition* Presentation)
{
	FinishEquipPresentation(true, true, 0.0f);
	if (!Presentation || GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	USkeletalMeshComponent* ArmsMesh = Character ? Character->GetFirstPersonMesh() : nullptr;
	USkeletalMeshComponent* WeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr;
	UAnimInstance* ArmsAnimInstance = ArmsMesh ? ArmsMesh->GetAnimInstance() : nullptr;
	if (!IsOwningPlayerFirstPerson(Character) || !ArmsAnimInstance || !WeaponMesh
		|| !Presentation->FirstPersonArmsEquipMontage
		|| !Presentation->FirstPersonWeaponEquipMontage)
	{
		// 声音和动作是一组；缺任一动画时保持安静，避免再次出现孤立装备声。
		return false;
	}

	const float ArmsDuration = ArmsAnimInstance->Montage_Play(
		Presentation->FirstPersonArmsEquipMontage);
	if (ArmsDuration <= 0.0f)
	{
		return false;
	}

	if (!PlayMontageOnMesh(WeaponMesh, Presentation->FirstPersonWeaponEquipMontage))
	{
		ArmsAnimInstance->Montage_Stop(0.0f, Presentation->FirstPersonArmsEquipMontage);
		return false;
	}

	bEquipPresentationActive = true;
	ActiveFirstPersonArmsEquipMontage = Presentation->FirstPersonArmsEquipMontage;
	ActiveFirstPersonWeaponEquipMontage = Presentation->FirstPersonWeaponEquipMontage;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UApecoxEquipmentComponent::OnEquipArmsMontageEnded);
	ArmsAnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveFirstPersonArmsEquipMontage);

	if (Presentation->EquipSound)
	{
		EquipAudioComponent = UGameplayStatics::SpawnSoundAttached(
			Presentation->EquipSound,
			WeaponMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			false);
	}

	return true;
}

void UApecoxEquipmentComponent::StopEquipPresentation(float BlendOutTime)
{
	FinishEquipPresentation(true, true, FMath::Max(BlendOutTime, 0.0f));
}

void UApecoxEquipmentComponent::OnEquipArmsMontageEnded(
	UAnimMontage* Montage, bool bInterrupted)
{
	if (!bEquipPresentationActive || Montage != ActiveFirstPersonArmsEquipMontage)
	{
		return;
	}

	FinishEquipPresentation(bInterrupted, false, 0.0f);
}

void UApecoxEquipmentComponent::FinishEquipPresentation(
	bool /*bInterrupted*/, bool bStopArmsMontage, float BlendOutTime)
{
	if (!bEquipPresentationActive)
	{
		return;
	}

	// 先清状态，吸收 Montage_Stop 同步触发的结束委托。
	bEquipPresentationActive = false;
	UAnimMontage* ArmsMontage = ActiveFirstPersonArmsEquipMontage;
	UAnimMontage* WeaponMontage = ActiveFirstPersonWeaponEquipMontage;
	ActiveFirstPersonArmsEquipMontage = nullptr;
	ActiveFirstPersonWeaponEquipMontage = nullptr;

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	if (bStopArmsMontage && Character)
	{
		StopMontageOnMesh(Character->GetFirstPersonMesh(), ArmsMontage, BlendOutTime);
	}
	USkeletalMeshComponent* WeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr;
	StopMontageOnMesh(WeaponMesh, WeaponMontage, BlendOutTime);

	if (IsValid(EquipAudioComponent))
	{
		// 自然结束时声音通常也已结束；Stop 对已完成组件同样安全。
		EquipAudioComponent->Stop();
		EquipAudioComponent = nullptr;
	}
}

bool UApecoxEquipmentComponent::PlayInspectPresentation()
{
	if (bInspectPresentationActive || bEquipPresentationActive
		|| GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef
		? WeaponDef->PresentationDefinition.Get() : nullptr;
	USkeletalMeshComponent* ArmsMesh = Character ? Character->GetFirstPersonMesh() : nullptr;
	USkeletalMeshComponent* WeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr;
	UAnimInstance* ArmsAnimInstance = ArmsMesh ? ArmsMesh->GetAnimInstance() : nullptr;
	if (!IsOwningPlayerFirstPerson(Character) || !Pres || !ArmsAnimInstance
		|| !WeaponMesh || !Pres->FirstPersonArmsInspectMontage
		|| !Pres->FirstPersonWeaponInspectMontage)
	{
		return false;
	}

	const float ArmsDuration = ArmsAnimInstance->Montage_Play(Pres->FirstPersonArmsInspectMontage);
	if (ArmsDuration <= 0.0f)
	{
		return false;
	}

	if (!PlayMontageOnMesh(WeaponMesh, Pres->FirstPersonWeaponInspectMontage))
	{
		ArmsAnimInstance->Montage_Stop(0.0f, Pres->FirstPersonArmsInspectMontage);
		return false;
	}

	bInspectPresentationActive = true;
	ActiveFirstPersonArmsInspectMontage = Pres->FirstPersonArmsInspectMontage;
	ActiveFirstPersonWeaponInspectMontage = Pres->FirstPersonWeaponInspectMontage;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UApecoxEquipmentComponent::OnInspectArmsMontageEnded);
	ArmsAnimInstance->Montage_SetEndDelegate(EndDelegate, ActiveFirstPersonArmsInspectMontage);

	if (Pres->InspectSound)
	{
		InspectAudioComponent = UGameplayStatics::SpawnSoundAttached(
			Pres->InspectSound,
			WeaponMesh,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			false);
	}

	return true;
}

void UApecoxEquipmentComponent::StopInspectPresentation(float BlendOutTime)
{
	FinishInspectPresentation(true, true, FMath::Max(BlendOutTime, 0.0f));
}

void UApecoxEquipmentComponent::OnInspectArmsMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInspectPresentationActive || Montage != ActiveFirstPersonArmsInspectMontage)
	{
		return;
	}

	// Arms 是主时钟。自然结束时只停止仍在运行的枪械/声音，不再次停止已经结束的 Arms。
	FinishInspectPresentation(bInterrupted, false, 0.0f);
}

void UApecoxEquipmentComponent::FinishInspectPresentation(
	bool bInterrupted, bool bStopArmsMontage, float BlendOutTime)
{
	if (!bInspectPresentationActive)
	{
		return;
	}

	// 先清 Active，Montage_Stop 同步触发 EndDelegate 时会被上方守卫吸收，确保只广播一次。
	bInspectPresentationActive = false;
	UAnimMontage* ArmsMontage = ActiveFirstPersonArmsInspectMontage;
	UAnimMontage* WeaponMontage = ActiveFirstPersonWeaponInspectMontage;
	ActiveFirstPersonArmsInspectMontage = nullptr;
	ActiveFirstPersonWeaponInspectMontage = nullptr;

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	if (bStopArmsMontage && Character)
	{
		StopMontageOnMesh(Character->GetFirstPersonMesh(), ArmsMontage, BlendOutTime);
	}
	USkeletalMeshComponent* WeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr;
	StopMontageOnMesh(WeaponMesh, WeaponMontage, BlendOutTime);

	if (IsValid(InspectAudioComponent))
	{
		InspectAudioComponent->Stop();
		InspectAudioComponent = nullptr;
	}

	OnInspectPresentationEnded.Broadcast(bInterrupted);
}

bool UApecoxEquipmentComponent::PlayReloadPresentation(bool bEmptyReload)
{
	FinishReloadPresentation(true, true, 0.0f);
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef
		? WeaponDef->PresentationDefinition.Get() : nullptr;
	if (!Character || !WeaponDef || !Pres)
	{
		return false;
	}

	const bool bFirstPerson = IsOwningPlayerFirstPerson(Character);
	USkeletalMeshComponent* CharacterMesh = bFirstPerson
		? Character->GetFirstPersonMesh() : Character->GetMesh();
	USkeletalMeshComponent* WeaponMesh = bFirstPerson
		? (FirstPersonWeaponPresentationActor
			? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr)
		: ThirdPersonWeaponMeshComponent.Get();
	UAnimMontage* CharacterMontage = bFirstPerson
		? (bEmptyReload ? Pres->FirstPersonArmsEmptyReloadMontage.Get()
			: Pres->FirstPersonArmsReloadMontage.Get())
		: Pres->ThirdPersonCharacterReloadMontage.Get();
	UAnimMontage* WeaponMontage = bFirstPerson
		? (bEmptyReload ? Pres->FirstPersonWeaponEmptyReloadMontage.Get()
			: Pres->FirstPersonWeaponReloadMontage.Get())
		: Pres->ThirdPersonWeaponReloadMontage.Get();
	USoundBase* ReloadSound = Pres ? (bEmptyReload
		? Pres->EmptyReloadSound.Get() : Pres->ReloadSound.Get()) : nullptr;

	UAnimInstance* CharacterAnimInstance = CharacterMesh ? CharacterMesh->GetAnimInstance() : nullptr;
	if (!CharacterAnimInstance || !WeaponMesh || !CharacterMontage || !WeaponMontage)
	{
		return false;
	}

	// FP 的 RAR Montage 已按玩法时长制作，保持原速。Lyra TP 只有一套 2.2 秒换弹，
	// 按普通/空仓的服务器事务总时长统一缩放，使角色与枪械在允许再次开火时同步结束。
	float PlayRate = 1.0f;
	if (!bFirstPerson)
	{
		const float TargetDuration = bEmptyReload
			? WeaponDef->ReloadConfig.EmptyReloadDuration
			: WeaponDef->ReloadConfig.TacticalReloadDuration;
		if (TargetDuration > UE_SMALL_NUMBER)
		{
			PlayRate = FMath::Max(CharacterMontage->GetPlayLength() / TargetDuration,
				UE_SMALL_NUMBER);
		}
	}

	if (!PlayMontageOnMesh(CharacterMesh, CharacterMontage, PlayRate))
	{
		return false;
	}
	if (!PlayMontageOnMesh(WeaponMesh, WeaponMontage, PlayRate))
	{
		CharacterAnimInstance->Montage_Stop(0.0f, CharacterMontage);
		return false;
	}

	bReloadPresentationActive = true;
	ActiveReloadCharacterMontage = CharacterMontage;
	ActiveReloadWeaponMontage = WeaponMontage;
	ActiveReloadCharacterMesh = CharacterMesh;
	ActiveReloadWeaponMesh = WeaponMesh;
	if (bFirstPerson)
	{
		BeginReloadMagazineVisuals(bEmptyReload, Pres);
	}
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UApecoxEquipmentComponent::OnReloadCharacterMontageEnded);
	CharacterAnimInstance->Montage_SetEndDelegate(EndDelegate, CharacterMontage);

	if (ReloadSound)
	{
		ReloadAudioComponent = UGameplayStatics::SpawnSoundAttached(
			ReloadSound, WeaponMesh, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget, true, 1.0f, 1.0f, 0.0f,
			nullptr, nullptr, false);
	}
	return true;
}

void UApecoxEquipmentComponent::SetReloadPresentationState(uint8 NewState)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	NewState = FMath::Min<uint8>(NewState, 2);
	if (ReloadPresentationState == NewState)
	{
		return;
	}

	ReloadPresentationState = NewState;
	MARK_PROPERTY_DIRTY_FROM_NAME(UApecoxEquipmentComponent, ReloadPresentationState, this);
	ApplyReloadPresentationState();
}

void UApecoxEquipmentComponent::ApplyReloadPresentationState()
{
	const UApecoxWeaponPresentationDefinition* Presentation =
		EquippedWeaponState.WeaponDefinition
		? EquippedWeaponState.WeaponDefinition->PresentationDefinition.Get() : nullptr;
	StopReloadPresentation(Presentation ? Presentation->ReloadInterruptBlendOutTime : 0.1f);
	if (ReloadPresentationState != 0)
	{
		PlayReloadPresentation(ReloadPresentationState == 2);
	}
}

void UApecoxEquipmentComponent::StopReloadPresentation(float BlendOutTime)
{
	FinishReloadPresentation(true, true, FMath::Max(BlendOutTime, 0.0f));
}

void UApecoxEquipmentComponent::OnReloadCharacterMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bReloadPresentationActive || Montage != ActiveReloadCharacterMontage)
	{
		return;
	}
	FinishReloadPresentation(bInterrupted, false, 0.0f);
}

void UApecoxEquipmentComponent::FinishReloadPresentation(
	bool bInterrupted, bool bStopCharacterMontage, float BlendOutTime)
{
	// 无论 Montage 状态标记是否仍有效，卸枪/失败/重复停止都必须留下正常持枪外观。
	ClearReloadMagazineVisualTimers();
	RestoreReloadMagazineVisuals();
	if (!bReloadPresentationActive)
	{
		return;
	}

	bReloadPresentationActive = false;
	UAnimMontage* CharacterMontage = ActiveReloadCharacterMontage;
	UAnimMontage* WeaponMontage = ActiveReloadWeaponMontage;
	USkeletalMeshComponent* CharacterMesh = ActiveReloadCharacterMesh;
	USkeletalMeshComponent* WeaponMesh = ActiveReloadWeaponMesh;
	ActiveReloadCharacterMontage = nullptr;
	ActiveReloadWeaponMontage = nullptr;
	ActiveReloadCharacterMesh = nullptr;
	ActiveReloadWeaponMesh = nullptr;

	if (bStopCharacterMontage)
	{
		StopMontageOnMesh(CharacterMesh, CharacterMontage, BlendOutTime);
	}
	StopMontageOnMesh(WeaponMesh, WeaponMontage, BlendOutTime);

	if (IsValid(ReloadAudioComponent))
	{
		if (bInterrupted)
		{
			ReloadAudioComponent->Stop();
		}
		// 自然完成时允许 RAR 声音中长于动画的尾部自行播放并 AutoDestroy。
		ReloadAudioComponent = nullptr;
	}
}

void UApecoxEquipmentComponent::BeginReloadMagazineVisuals(
	bool bEmptyReload, const UApecoxWeaponPresentationDefinition* Presentation)
{
	ClearReloadMagazineVisualTimers();
	if (!FirstPersonWeaponPresentationActor || !Presentation || !GetWorld())
	{
		return;
	}

	// RAR 在换弹第 0 帧显示第二只弹匣；固定弹匣稍后才离开枪槽。
	FirstPersonWeaponPresentationActor->SetReloadMagazineVisualState(true, true);

	const float HideTime = bEmptyReload
		? Presentation->EmptyReloadHideLoadedMagazineTime
		: Presentation->TacticalReloadHideLoadedMagazineTime;
	const float RestoreTime = bEmptyReload
		? Presentation->EmptyReloadRestoreMagazineTime
		: Presentation->TacticalReloadRestoreMagazineTime;

	GetWorld()->GetTimerManager().SetTimer(
		ReloadHideLoadedMagazineTimerHandle, this,
		&UApecoxEquipmentComponent::HideLoadedReloadMagazine,
		FMath::Max(HideTime, KINDA_SMALL_NUMBER), false);
	GetWorld()->GetTimerManager().SetTimer(
		ReloadRestoreMagazineTimerHandle, this,
		&UApecoxEquipmentComponent::RestoreReloadMagazineVisuals,
		FMath::Max(RestoreTime, KINDA_SMALL_NUMBER), false);
}

void UApecoxEquipmentComponent::HideLoadedReloadMagazine()
{
	if (bReloadPresentationActive && FirstPersonWeaponPresentationActor)
	{
		FirstPersonWeaponPresentationActor->SetReloadMagazineVisualState(false, true);
	}
}

void UApecoxEquipmentComponent::RestoreReloadMagazineVisuals()
{
	if (FirstPersonWeaponPresentationActor)
	{
		FirstPersonWeaponPresentationActor->ResetReloadMagazineVisuals();
	}
}

void UApecoxEquipmentComponent::ClearReloadMagazineVisualTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadHideLoadedMagazineTimerHandle);
		World->GetTimerManager().ClearTimer(ReloadRestoreMagazineTimerHandle);
	}
}

void UApecoxEquipmentComponent::PlayEmptyFirePresentation()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef
		? WeaponDef->PresentationDefinition.Get() : nullptr;
	if (!IsOwningPlayerFirstPerson(Character) || !Pres)
	{
		return;
	}

	PlayMontageOnMesh(Character->GetFirstPersonMesh(), Pres->FirstPersonArmsEmptyFireMontage);
	USkeletalMeshComponent* WeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh() : nullptr;
	if (Pres->EmptyFireSound && WeaponMesh)
	{
		UGameplayStatics::SpawnSoundAttached(Pres->EmptyFireSound, WeaponMesh);
	}
}

bool UApecoxEquipmentComponent::ToggleLaserPresentation()
{
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef
		? WeaponDef->PresentationDefinition.Get() : nullptr;
	if (!Pres || !Pres->bSupportsLaser || !FirstPersonWeaponPresentationActor
		|| !FirstPersonWeaponPresentationActor->IsLaserConfigured())
	{
		return false;
	}

	bLaserRequestedOn = !bLaserRequestedOn;
	RefreshLaserPresentation(true);

	if (Pres->LaserToggleSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			Pres->LaserToggleSound,
			FirstPersonWeaponPresentationActor->GetWeaponMesh(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget);
	}
	return true;
}

void UApecoxEquipmentComponent::RefreshLaserPresentation(bool bPresentationAllowed)
{
	const bool bShouldBeVisible = bLaserRequestedOn && bPresentationAllowed;
	if (FirstPersonWeaponPresentationActor)
	{
		FirstPersonWeaponPresentationActor->SetLaserEnabled(bShouldBeVisible);
	}

	AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
	if (IsOwningPlayerFirstPerson(Character)
		&& bShouldBeVisible != bLastSubmittedLaserPresentationEnabled)
	{
		bLastSubmittedLaserPresentationEnabled = bShouldBeVisible;
		if (GetOwnerRole() == ROLE_Authority)
		{
			SetLaserPresentationEnabledAuthority(bShouldBeVisible);
		}
		else
		{
			ServerSetLaserPresentationEnabled(bShouldBeVisible);
		}
	}
}

void UApecoxEquipmentComponent::ServerSetLaserPresentationEnabled_Implementation(bool bEnabled)
{
	const UApecoxWeaponDefinition* WeaponDef = EquippedWeaponState.WeaponDefinition;
	const UApecoxWeaponPresentationDefinition* Pres = WeaponDef
		? WeaponDef->PresentationDefinition.Get() : nullptr;
	const bool bAccepted = !bEnabled
		|| (IsArmed() && Pres && Pres->bSupportsLaser);
	SetLaserPresentationEnabledAuthority(bAccepted && bEnabled);
}

void UApecoxEquipmentComponent::SetLaserPresentationEnabledAuthority(bool bEnabled)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (bLaserPresentationEnabled != bEnabled)
	{
		bLaserPresentationEnabled = bEnabled;
		MARK_PROPERTY_DIRTY_FROM_NAME(
			UApecoxEquipmentComponent, bLaserPresentationEnabled, this);
	}
	ApplyThirdPersonLaserPresentation();
}

void UApecoxEquipmentComponent::ApplyThirdPersonLaserPresentation()
{
	if (ThirdPersonLaserPresentationActor)
	{
		const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(GetOwner());
		const UApecoxHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
		if (Health && Health->IsDeadOrDying())
		{
			ThirdPersonLaserPresentationActor->ShutdownPresentation();
			ThirdPersonLaserPresentationActor = nullptr;
			return;
		}
		ThirdPersonLaserPresentationActor->SetLaserEnabled(bLaserPresentationEnabled);
	}
}

void UApecoxEquipmentComponent::HandleOwnerDeathPresentation()
{
	// Owning Client 先提交关闭状态；Authority 为远端 Pawn 直接清零公开摘要。
	// 两条路径都幂等，随后立即销毁本端独立表现 Actor。
	RefreshLaserPresentation(false);
	bLaserRequestedOn = false;
	bLastSubmittedLaserPresentationEnabled = false;
	if (GetOwnerRole() == ROLE_Authority)
	{
		SetLaserPresentationEnabledAuthority(false);
	}
	DestroyWeaponPresentation();

	// 兜底清理同一 Pawn 名下可能由旧刷新路径遗留、已不在成员指针中的表现 Actor。
	// 只在死亡时执行一次，不进入 Tick，也不会影响其他角色的表现。
	if (UWorld* World = GetWorld())
	{
		TArray<TWeakObjectPtr<AApecoxWeaponPresentationActor>> OwnedPresentations;
		for (TActorIterator<AApecoxWeaponPresentationActor> It(World); It; ++It)
		{
			AApecoxWeaponPresentationActor* PresentationActor = *It;
			if (PresentationActor && PresentationActor->GetOwner() == GetOwner()
				&& !PresentationActor->IsActorBeingDestroyed())
			{
				OwnedPresentations.Add(PresentationActor);
			}
		}
		for (const TWeakObjectPtr<AApecoxWeaponPresentationActor>& PresentationActor : OwnedPresentations)
		{
			if (PresentationActor.IsValid())
			{
				PresentationActor->ShutdownPresentation();
			}
		}
	}
}

bool UApecoxEquipmentComponent::PlayMontageOnMesh(
	USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, float PlayRate) const
{
	if (!MeshComponent || !Montage || !FMath::IsFinite(PlayRate) || PlayRate <= 0.0f)
	{
		return false;
	}

	// A Montage authored for another skeleton can still reach the Single Node fallback on a
	// weapon mesh. UE will then evaluate incompatible bone tracks and may rotate the entire
	// attached weapon. Reject the presentation before changing Animation Mode.
	const USkeletalMesh* SkeletalMesh = MeshComponent->GetSkeletalMeshAsset();
	const USkeleton* MeshSkeleton = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr;
	const USkeleton* MontageSkeleton = Montage->GetSkeleton();
	if (MeshSkeleton && MontageSkeleton && MeshSkeleton != MontageSkeleton)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] PlayMontageOnMesh: skipping montage '%s' on mesh '%s' because skeletons differ (%s vs %s)."),
			*GetNameSafe(Montage), *MeshComponent->GetName(),
			*GetNameSafe(MontageSkeleton), *GetNameSafe(MeshSkeleton));
		return false;
	}

	// 普通 AnimBP 实例：Character/Arms（或任何配了 AnimClass 的 Mesh）走 Montage_Play。
	// 关键：绝不能把 Character/Arms 从 Animation Blueprint 模式切成 Single Node。
	// GetSingleNodeInstance() 只在已有 Single Node 实例时非空，用来排除 Weapon Mesh 已回退过的情况。
	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	const bool bHasAnimBPInstance = (AnimInstance != nullptr) && (MeshComponent->GetSingleNodeInstance() == nullptr);
	if (bHasAnimBPInstance)
	{
		// Montage_Play 返回 Montage 时长；0 表示 Slot 不匹配/Skeleton 不兼容等播放失败
		const float MontageLength = AnimInstance->Montage_Play(Montage, PlayRate);
		return MontageLength > 0.0f;
	}

	// 没有普通 AnimInstance：Single Node 回退只允许命中 FP Presentation Actor 的 WeaponMesh
	// 与原 TP Weapon Mesh Component。显式比较指针，禁止 Character/Arms 因缺 AnimInstance 被误切成 Single Node。
	USkeletalMeshComponent* FPWeaponMesh = FirstPersonWeaponPresentationActor
		? FirstPersonWeaponPresentationActor->GetWeaponMesh()
		: nullptr;
	const bool bIsWeaponMesh = (MeshComponent == FPWeaponMesh)
		|| (MeshComponent == ThirdPersonWeaponMeshComponent);
	if (!bIsWeaponMesh)
	{
		// FP Arms / TP Character 缺少 AnimInstance：直接失败，不改变其 Animation Mode。
		// 本函数只在 Fire Cue 时调用，不会每 Tick 输出。
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] PlayMontageOnMesh: Mesh '%s' has no AnimInstance and is not a weapon mesh — "
				"skipping montage '%s'."),
			*MeshComponent->GetName(), *GetNameSafe(Montage));
		return false;
	}

	// Weapon Mesh 无 AnimInstance：机械动作没有 Locomotion 图，允许 Single Node 播放 Montage。
	// PlayAnimation 在无 AnimInstance 时会创建 UAnimSingleNodeInstance。
	MeshComponent->PlayAnimation(Montage, false);
	if (UAnimSingleNodeInstance* SingleNode = MeshComponent->GetSingleNodeInstance())
	{
		SingleNode->SetPlayRate(PlayRate);
	}
	return true;
}

void UApecoxEquipmentComponent::StopMontageOnMesh(
	USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage, float BlendOutTime) const
{
	if (!MeshComponent || !Montage)
	{
		return;
	}

	UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance();
	if (AnimInstance && MeshComponent->GetSingleNodeInstance() == nullptr)
	{
		AnimInstance->Montage_Stop(FMath::Max(BlendOutTime, 0.0f), Montage);
		return;
	}

	// 独立枪械 Mesh 使用 Single Node 回退，Stop 会停止当前机械动作。
	MeshComponent->Stop();
}

void UApecoxEquipmentComponent::PlayMuzzlePresentation(
	USceneComponent* AttachComponent,
	FName AttachPointName,
	const UApecoxWeaponPresentationDefinition* Presentation) const
{
	if (!AttachComponent || !Presentation)
	{
		return;
	}

	// 附着点语义分流：
	// - AttachPointName != None：使用指定 Socket（TP 的 MuzzleSocketName），播放前验证存在；
	// - AttachPointName == None 且 AttachComponent 是 FP MuzzlePoint（普通 SceneComponent）：
	//   使用组件自身 Transform；
	// - AttachPointName == None 且 AttachComponent 是 SkeletalMesh（TP 武器，MuzzleSocketName 为空）：
	//   保持旧语义——跳过 TP 枪口表现，不回退到武器原点。
	if (AttachPointName != NAME_None)
	{
		// Socket 缺失时不崩溃——只输出一次 Verbose 日志，枪口表现可安全缺失
		if (!AttachComponent->DoesSocketExist(AttachPointName))
		{
			UE_LOG(LogTemp, Verbose,
				TEXT("[Apecox] PlayMuzzlePresentation: Socket '%s' not found on '%s' — skipping muzzle FX/SFX."),
				*AttachPointName.ToString(), *AttachComponent->GetName());
			return;
		}
	}
	else if (AttachComponent->IsA<USkeletalMeshComponent>())
	{
		// TP 武器的 MuzzleSocketName 为空：不回退到武器原点，直接跳过
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] PlayMuzzlePresentation: No muzzle socket on TP weapon '%s' — skipping muzzle FX/SFX."),
			*AttachComponent->GetName());
		return;
	}
	// 否则（FP MuzzlePoint，AttachPointName == None）落到下方，使用组件自身 Transform

	// 一次性枪口 Niagara——附着到同一 Component/AttachPoint
	if (Presentation->MuzzleFlashSystem)
	{
		UNiagaraComponent* MuzzleFlash = UNiagaraFunctionLibrary::SpawnSystemAttached(
			Presentation->MuzzleFlashSystem,
			AttachComponent,
			AttachPointName,
			FVector::ZeroVector,
			Presentation->MuzzleEffectRotation,
			EAttachLocation::SnapToTarget,
			true,  // bAutoDestroy
			false, // 先应用可选渲染类型，再激活
			ENCPoolMethod::None,
			true); // bPreCullCheck
		if (MuzzleFlash)
		{
			// Apecox的武器使用UE独立第一人称FOV/深度路径。实测如果枪口Niagara
			// 不使用同一Primitive Type，整套火光与烟雾都会丢失，因此FP路径必须同步。
			if (FirstPersonWeaponPresentationActor
				&& AttachComponent == FirstPersonWeaponPresentationActor->GetMuzzlePoint())
			{
				MuzzleFlash->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
			}
			MuzzleFlash->Activate();
		}
	}

	// 一次性开火声音——附着到同一 Component/AttachPoint
	if (Presentation->FireSound)
	{
		UGameplayStatics::SpawnSoundAttached(
			Presentation->FireSound,
			AttachComponent,
			AttachPointName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget);
	}
}

// ========================================================================
// 复制注册
// ========================================================================

void UApecoxEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 装备摘要对所有相关客户端可见——驱动 TP 武器表现
	DOREPLIFETIME(UApecoxEquipmentComponent, EquippedWeaponState);
	DOREPLIFETIME(UApecoxEquipmentComponent, ReloadPresentationState);
	DOREPLIFETIME(UApecoxEquipmentComponent, bLaserPresentationEnabled);

	// 当前装备实例只对 Owner 可见——Owing Client 用于武器 GA 来源身份校验
	DOREPLIFETIME_CONDITION(UApecoxEquipmentComponent, CurrentWeaponInstance, COND_OwnerOnly);
}
