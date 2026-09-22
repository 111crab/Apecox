// Copyright Apecox. All Rights Reserved.

#include "Character/ApecoxPlayerCharacter.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/ApecoxAbilitySet.h"
#include "Input/ApecoxInputConfig.h"
#include "Input/ApecoxInputComponent.h"
#include "Character/ApecoxHealthComponent.h"
#include "Game/ApecoxGameMode.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/CollisionProfile.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Weapons/ApecoxWeaponPickup.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "Inventory/ApecoxInventoryComponent.h"
#include "Player/ApecoxPlayerController.h"
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

AApecoxPlayerCharacter::AApecoxPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CachedAbilitySystemComponent = nullptr;

	// --- 胶囊体：使用 UE 5.8 第一人称模板基线（紧凑站立 - Radius 34, Half Height 96） ---
	// 同一个 Capsule 用于第一/第三人称，确保碰撞和移动模拟在所有视角下一致
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(34.0f, 96.0f);
	}

	// --- 第三人称全身 Mesh（已有） ---
	// GetMesh()：仅非拥有者可见（OwnerNoSee），避免本地第一人称看到重叠的全身
	// WorldSpaceRepresentation：按世界空间渲染第三人称，不影响第一人称摄像机
	USkeletalMeshComponent* ThirdPersonMesh = GetMesh();
	if (ThirdPersonMesh)
	{
		ThirdPersonMesh->SetOwnerNoSee(true);
		ThirdPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	}

	// --- 第一人称摄像机 ---
	// 附着于 Capsule/Root，使用 Pawn Control Rotation 控制视角。
	// FirstPersonFieldOfView/FirstPersonScale 复现了 UE 5.8 官方模板的 FOV 修正与视线遮挡比例。
	// BaseEyeHeight 提供站立相机的初始局部 Z，运行时由统一姿态镜头更新；
	// GetPawnViewLocation 返回实际相机位置，供已有射击视点入口使用。
	// 精确 Arms 相对 Transform 由 BP_ApecoxPlayerCharacter 配置，不在 C++ 硬编码。
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
		FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, BaseEyeHeight));
		FirstPersonCamera->bUsePawnControlRotation = true;
		FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
		FirstPersonCamera->bEnableFirstPersonScale = true;
		FirstPersonCamera->FirstPersonFieldOfView = 70.0f;
		// 将手臂和枪械的渲染深度一起压向相机，减轻近墙遮挡；透视画面大小由 FP FOV 保持。
		// 这是渲染参数，不缩放骨骼、Socket 或用于命中检测的世界位置。
		FirstPersonCamera->FirstPersonScale = 0.1f;
	}

	// --- 第一人称 Mesh ---
	// 仅拥有者可见（OnlyOwnerSee），附着于 FirstPersonCamera——不再附着完整 Manny。
	// 专用 Arms 结构：摄像机不再附着 FirstPersonMesh 的 head Socket，
	// FirstPersonMesh 也不再附着 ThirdPersonMesh。
	// 资产（SkeletalMesh/AnimClass）与精确相对 Transform 不写死在 C++ 中，由蓝图子类配置。
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetupAttachment(FirstPersonCamera);
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		FirstPersonMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}

	// --- 角色移动配置 ---
	// 角色 Yaw 跟随 Controller Yaw（bUseControllerRotationYaw=true），
	// 因此 HandleMoveInput 的 Actor Forward/Right 方向与第一人称视角方向一致。
	// bOrientRotationToMovement=false 禁止 CMC 自动根据速度方向旋转角色，
	// 避免与 Controller 视角旋转冲突。
	bUseControllerRotationYaw = true;

	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->bOrientRotationToMovement = false;
		CMC->MaxWalkSpeed = WalkSpeed;
		CMC->BrakingDecelerationFalling = 1500.0f;
		CMC->AirControl = 0.5f;

		// 蹲伏使用内建 CMC：复用其客户端预测、服务器校正、胶囊高度和复制行为，
		// 不新增 RPC、GA、GE 或状态 Tag。
		CMC->GetNavAgentPropertiesRef().bCanCrouch = true;
		CMC->MaxWalkSpeedCrouched = CrouchedSpeed;
		// UE 默认阻止蹲伏角色主动走下悬崖边缘，会在平台边缘表现得像撞到不可见墙。
		// Apecox 的蹲伏仍应能自然进入 Falling，由现有 CMC 预测/复制和 TP FallLoop 表现接管。
		CMC->bCanWalkOffLedgesWhenCrouching = true;
	}

	// HealthComponent 生命周期由 Character 拥有；绑定死亡委托
	HealthComponent = CreateDefaultSubobject<UApecoxHealthComponent>(TEXT("HealthComponent"));

	// EquipmentComponent：当前装备真相——Authority 管理，公开摘要复制给其他客户端
	EquipmentComponent = CreateDefaultSubobject<UApecoxEquipmentComponent>(TEXT("EquipmentComponent"));
}

void AApecoxPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (EquipmentComponent)
	{
		EquipmentComponent->OnInspectPresentationEnded.AddUObject(
			this, &AApecoxPlayerCharacter::OnInspectPresentationEnded);
	}

	if (FirstPersonMesh)
	{
		BaseFirstPersonMeshRelativeTransform = FirstPersonMesh->GetRelativeTransform();
		FirstPersonMesh->AddTickPrerequisiteActor(this);
	}

	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		StandingCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	}

	// C++ 构造函数执行时还看不到 Blueprint 最终覆盖的 WalkSpeed 等字段。
	// BeginPlay 再同步一次，确保玩家与由其 Blueprint 复制出的 Bot 从出生第一帧
	// 就使用各自 Class Defaults 中的真实配置，而不是原生 CDO 的临时构造值。
	RefreshMovementSpeed();

	LastFootstepLocation = GetActorLocation();
	bFootstepLocationInitialized = true;

	if (FirstPersonCamera)
	{
		// 读取蓝图最终值作为运行时基准。用户在 BP 中配置的 90 FOV 不会被 C++ 默认值覆盖。
		BaseCameraFieldOfView = FirstPersonCamera->FieldOfView;
		BaseFirstPersonFieldOfView = FirstPersonCamera->FirstPersonFieldOfView;
		BaseFirstPersonCameraRelativeLocation = FirstPersonCamera->GetRelativeLocation();
		BaseFirstPersonCameraRelativeRotation = FirstPersonCamera->GetRelativeRotation();
		CurrentViewHeight = GetTargetViewHeight();
		bStanceCameraInitialized = true;
		UpdateFirstPersonCamera(0.0f);
	}
}

void AApecoxPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateWeaponReloadState();
	UpdateAimState(DeltaSeconds);
	UpdateLocomotionState(DeltaSeconds);
	if (EquipmentComponent)
	{
		EquipmentComponent->RefreshLaserPresentation(CanUseWeaponLaser());
	}
	UpdateMovementAudio();
	UpdateFirstPersonLookSway(DeltaSeconds);
	UpdateFirstPersonRecoil(DeltaSeconds);
}

void AApecoxPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (!HealthComponent || !HealthComponent->IsDeadOrDying())
	{
		PlayMovementSoundAtLocation(LandingSound);
	}
}

FVector AApecoxPlayerCharacter::GetPawnViewLocation() const
{
	return FirstPersonCamera ? FirstPersonCamera->GetComponentLocation() : Super::GetPawnViewLocation();
}

void AApecoxPlayerCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (bSuppressNextCrouchStartSound)
	{
		bSuppressNextCrouchStartSound = false;
	}
	else
	{
		PlayStanceSound(CrouchStartSound);
	}
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (bStanceCameraInitialized && CMC && !CMC->bCrouchMaintainsBaseLocation)
	{
		// 空中蹲伏原地缩胶囊，底部会升高；同步换算眼高基准，避免镜头被抬起。
		CurrentViewHeight -= HalfHeightAdjust;
	}
	// 地面蹲伏会移动胶囊中心。立即重算局部 Z，下一帧再推进平滑；
	// 同时保留原生 Mesh/眼高/Blueprint 通知。
	UpdateFirstPersonCamera(0.0f);
}

void AApecoxPlayerCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	// 蹲姿直接交接到趴姿时，SetProne会播放一次趴下布料声；这里不能再播放起身声。
	if (!bIsProne)
	{
		PlayStanceSound(CrouchStopSound);
	}
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (bStanceCameraInitialized && !bIsProne && CMC && !CMC->bCrouchMaintainsBaseLocation)
	{
		// 自定义入趴始终保持底部；这里仅换算原生空中起身的基准变化。
		CurrentViewHeight += HalfHeightAdjust;
	}
	UpdateFirstPersonCamera(0.0f);
}

void AApecoxPlayerCharacter::UpdateMovementAudio()
{
	const FVector CurrentLocation = GetActorLocation();
	if (!bFootstepLocationInitialized)
	{
		LastFootstepLocation = CurrentLocation;
		bFootstepLocationInitialized = true;
		return;
	}

	// 姿态改变只移动Z；只累计水平路程，避免蹲下/趴下本身被误判为落脚。
	const float AddedDistance = FVector::Dist2D(CurrentLocation, LastFootstepLocation);
	LastFootstepLocation = CurrentLocation;

	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	const FVector Velocity2D = FVector(GetVelocity().X, GetVelocity().Y, 0.0f);
	const bool bCanStep = CMC && CMC->IsMovingOnGround() && Velocity2D.SizeSquared() > FMath::Square(5.0f)
		&& (!HealthComponent || !HealthComponent->IsDeadOrDying());
	if (!bCanStep)
	{
		return;
	}

	if (AccumulateFootstepDistance(AddedDistance, GetCurrentFootstepDistance()))
	{
		PlayMovementSoundAtLocation(FootstepSound);
	}
}

float AApecoxPlayerCharacter::GetCurrentFootstepDistance() const
{
	if (bIsProne || IsCrouched())
	{
		return LowStanceFootstepDistance;
	}
	if (bIsSprinting)
	{
		return SprintFootstepDistance;
	}
	return bIsAiming ? AimFootstepDistance : WalkFootstepDistance;
}

bool AApecoxPlayerCharacter::AccumulateFootstepDistance(float AddedDistance, float RequiredDistance)
{
	if (AddedDistance <= 0.0f || RequiredDistance <= 0.0f)
	{
		return false;
	}

	AccumulatedFootstepDistance += AddedDistance;
	if (AccumulatedFootstepDistance < RequiredDistance)
	{
		return false;
	}

	// 与RAR一致，每次更新最多播放一步并保留超出阈值的余量。
	AccumulatedFootstepDistance = FMath::Max(0.0f, AccumulatedFootstepDistance - RequiredDistance);
	return true;
}

void AApecoxPlayerCharacter::PlayMovementSoundAtLocation(USoundBase* Sound) const
{
	if (Sound && GetNetMode() != NM_DedicatedServer)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

void AApecoxPlayerCharacter::PlayStanceSound(USoundBase* Sound) const
{
	if (!Sound || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (IsLocallyControlled() && FirstPersonCamera)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, FirstPersonCamera);
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
}

UAbilitySystemComponent* AApecoxPlayerCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystemComponent;
}

UApecoxAbilitySystemComponent* AApecoxPlayerCharacter::GetApecoxAbilitySystemComponent() const
{
	return CachedAbilitySystemComponent;
}

void AApecoxPlayerCharacter::InitializeAbilitySystem()
{
	AApecoxPlayerState* ApecoxPS = GetPlayerState<AApecoxPlayerState>();
	if (!ApecoxPS)
	{
		return;
	}

	UApecoxAbilitySystemComponent* ASC = ApecoxPS->GetApecoxAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (CachedAbilitySystemComponent == ASC && ASC->GetAvatarActor() == this)
	{
		return;
	}

	if (CachedAbilitySystemComponent && CachedAbilitySystemComponent != ASC)
	{
		UninitializeAbilitySystem();
	}

	AActor* ExistingAvatar = ASC->GetAvatarActor();
	if (ExistingAvatar && ExistingAvatar != this)
	{
		if (ExistingAvatar == ApecoxPS)
		{
			// PlayerState/PlayerState 是 ASC::InitializeComponent() 默认调用
			// InitAbilityActorInfo(Owner, Owner) 的合法过渡状态。
			// 无需 ensure、无需 SetAvatarActor(nullptr)，直接继续到正式绑定。
		}
		else if (AApecoxPlayerCharacter* OldCharacter = Cast<AApecoxPlayerCharacter>(ExistingAvatar))
		{
			OldCharacter->UninitializeAbilitySystem();
		}
		else
		{
			ensureMsgf(false, TEXT("[Apecox] ASC %s has unexpected AvatarActor %s when %s is binding."),
				*GetNameSafe(ASC), *GetNameSafe(ExistingAvatar), *GetNameSafe(this));
			ASC->SetAvatarActor(nullptr);
		}
	}

	ASC->InitAbilityActorInfo(ApecoxPS, this);
	CachedAbilitySystemComponent = ASC;

	GrantPawnAbilitySets();

	// 初始化 HealthComponent——绑定 VitalAttributeSet 委托，开始监听生命值
	HealthComponent->InitializeWithAbilitySystem(ASC);

	// 初始化 EquipmentComponent——绑定 ASC，为武器 AbilitySet 授予做准备
	EquipmentComponent->InitializeWithAbilitySystem(ASC);

	// 绑定死亡状态转换委托；解绑在 UninitializeAbilitySystem 中处理
	OnDeathStartedHandle = HealthComponent->OnDeathStarted.AddUObject(
		this, &AApecoxPlayerCharacter::OnDeathStarted);
	OnDeathFinishedHandle = HealthComponent->OnDeathFinished.AddUObject(
		this, &AApecoxPlayerCharacter::OnDeathFinished);

	// 仅在 Authority 上应用 PawnInitializationEffect 恢复出生属性
	ApplyPawnInitializationEffect();

	// 护盾成长属于 PlayerState，因此玩家重生时保留等级/进化点并补满当前等级护盾。
	// AI 仍复用同一 ASC/Health 链路，但明确保持 0 护盾、0 进化点。
	ApecoxPS->InitializeCombatAttributesForPawn(IsPlayerControlled());
}

void AApecoxPlayerCharacter::UninitializeAbilitySystem()
{
	bWeaponFireInputHeld = false;
	CancelWeaponReload();
	CancelInspect();
	ResetAimState();
	ResetFirstPersonRecoil();
	if (CachedAbilitySystemComponent)
	{
		if (CachedAbilitySystemComponent->GetAvatarActor() == this)
		{
			// 严格按批准顺序执行，保证旧 Pawn 清理时 ASC Avatar 仍指向当前 Pawn：
			// 1) 移除 Character 对 HealthComponent 的死亡委托
			if (HealthComponent)
			{
				HealthComponent->OnDeathStarted.Remove(OnDeathStartedHandle);
				HealthComponent->OnDeathFinished.Remove(OnDeathFinishedHandle);
			}

			// 2) HealthComponent 反初始化——此时 ASC Avatar 仍是当前 Pawn，
			//    能正确清除 State.Death.Dead 等死亡 Tag，不留到新 Pawn
			if (HealthComponent)
			{
				HealthComponent->UninitializeFromAbilitySystem();
			}

			// 2.5) EquipmentComponent 反初始化——撤销武器 AbilitySet、销毁表现、
			//    清空摘要。必须在 ASC 仍有效且 PawnAbilitySet 未撤销时执行，
			//    确保武器 GA 精确取消不误伤英雄技能
			if (EquipmentComponent)
			{
				EquipmentComponent->UninitializeFromAbilitySystem();
			}

			// 3) 清输入、取消 Activity、移除 Pawn AbilitySet
			CachedAbilitySystemComponent->ClearAbilityInput();
			CachedAbilitySystemComponent->CancelAllAbilities();
			RemovePawnAbilitySets();

			// 4) 最后清空 ASC Avatar/ActorInfo
			if (CachedAbilitySystemComponent->GetOwnerActor() != nullptr)
			{
				CachedAbilitySystemComponent->SetAvatarActor(nullptr);
			}
			else
			{
				CachedAbilitySystemComponent->ClearActorInfo();
			}
		}
		else
		{
			// 旧 Pawn 晚解绑保护：ASC Avatar 已不是当前 Pawn，
			// 不碰 ASC 状态（新 Pawn 已绑定），仅清理本地引用
			if (GetLocalRole() == ROLE_Authority)
			{
				RemovePawnAbilitySets();
			}

			// 移除死亡委托并反初始化 HealthComponent；
			// UninitializeFromAbilitySystem 内部有 ASC Avatar == Owner 守卫，
			// 因此只会解除旧组件对 VitalAttributeSet 的委托，不会清除新 Pawn 的死亡 Tag
			if (HealthComponent)
			{
				HealthComponent->OnDeathStarted.Remove(OnDeathStartedHandle);
				HealthComponent->OnDeathFinished.Remove(OnDeathFinishedHandle);
				HealthComponent->UninitializeFromAbilitySystem();
			}

			// 旧 Pawn 晚解绑：EquipmentComponent 销毁表现和清空状态
			if (EquipmentComponent)
			{
				EquipmentComponent->UninitializeFromAbilitySystem();
			}
		}

		CachedAbilitySystemComponent = nullptr;
	}
}

void AApecoxPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
	TryEquipStartingWeapon();
}

void AApecoxPlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// On an owning client the public equipment summary can arrive before Controller ownership.
	// That first OnRep legitimately classifies the Pawn as non-local and builds TP presentation.
	// ClientRestart is the engine point at which local possession is ready; rebuild so FP arms and
	// weapon no longer depend on network property arrival order.
	// Listen host/Standalone already equip after authority possession and have no replication race;
	// rebuilding there would replay the equip presentation unnecessarily.
	if (GetNetMode() == NM_Client && IsLocallyControlled() && EquipmentComponent)
	{
		EquipmentComponent->RebuildPresentationAfterClientRestart();
	}
}

void AApecoxPlayerCharacter::TryEquipStartingWeapon()
{
	if (!HasAuthority() || !StartingWeaponDefinition || !EquipmentComponent)
	{
		return;
	}

	AApecoxPlayerController* ApecoxController = Cast<AApecoxPlayerController>(GetController());
	UApecoxInventoryComponent* Inventory = ApecoxController
		? ApecoxController->GetInventoryComponent()
		: nullptr;
	if (!Inventory)
	{
		return;
	}

	// Pawn 替换但 Controller 库存被保留时，重新把既有 Primary 装到新 Pawn，
	// 不创建第二个实例，也不重置其弹药。
	if (UApecoxWeaponInstance* ExistingPrimary = Inventory->GetPrimaryWeapon())
	{
		if (EquipmentComponent->GetCurrentWeaponInstance() != ExistingPrimary)
		{
			EquipmentComponent->EquipWeapon(EApecoxWeaponSlot::Primary, ExistingPrimary);
		}
		return;
	}

	// 正常出生/死亡清库存后的重生：复用拾取已经验证过的原子创建与装备事务，
	// Pickup 为空表示这是出生配装，不涉及 Claim 或销毁世界 Actor。
	Inventory->TryAddAndEquipWeapon(StartingWeaponDefinition, nullptr);
}

void AApecoxPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (GetPlayerState())
	{
		InitializeAbilitySystem();
	}
	else
	{
		UninitializeAbilitySystem();
	}
}

void AApecoxPlayerCharacter::UnPossessed()
{
	// 修复 9：解除占有前先移除本地 IMC
	RemoveDefaultInputMappingContext();
	UninitializeAbilitySystem();
	Super::UnPossessed();
}

void AApecoxPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 修复 9：销毁前安全清理 IMC
	RemoveDefaultInputMappingContext();
	UninitializeAbilitySystem();
	if (EquipmentComponent)
	{
		EquipmentComponent->OnInspectPresentationEnded.RemoveAll(this);
	}
	Super::EndPlay(EndPlayReason);
}

// --- 输入 ---

void AApecoxPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UApecoxInputComponent* ApecoxIC = Cast<UApecoxInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(ApecoxIC, TEXT("[Apecox] PlayerInputComponent is not UApecoxInputComponent.")))
	{
		return;
	}

	ApecoxIC->RemoveBinds(AbilityInputBindingHandles);

	const APlayerController* PC = GetController<APlayerController>();
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (const ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// 修复 9：添加前先移除同一 Context，保证幂等
			if (DefaultInputMappingContext)
			{
				Subsystem->RemoveMappingContext(DefaultInputMappingContext);
				Subsystem->AddMappingContext(DefaultInputMappingContext, DefaultInputMappingPriority);
			}
		}
	}

	if (InputConfig)
	{
		ApecoxIC->BindAbilityActions(InputConfig, this,
			&AApecoxPlayerCharacter::HandleAbilityInputTagPressed,
			&AApecoxPlayerCharacter::HandleAbilityInputTagReleased,
			AbilityInputBindingHandles);

		// --- Native 输入绑定（第一/第三人称共用） ---
		// 移动和观察不经过 ASC/GAS 路由——直接调用本地 CMC/Controller 方法，
		// 依赖 CMC 原生客户端预测和服务器校正，不需要单独网络 RPC。
		// 死亡后 CMC 已由 OnDeathStarted 禁用，不需要在输入函数中额外判断死亡状态。
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Move,
			ETriggerEvent::Triggered, this, &AApecoxPlayerCharacter::HandleMoveInput);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Look_Mouse,
			ETriggerEvent::Triggered, this, &AApecoxPlayerCharacter::HandleLookInput);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Jump,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleJumpStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Jump,
			ETriggerEvent::Completed, this, &AApecoxPlayerCharacter::HandleJumpCompleted);

		// Canceled 绑定：当 EnhancedInput 取消当前 IA（如切换 IMC）时释放跳跃，
		// 避免跳跃持续输入使新角色不受控
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Jump,
			ETriggerEvent::Canceled, this, &AApecoxPlayerCharacter::HandleJumpCompleted);

		// 蹲伏：按下进入，松开/取消退出；复用 CMC 原生预测，无单独 RPC
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Crouch,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleCrouchStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Crouch,
			ETriggerEvent::Completed, this, &AApecoxPlayerCharacter::HandleCrouchCompleted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Crouch,
			ETriggerEvent::Canceled, this, &AApecoxPlayerCharacter::HandleCrouchCompleted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Sprint,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleSprintStarted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Sprint,
			ETriggerEvent::Completed, this, &AApecoxPlayerCharacter::HandleSprintCompleted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Sprint,
			ETriggerEvent::Canceled, this, &AApecoxPlayerCharacter::HandleSprintCompleted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Prone,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleProneStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Weapon_Aim,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleAimStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Weapon_Inspect,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleInspectStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Weapon_Reload,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleReloadStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Weapon_Laser,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleLaserToggleStarted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Left,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleLeanLeftStarted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Left,
			ETriggerEvent::Completed, this, &AApecoxPlayerCharacter::HandleLeanLeftCompleted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Left,
			ETriggerEvent::Canceled, this, &AApecoxPlayerCharacter::HandleLeanLeftCompleted);

		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Right,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleLeanRightStarted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Right,
			ETriggerEvent::Completed, this, &AApecoxPlayerCharacter::HandleLeanRightCompleted);
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Lean_Right,
			ETriggerEvent::Canceled, this, &AApecoxPlayerCharacter::HandleLeanRightCompleted);

		// 交互——如 E 键拾取步枪
		ApecoxIC->BindNativeAction(InputConfig, ApecoxGameplayTags::InputTag_Interact,
			ETriggerEvent::Started, this, &AApecoxPlayerCharacter::HandleInteractStarted);
	}
}

void AApecoxPlayerCharacter::RemoveDefaultInputMappingContext()
{
	const APlayerController* PC = GetController<APlayerController>();
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (const ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultInputMappingContext)
			{
				Subsystem->RemoveMappingContext(DefaultInputMappingContext);
			}
		}
	}
}

void AApecoxPlayerCharacter::HandleAbilityInputTagPressed(const FInputActionValue& ActionValue, FGameplayTag InputTag)
{
	if (InputTag == ApecoxGameplayTags::InputTag_Weapon_Fire && EquipmentComponent && EquipmentComponent->IsArmed())
	{
		CancelEquipPresentation();
		CancelInspect();
		UApecoxRangedWeaponInstance* Weapon = Cast<UApecoxRangedWeaponInstance>(
			EquipmentComponent->GetCurrentWeaponInstance());
		if (Weapon && Weapon->IsReloading())
		{
			return;
		}
		if (Weapon && Weapon->IsMagazineEmpty())
		{
			bWeaponFireInputHeld = false;
			ApplyWeaponFireMovementPriority();
			TryPlayEmptyFireFeedback();
			return;
		}
		bWeaponFireInputHeld = true;
		// 先解决移动状态冲突，再把输入交给GAS，确保首发不因旧冲刺/爬行状态被拒绝。
		ApplyWeaponFireMovementPriority();
	}
	if (CachedAbilitySystemComponent)
	{
		CachedAbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AApecoxPlayerCharacter::HandleAbilityInputTagReleased(const FInputActionValue& ActionValue, FGameplayTag InputTag)
{
	if (InputTag == ApecoxGameplayTags::InputTag_Weapon_Fire)
	{
		bWeaponFireInputHeld = false;
		if (EquipmentComponent)
		{
			if (UApecoxRangedWeaponInstance* Weapon =
				Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()))
			{
				// 每次松键只推进本地Burst身份；Authority会从下一发TargetData识别新Burst，
				// 不需要另发一个可能与最后一发跨Actor乱序的RPC。
				Weapon->EndLocalFireBurst();
			}
		}
	}
	if (CachedAbilitySystemComponent)
	{
		CachedAbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

// --- Native 输入处理（第一/第三人称共用） ---
// 角色 Yaw 跟随 Controller Yaw，因此 Actor Forward 即第一人称"正前方"。
// 移动输入不做死亡判断——死亡后 CMC 已被 OnDeathStarted 禁用，
// AddMovementInput 不会产生实际移动。

void AApecoxPlayerCharacter::HandleMoveInput(const FInputActionValue& ActionValue)
{
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();
	if (MovementVector.IsNearlyZero() || (bIsProne && bWeaponFireInputHeld))
	{
		return;
	}

	if (AController* MyController = GetController())
	{
		// 取 Controller Yaw 构建世界方向（忽略 Pitch/Roll，二维移动不需要）
		const FRotator YawRotation(0.0f, MyController->GetControlRotation().Yaw, 0.0f);
		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Forward, MovementVector.Y);
		AddMovementInput(Right, MovementVector.X);
	}
}

void AApecoxPlayerCharacter::HandleLookInput(const FInputActionValue& ActionValue)
{
	const FVector2D LookVector = ActionValue.Get<FVector2D>();

	// Character 只转交二维观察意图，不在此处理灵敏度和正负约定。
	// 为什么观察由 Controller 管理：
	// - 灵敏度是本地玩家偏好，应跨 Pawn 重生保持一致。
	// - PlayerController 是本地表现入口，拥有 Yaw/Pitch Input 和 CameraManager。
	// - Character 不复制灵敏度、不发送网络 RPC、不越权操作 ControlRotation。
	AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(GetController());
	if (PC)
	{
		const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
		const float AimYawMultiplier = Presentation ? Presentation->AimYawSensitivityMultiplier : 0.5f;
		const float AimPitchMultiplier = Presentation ? Presentation->AimPitchSensitivityMultiplier : 0.5f;
		PC->AddMouseLookInput(LookVector, FVector2D(
			FMath::Lerp(1.0f, AimYawMultiplier, AimAlpha),
			FMath::Lerp(1.0f, AimPitchMultiplier, AimAlpha)));
	}
}

void AApecoxPlayerCharacter::HandleJumpStarted(const FInputActionValue& ActionValue)
{
	if (bIsProne)
	{
		// 趴姿的这次跳跃输入只用于起身：成功和低顶失败都不能继续触发 Jump。
		StopJumping();
		SetProne(false);
		return;
	}

	Jump();
}

void AApecoxPlayerCharacter::HandleJumpCompleted(const FInputActionValue& ActionValue)
{
	StopJumping();
}

void AApecoxPlayerCharacter::HandleCrouchStarted(const FInputActionValue& ActionValue)
{
	CancelEquipPresentation();
	// 当前检视动作只与站立/普通移动基础姿势兼容；先收口双Montage和声音再进入蹲姿。
	CancelInspect();
	bSprintInputHeld = false;
	bIsSprinting = false;

	const bool bChangingFromProneToCrouch = bIsProne;
	if (bChangingFromProneToCrouch && !SetProne(false))
	{
		return;
	}
	// SetProne(false)已经播放一次起身布料声；随后实际进入蹲伏时抑制第二次声音。
	bSuppressNextCrouchStartSound = bChangingFromProneToCrouch;

	Crouch();
	RefreshMovementSpeed();
}

void AApecoxPlayerCharacter::HandleCrouchCompleted(const FInputActionValue& ActionValue)
{
	UnCrouch();
	RefreshMovementSpeed();
}

void AApecoxPlayerCharacter::HandleSprintStarted(const FInputActionValue& ActionValue)
{
	if (IsReloading())
	{
		return;
	}
	CancelEquipPresentation();
	CancelInspect();
	// 冲刺输入优先结束 ADS；之后必须重新按下瞄准键才会进入 ADS。
	bAimInputRequested = false;
	bIsAiming = false;
	bSprintInputHeld = true;
	RefreshMovementSpeed();
}

void AApecoxPlayerCharacter::HandleSprintCompleted(const FInputActionValue& ActionValue)
{
	bSprintInputHeld = false;
	bIsSprinting = false;
	RefreshMovementSpeed();
}

void AApecoxPlayerCharacter::HandleProneStarted(const FInputActionValue& ActionValue)
{
	CancelEquipPresentation();
	// 趴姿没有匹配的检视轨迹，姿态输入优先并中断当前检视。
	CancelInspect();
	SetProne(!bIsProne);
}

void AApecoxPlayerCharacter::HandleAimStarted(const FInputActionValue& ActionValue)
{
	if (bAimInputRequested)
	{
		bAimInputRequested = false;
		bIsAiming = false;
		return;
	}
	if (IsReloading())
	{
		return;
	}
	CancelEquipPresentation();
	CancelInspect();
	// ADS 可以打断冲刺，并消费当前冲刺按住意图，避免瞄准后立刻又自动冲刺。
	bSprintInputHeld = false;
	bIsSprinting = false;
	RefreshMovementSpeed();
	bAimInputRequested = true;
	bIsAiming = CanAim();
}

void AApecoxPlayerCharacter::HandleInspectStarted(const FInputActionValue& ActionValue)
{
	if (!CanInspect() || !EquipmentComponent)
	{
		return;
	}

	// 播放前先立刻撤掉左手IK；若任一必要Montage启动失败则恢复普通握持状态。
	bIsInspecting = true;
	if (!EquipmentComponent->PlayInspectPresentation())
	{
		bIsInspecting = false;
	}
}

void AApecoxPlayerCharacter::HandleReloadStarted(const FInputActionValue& ActionValue)
{
	UApecoxRangedWeaponInstance* Weapon = EquipmentComponent
		? Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()) : nullptr;
	if (!Weapon || !Weapon->CanReload() || (HealthComponent && HealthComponent->IsDeadOrDying()))
	{
		return;
	}

	// RAR 的 Reload 会结束 Fire/Run/Inspect；这里在发 RPC 前先收口本地输入和表现。
	CancelEquipPresentation();
	CancelInspect();
	bWeaponFireInputHeld = false;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bAimInputRequested = false;
	bIsAiming = false;
	RefreshMovementSpeed();
	if (CachedAbilitySystemComponent)
	{
		CachedAbilitySystemComponent->AbilityInputTagReleased(ApecoxGameplayTags::InputTag_Weapon_Fire);
	}
	Weapon->EndLocalFireBurst();

	if (HasAuthority())
	{
		TryStartWeaponReloadAuthority();
	}
	else
	{
		ServerRequestReload();
	}
}

void AApecoxPlayerCharacter::HandleLaserToggleStarted(const FInputActionValue& ActionValue)
{
	if (EquipmentComponent && CanUseWeaponLaser())
	{
		EquipmentComponent->ToggleLaserPresentation();
	}
}

void AApecoxPlayerCharacter::ServerRequestReload_Implementation()
{
	TryStartWeaponReloadAuthority();
}

bool AApecoxPlayerCharacter::TryStartWeaponReloadAuthority()
{
	if (!HasAuthority() || !EquipmentComponent || (HealthComponent && HealthComponent->IsDeadOrDying()))
	{
		return false;
	}
	UApecoxRangedWeaponInstance* Weapon = Cast<UApecoxRangedWeaponInstance>(
		EquipmentComponent->GetCurrentWeaponInstance());
	if (!Weapon || !Weapon->BeginReload())
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(ReloadCommitTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadFinishTimerHandle);
	ReloadingWeapon = Weapon;
	const UApecoxWeaponDefinition* WeaponDef = Weapon->GetRangedWeaponDefinition();
	const bool bEmptyReload = Weapon->GetReloadState() == EApecoxWeaponReloadState::Empty;
	const float CommitTime = bEmptyReload
		? WeaponDef->ReloadConfig.EmptyReloadCommitTime
		: WeaponDef->ReloadConfig.TacticalReloadCommitTime;
	const float FinishTime = bEmptyReload
		? WeaponDef->ReloadConfig.EmptyReloadDuration
		: WeaponDef->ReloadConfig.TacticalReloadDuration;

	if (CommitTime <= UE_SMALL_NUMBER)
	{
		CommitWeaponReload();
	}
	else
	{
		GetWorldTimerManager().SetTimer(ReloadCommitTimerHandle, this,
			&AApecoxPlayerCharacter::CommitWeaponReload, CommitTime, false);
	}
	GetWorldTimerManager().SetTimer(ReloadFinishTimerHandle, this,
		&AApecoxPlayerCharacter::FinishWeaponReload, FinishTime, false);
	UpdateWeaponReloadState();
	return true;
}

void AApecoxPlayerCharacter::CommitWeaponReload()
{
	UApecoxRangedWeaponInstance* Weapon = ReloadingWeapon.Get();
	if (!HasAuthority() || !Weapon || !EquipmentComponent
		|| EquipmentComponent->GetCurrentWeaponInstance() != Weapon)
	{
		CancelWeaponReload();
		return;
	}
	Weapon->CommitReload();
}

void AApecoxPlayerCharacter::FinishWeaponReload()
{
	UApecoxRangedWeaponInstance* Weapon = ReloadingWeapon.Get();
	GetWorldTimerManager().ClearTimer(ReloadCommitTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadFinishTimerHandle);
	if (HasAuthority() && Weapon)
	{
		// 极端帧时序下完成计时可能与提交同帧，重复提交由 WeaponInstance 自身拒绝。
		Weapon->CommitReload();
		Weapon->FinishReload();
	}
	ReloadingWeapon = nullptr;
	UpdateWeaponReloadState();
}

void AApecoxPlayerCharacter::CancelWeaponReload()
{
	GetWorldTimerManager().ClearTimer(ReloadCommitTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadFinishTimerHandle);
	if (HasAuthority())
	{
		if (UApecoxRangedWeaponInstance* Weapon = ReloadingWeapon.Get())
		{
			Weapon->FinishReload();
		}
	}
	ReloadingWeapon = nullptr;
	if (EquipmentComponent)
	{
		if (HasAuthority())
		{
			EquipmentComponent->SetReloadPresentationState(0);
		}
		else
		{
			const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
			EquipmentComponent->StopReloadPresentation(
				Presentation ? Presentation->ReloadInterruptBlendOutTime : 0.1f);
		}
	}
	ObservedReloadState = 0;
}

void AApecoxPlayerCharacter::UpdateWeaponReloadState()
{
	// 私有 WeaponInstance 与 ReloadState 都是 OwnerOnly。只有服务器读取该真相，
	// 再由 EquipmentComponent 的紧凑公开状态驱动 Owner 与所有 Simulated Proxy 的表现。
	if (!HasAuthority())
	{
		return;
	}

	UApecoxRangedWeaponInstance* Weapon = EquipmentComponent
		? Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()) : nullptr;
	if (ReloadingWeapon.IsValid() && Weapon != ReloadingWeapon.Get())
	{
		CancelWeaponReload();
		return;
	}
	const uint8 CurrentState = Weapon ? static_cast<uint8>(Weapon->GetReloadState()) : 0;
	if (CurrentState == ObservedReloadState)
	{
		return;
	}
	ObservedReloadState = CurrentState;
	if (EquipmentComponent)
	{
		EquipmentComponent->SetReloadPresentationState(CurrentState);
	}
}

void AApecoxPlayerCharacter::TryPlayEmptyFireFeedback()
{
	UApecoxRangedWeaponInstance* Weapon = EquipmentComponent
		? Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()) : nullptr;
	const UApecoxWeaponDefinition* WeaponDef = Weapon ? Weapon->GetRangedWeaponDefinition() : nullptr;
	const UWorld* World = GetWorld();
	if (!Weapon || !Weapon->IsMagazineEmpty() || Weapon->IsReloading() || !WeaponDef || !World)
	{
		return;
	}
	const float Now = World->GetTimeSeconds();
	if (Now - LastEmptyFireFeedbackTime < WeaponDef->ReloadConfig.EmptyFireInterval)
	{
		return;
	}
	LastEmptyFireFeedbackTime = Now;
	if (EquipmentComponent)
	{
		EquipmentComponent->PlayEmptyFirePresentation();
	}
}

bool AApecoxPlayerCharacter::CanInspect() const
{
	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	return !bIsInspecting && !bIsAiming && !bIsSprinting && !bWeaponFireInputHeld && !IsReloading()
		&& !bIsProne && CMC && !CMC->IsCrouching()
		&& IsLocallyControlled() && EquipmentComponent && EquipmentComponent->IsArmed()
		&& !EquipmentComponent->IsEquipPresentationActive()
		&& (!HealthComponent || !HealthComponent->IsDeadOrDying())
		&& Presentation && Presentation->FirstPersonArmsInspectMontage
		&& Presentation->FirstPersonWeaponInspectMontage;
}

void AApecoxPlayerCharacter::CancelEquipPresentation()
{
	if (!EquipmentComponent || !EquipmentComponent->IsEquipPresentationActive())
	{
		return;
	}

	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	const float BlendOutTime = Presentation ? Presentation->EquipInterruptBlendOutTime : 0.1f;
	EquipmentComponent->StopEquipPresentation(BlendOutTime);
}

void AApecoxPlayerCharacter::CancelInspect()
{
	if (!bIsInspecting)
	{
		return;
	}

	bIsInspecting = false;
	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	const float BlendOutTime = Presentation ? Presentation->InspectInterruptBlendOutTime : 0.1f;
	if (EquipmentComponent)
	{
		EquipmentComponent->StopInspectPresentation(BlendOutTime);
	}
}

void AApecoxPlayerCharacter::OnInspectPresentationEnded(bool bInterrupted)
{
	bIsInspecting = false;
}

void AApecoxPlayerCharacter::HandleLeanLeftStarted(const FInputActionValue& ActionValue)
{
	bLeanLeftInputHeld = true;
}

void AApecoxPlayerCharacter::HandleLeanLeftCompleted(const FInputActionValue& ActionValue)
{
	bLeanLeftInputHeld = false;
}

void AApecoxPlayerCharacter::HandleLeanRightStarted(const FInputActionValue& ActionValue)
{
	bLeanRightInputHeld = true;
}

void AApecoxPlayerCharacter::HandleLeanRightCompleted(const FInputActionValue& ActionValue)
{
	bLeanRightInputHeld = false;
}

void AApecoxPlayerCharacter::UpdateLocomotionState(float DeltaSeconds)
{
	if (bWeaponFireInputHeld && (!EquipmentComponent || !EquipmentComponent->IsArmed()))
	{
		bWeaponFireInputHeld = false;
	}
	if (!EquipmentComponent || !EquipmentComponent->IsArmed())
	{
		CancelInspect();
		bAimInputRequested = false;
		bIsAiming = false;
	}
	const bool bShouldSprint = bSprintInputHeld && CanSprint();
	if (bIsSprinting != bShouldSprint)
	{
		bIsSprinting = bShouldSprint;
		RefreshMovementSpeed();
	}

	UpdateFirstPersonCamera(DeltaSeconds);
}

void AApecoxPlayerCharacter::RefreshMovementSpeed()
{
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->MaxWalkSpeed = bIsProne ? ProneSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);
		CMC->MaxWalkSpeedCrouched = CrouchedSpeed;
	}
}

bool AApecoxPlayerCharacter::CanSprint() const
{
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC || bWeaponFireInputHeld || bIsAiming || IsReloading()
		|| bIsProne || CMC->IsCrouching() || !CMC->IsMovingOnGround())
	{
		return false;
	}

	// GAS在Controller输入末尾处理，可能早于本帧CMC更新，先看待消费输入。
	const FVector PendingInput = GetPendingMovementInputVector();
	const FVector CurrentAcceleration = !PendingInput.IsNearlyZero()
		? PendingInput : CMC->GetCurrentAcceleration();
	const FVector Acceleration2D(CurrentAcceleration.X, CurrentAcceleration.Y, 0.0f);
	return !Acceleration2D.IsNearlyZero()
		&& FVector::DotProduct(Acceleration2D.GetSafeNormal(), GetActorForwardVector()) > 0.5f;
}

const UApecoxWeaponPresentationDefinition* AApecoxPlayerCharacter::GetEquippedPresentationDefinition() const
{
	const UApecoxWeaponDefinition* WeaponDefinition = EquipmentComponent
		? EquipmentComponent->GetEquippedWeaponDefinition() : nullptr;
	return WeaponDefinition ? WeaponDefinition->PresentationDefinition.Get() : nullptr;
}

bool AApecoxPlayerCharacter::CanAim() const
{
	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	return Presentation && Presentation->bSupportsAim && EquipmentComponent && EquipmentComponent->IsArmed()
		&& (!HealthComponent || !HealthComponent->IsDeadOrDying()) && !bIsSprinting && !IsReloading();
}

bool AApecoxPlayerCharacter::CanUseWeaponLaser() const
{
	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	return IsLocallyControlled() && Presentation && Presentation->bSupportsLaser
		&& EquipmentComponent && EquipmentComponent->IsArmed()
		&& (!HealthComponent || !HealthComponent->IsDeadOrDying())
		&& !bIsSprinting && !bIsInspecting && !IsReloading()
		&& !EquipmentComponent->IsEquipPresentationActive();
}

bool AApecoxPlayerCharacter::IsReloading() const
{
	const UApecoxRangedWeaponInstance* Weapon = EquipmentComponent
		? Cast<UApecoxRangedWeaponInstance>(EquipmentComponent->GetCurrentWeaponInstance()) : nullptr;
	return Weapon && Weapon->IsReloading();
}

void AApecoxPlayerCharacter::UpdateAimState(float DeltaSeconds)
{
	bIsAiming = bAimInputRequested && CanAim();
	const UApecoxWeaponPresentationDefinition* Presentation = GetEquippedPresentationDefinition();
	const float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	const float BlendTime = Presentation ? Presentation->AimBlendTime : 0.2f;
	AimAlpha = BlendTime <= UE_SMALL_NUMBER
		? TargetAlpha
		: FMath::FInterpConstantTo(AimAlpha, TargetAlpha, FMath::Max(DeltaSeconds, 0.0f), 1.0f / BlendTime);

	if (!FirstPersonCamera || !IsLocallyControlled())
	{
		return;
	}

	const float CameraMultiplier = Presentation
		? FMath::Clamp(Presentation->AimCameraFieldOfViewMultiplier, 0.1f, 1.0f) : 1.0f;
	const float AimedCameraFOV = BaseCameraFieldOfView * CameraMultiplier;
	const float AimedViewmodelFOV = Presentation
		? Presentation->AimFirstPersonFieldOfView : BaseFirstPersonFieldOfView;
	FirstPersonCamera->SetFieldOfView(FMath::Lerp(BaseCameraFieldOfView, AimedCameraFOV, AimAlpha));
	FirstPersonCamera->SetFirstPersonFieldOfView(
		FMath::Lerp(BaseFirstPersonFieldOfView, AimedViewmodelFOV, AimAlpha));
}

void AApecoxPlayerCharacter::ResetAimState()
{
	bAimInputRequested = false;
	bIsAiming = false;
	AimAlpha = 0.0f;
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(BaseCameraFieldOfView);
		FirstPersonCamera->SetFirstPersonFieldOfView(BaseFirstPersonFieldOfView);
	}
}

void AApecoxPlayerCharacter::ApplyWeaponFireMovementPriority()
{
	bIsSprinting = false;
	RefreshMovementSpeed();
	if (bIsProne)
	{
		// 同帧的移动回调可能先于开火回调：清掉待消费输入与刹停速度，首发即可在静止趴姿成立。
		ConsumeMovementInputVector();
		if (UCharacterMovementComponent* CMC = GetCharacterMovement())
		{
			CMC->StopMovementImmediately();
		}
	}
}

bool AApecoxPlayerCharacter::IsWeaponFireBlockedByMovement() const
{
	if (IsReloading())
	{
		return true;
	}
	if (bIsSprinting || (bSprintInputHeld && CanSprint()))
	{
		return true;
	}

	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	// 同时覆盖起步意图和松键后的刹停，避免只看动画或旧状态漏出一发。
	constexpr double ProneStillSpeed = 1.0; // cm/s，过滤静止时数值噪声
	return bIsProne && (GetVelocity().SizeSquared2D() > FMath::Square(ProneStillSpeed)
		|| GetPendingMovementInputVector().SizeSquared2D() > UE_KINDA_SMALL_NUMBER
		|| (CMC && CMC->GetCurrentAcceleration().SizeSquared2D() > UE_KINDA_SMALL_NUMBER));
}

void AApecoxPlayerCharacter::UpdateFirstPersonLookSway(float DeltaSeconds)
{
	if (!FirstPersonMesh)
	{
		return;
	}

	AController* CurrentController = GetController();
	const FRotator ControlRotation = GetControlRotation();
	const bool bCanSway = bEnableFirstPersonLookSway && IsLocallyControlled()
		&& CurrentController && EquipmentComponent && EquipmentComponent->IsArmed();
	const FRotator LookDelta = (ControlRotation - PreviousLookSwayControlRotation).GetNormalized();
	const bool bReset = !bCanSway || !bLookSwayActive || LookSwayController.Get() != CurrentController
		|| DeltaSeconds > 0.1f || FMath::Abs(LookDelta.Yaw) > 45.0f || FMath::Abs(LookDelta.Pitch) > 45.0f;
	PreviousLookSwayControlRotation = ControlRotation;
	LookSwayController = CurrentController;
	if (bReset)
	{
		LookSwayAngles = FVector::ZeroVector;
		LookSwayAngularVelocity = FVector::ZeroVector;
		LookSwayLocation = FVector::ZeroVector;
		LookSwayLocationVelocity = FVector::ZeroVector;
		bLookSwayActive = bCanSway;
		FirstPersonMesh->SetRelativeLocationAndRotation(BaseFirstPersonMeshRelativeTransform.GetLocation(),
			BaseFirstPersonMeshRelativeTransform.GetRotation());
		return;
	}
	if (DeltaSeconds <= UE_SMALL_NUMBER)
	{
		return;
	}

	// 输入用角速度而非每帧角度，保证相同转向速度在不同帧率下幅度一致。
	const double ReferenceDelta = FMath::Max(LookSwayReferenceTurnSpeed, 1.0f) * DeltaSeconds;
	const double PitchInput = FMath::Clamp(LookDelta.Pitch / ReferenceDelta, -1.0, 1.0);
	const double YawInput = FMath::Clamp(LookDelta.Yaw / ReferenceDelta, -1.0, 1.0);
	const FVector TargetAngles(-PitchInput * FMath::Abs(LookSwayMaxRotation.Pitch),
		-YawInput * FMath::Abs(LookSwayMaxRotation.Yaw),
		-YawInput * FMath::Abs(LookSwayMaxRotation.Roll));
	const FVector TargetLocation(-FMath::Abs(YawInput) * FMath::Abs(LookSwayMaxLocation.X),
		-YawInput * FMath::Abs(LookSwayMaxLocation.Y),
		-PitchInput * FMath::Abs(LookSwayMaxLocation.Z));
	const int32 Substeps = FMath::Max(1, FMath::CeilToInt(DeltaSeconds * 120.0f));
	for (int32 Step = 0; Step < Substeps; ++Step)
	{
		FMath::SpringDamperSmoothing(LookSwayAngles, LookSwayAngularVelocity, TargetAngles,
			FVector::ZeroVector, DeltaSeconds / Substeps, FMath::Max(LookSwaySmoothingTime, 0.02f), 1.0f);
		FMath::SpringDamperSmoothing(LookSwayLocation, LookSwayLocationVelocity, TargetLocation,
			FVector::ZeroVector, DeltaSeconds / Substeps, FMath::Max(LookSwaySmoothingTime, 0.02f), 1.0f);
	}

	// ADS 过程中平滑撤掉 Look Sway；完全开镜时精确为零。瞄具红点和汇聚到相机中心的
	// 镭射落点因此使用同一屏幕参考，不会在鼠标转向时产生动态分离。内部弹簧继续更新，
	// 退出 ADS 时随 AimAlpha 平滑恢复，不产生一次性的积累跳变。
	const double SwayOutputScale = 1.0 - FMath::Clamp(static_cast<double>(AimAlpha), 0.0, 1.0);
	const FVector AppliedSwayAngles = LookSwayAngles * SwayOutputScale;
	const FVector AppliedSwayLocation = LookSwayLocation * SwayOutputScale;

	// 在镜头空间绕原点旋转完整手臂，不能绕骨架脚底转；枪随现有挂点共同变化。
	// 每次从作者基线重建，不累积Transform，也不写相机或ControlRotation。
	const FQuat SwayRotation = FRotator(
		AppliedSwayAngles.X, AppliedSwayAngles.Y, AppliedSwayAngles.Z).Quaternion();
	FirstPersonMesh->SetRelativeLocationAndRotation(
		SwayRotation.RotateVector(BaseFirstPersonMeshRelativeTransform.GetLocation()) + AppliedSwayLocation,
		SwayRotation * BaseFirstPersonMeshRelativeTransform.GetRotation());
}

FRotator AApecoxPlayerCharacter::GetFirstPersonWeaponRecoilRotation() const
{
	// RAR的Vector To Rotator函数：X->Roll、Y->Pitch、Z->Yaw。
	return FRotator(CurrentWeaponRecoilRotation.Y, CurrentWeaponRecoilRotation.Z,
		CurrentWeaponRecoilRotation.X);
}

void AApecoxPlayerCharacter::NotifyLocalWeaponShot(const UApecoxRangedWeaponInstance* Weapon,
	int32 BurstShotIndex)
{
	if (!Weapon || !IsLocallyControlled() || !EquipmentComponent
		|| EquipmentComponent->GetCurrentWeaponInstance() != Weapon)
	{
		return;
	}

	const FApecoxRangedFireConfig* Config = Weapon->GetRangedFireConfig();
	if (!Config || !Config->IsValidRangedConfig())
	{
		return;
	}

	if (RecoilWeapon.IsValid() && RecoilWeapon.Get() != Weapon)
	{
		ResetFirstPersonRecoil();
	}
	RecoilWeapon = Weapon;
	RecoilController = GetController();

	// 与本发 TargetData 使用同一 ADS 状态，切换到RAR预留的Aiming曲线与倍率。
	const bool bShotWasAimed = bIsAiming;
	FVector EvaluatedCameraRecoilTarget = FVector::ZeroVector;
	Config->EvaluateRecoilTargets(BurstShotIndex, bShotWasAimed, TargetWeaponRecoilLocation,
		TargetWeaponRecoilRotation, EvaluatedCameraRecoilTarget);
	StartCameraRecoilKick(EvaluatedCameraRecoilTarget, BurstShotIndex == 0);

	const FApecoxRecoilSpringSettings& LocationSpring = bShotWasAimed
		? Config->AimingWeaponRecoilLocationSpring : Config->StandingWeaponRecoilLocationSpring;
	const FApecoxRecoilSpringSettings& RotationSpring = bShotWasAimed
		? Config->AimingWeaponRecoilRotationSpring : Config->StandingWeaponRecoilRotationSpring;
	WeaponRecoilLocationSpringParameters = FVector(LocationSpring.Stiffness,
		LocationSpring.CriticalDampingFactor, LocationSpring.Mass);
	WeaponRecoilRotationSpringParameters = FVector(RotationSpring.Stiffness,
		RotationSpring.CriticalDampingFactor, RotationSpring.Mass);
	CameraRecoilRotationSpringParameters = FVector(Config->CameraRecoilRotationSpring.Stiffness,
		Config->CameraRecoilRotationSpring.CriticalDampingFactor, Config->CameraRecoilRotationSpring.Mass);
}

void AApecoxPlayerCharacter::StartCameraRecoilKick(const FVector& NewTarget, bool bStartsNewBurst)
{
	if (bStartsNewBurst)
	{
		// 每轮连发的RAR曲线都从Shot 1重新采样。真实ControlRotation已经保存上一轮后坐和玩家压枪的结果，
		// 因此这里只清空上一轮仍在静默恢复的内部坐标；否则旧值高于新一轮首发目标时，
		// 弹簧会产生负增量并令准星在快速重新开火时向下跳。
		TargetCameraRecoilRotation = FVector::ZeroVector;
		CurrentCameraRecoilRotation = FVector::ZeroVector;
		AppliedCameraRecoilRotation = FVector::ZeroVector;
		CameraRecoilRotationSpringState.Reset();
	}

	TargetCameraRecoilRotation = NewTarget;
	bCameraRecoilKickActive = true;
}

void AApecoxPlayerCharacter::UpdateFirstPersonRecoil(float DeltaSeconds)
{
	AController* CurrentController = GetController();
	if (!IsLocallyControlled() || !CurrentController || DeltaSeconds <= UE_SMALL_NUMBER)
	{
		ResetFirstPersonRecoil();
		return;
	}
	if (RecoilController.IsValid() && RecoilController.Get() != CurrentController)
	{
		ResetFirstPersonRecoil();
	}
	RecoilController = CurrentController;

	const UApecoxRangedWeaponInstance* Weapon = RecoilWeapon.Get();
	const bool bKeepShotTarget = Weapon && bWeaponFireInputHeld && !Weapon->IsMagazineEmpty()
		&& EquipmentComponent && EquipmentComponent->GetCurrentWeaponInstance() == Weapon;
	if (!bKeepShotTarget)
	{
		TargetWeaponRecoilLocation = FVector::ZeroVector;
		TargetWeaponRecoilRotation = FVector::ZeroVector;
		// 最后一发打空或玩家松键时，先完成本发抬枪；随后只让内部表现弹簧归零。
		if (!bCameraRecoilKickActive)
		{
			TargetCameraRecoilRotation = FVector::ZeroVector;
		}
	}

	// 小质量弹簧在长帧中分步求解，保留RAR参数同时避免一次大步导致数值爆炸。
	const float SimulatedDelta = FMath::Min(DeltaSeconds, 0.1f);
	const int32 Substeps = FMath::Max(1, FMath::CeilToInt(SimulatedDelta * 120.0f));
	const float StepDelta = SimulatedDelta / static_cast<float>(Substeps);
	for (int32 Step = 0; Step < Substeps; ++Step)
	{
		CurrentWeaponRecoilLocation = UKismetMathLibrary::VectorSpringInterp(
			CurrentWeaponRecoilLocation, TargetWeaponRecoilLocation, WeaponRecoilLocationSpringState,
			WeaponRecoilLocationSpringParameters.X, WeaponRecoilLocationSpringParameters.Y, StepDelta,
			WeaponRecoilLocationSpringParameters.Z, 0.0f, false);
		CurrentWeaponRecoilRotation = UKismetMathLibrary::VectorSpringInterp(
			CurrentWeaponRecoilRotation, TargetWeaponRecoilRotation, WeaponRecoilRotationSpringState,
			WeaponRecoilRotationSpringParameters.X, WeaponRecoilRotationSpringParameters.Y, StepDelta,
			WeaponRecoilRotationSpringParameters.Z, 0.0f, false);
		CurrentCameraRecoilRotation = UKismetMathLibrary::VectorSpringInterp(
			CurrentCameraRecoilRotation, TargetCameraRecoilRotation, CameraRecoilRotationSpringState,
			CameraRecoilRotationSpringParameters.X, CameraRecoilRotationSpringParameters.Y, StepDelta,
			CameraRecoilRotationSpringParameters.Z, 0.0f, false);
	}

	if (!bKeepShotTarget && CurrentWeaponRecoilLocation.IsNearlyZero(0.001)
		&& WeaponRecoilLocationSpringState.Velocity.IsNearlyZero(0.001))
	{
		CurrentWeaponRecoilLocation = FVector::ZeroVector;
		WeaponRecoilLocationSpringState.Reset();
	}
	if (!bKeepShotTarget && CurrentWeaponRecoilRotation.IsNearlyZero(0.001)
		&& WeaponRecoilRotationSpringState.Velocity.IsNearlyZero(0.001))
	{
		CurrentWeaponRecoilRotation = FVector::ZeroVector;
		WeaponRecoilRotationSpringState.Reset();
	}
	if (!bKeepShotTarget && CurrentCameraRecoilRotation.IsNearlyZero(0.001)
		&& CameraRecoilRotationSpringState.Velocity.IsNearlyZero(0.001))
	{
		CurrentCameraRecoilRotation = FVector::ZeroVector;
		CameraRecoilRotationSpringState.Reset();
	}

	if (bCameraRecoilKickActive)
	{
		const FVector CameraKickDelta = CurrentCameraRecoilRotation - AppliedCameraRecoilRotation;
		const FRotator CameraDelta(CameraKickDelta.Y, CameraKickDelta.Z, 0.0f);
		if (!CameraDelta.IsNearlyZero())
		{
			CurrentController->SetControlRotation(
				(CurrentController->GetControlRotation() + CameraDelta).GetNormalized());
			// 程序后坐已经作用于真实视线；同步Look Sway基准，避免下一帧把它当作鼠标输入再摆一次。
			if (LookSwayController.Get() == CurrentController)
			{
				PreviousLookSwayControlRotation =
					(PreviousLookSwayControlRotation + CameraDelta).GetNormalized();
			}
		}
	}

	if (bCameraRecoilKickActive
		&& CurrentCameraRecoilRotation.Equals(TargetCameraRecoilRotation, 0.001)
		&& CameraRecoilRotationSpringState.Velocity.IsNearlyZero(0.001))
	{
		bCameraRecoilKickActive = false;
		if (!bKeepShotTarget)
		{
			TargetCameraRecoilRotation = FVector::ZeroVector;
		}
	}
	// Applied始终跟随内部弹簧。恢复差值不写回ControlRotation，瞄准方向保持在停火位置。
	AppliedCameraRecoilRotation = CurrentCameraRecoilRotation;

	if (!bKeepShotTarget && CurrentWeaponRecoilLocation.IsZero()
		&& CurrentWeaponRecoilRotation.IsZero() && CurrentCameraRecoilRotation.IsZero())
	{
		RecoilWeapon.Reset();
	}
}

void AApecoxPlayerCharacter::ResetFirstPersonRecoil()
{
	TargetWeaponRecoilLocation = CurrentWeaponRecoilLocation = FVector::ZeroVector;
	TargetWeaponRecoilRotation = CurrentWeaponRecoilRotation = FVector::ZeroVector;
	TargetCameraRecoilRotation = CurrentCameraRecoilRotation = FVector::ZeroVector;
	AppliedCameraRecoilRotation = FVector::ZeroVector;
	bCameraRecoilKickActive = false;
	WeaponRecoilLocationSpringState.Reset();
	WeaponRecoilRotationSpringState.Reset();
	CameraRecoilRotationSpringState.Reset();
	RecoilWeapon.Reset();
	RecoilController.Reset();
}

bool AApecoxPlayerCharacter::SetProne(bool bNewProne)
{
	if (bIsProne == bNewProne)
	{
		return true;
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!Capsule || !CMC)
	{
		return false;
	}

	if (bNewProne)
	{
		if (!CMC->IsMovingOnGround())
		{
			return false;
		}

		bSprintInputHeld = false;
		bIsSprinting = false;
		const bool bWasCrouched = IsCrouched();
		const float PreviousHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();

		// Character::UnCrouch 只撤销蹲伏意图，不会立即结束原生蹲伏状态。
		// 入趴由这里接管胶囊：同时结束状态，避免 CMC 在下一次更新时执行
		// 延后的起身、把刚缩小的胶囊扩回站立。这里不经过站立尺寸，
		// 因此角色在只能蹲伏的低顶空间中也能直接进入更低的趴姿。
		UnCrouch();
		SetIsCrouched(false);

		const float TargetHalfHeight = FMath::Max(ProneCapsuleHalfHeight, Capsule->GetUnscaledCapsuleRadius());
		const float HalfHeightAdjust = PreviousHalfHeight - TargetHalfHeight;
		Capsule->SetCapsuleHalfHeight(TargetHalfHeight, true);
		AddActorWorldOffset(FVector(0.0f, 0.0f, -HalfHeightAdjust), false, nullptr, ETeleportType::TeleportPhysics);
		bIsProne = true;
		if (bWeaponFireInputHeld)
		{
			ApplyWeaponFireMovementPriority();
		}

		if (bWasCrouched)
		{
			// 保留原生蹲伏结束的眼高、Mesh 偏移和 Blueprint 通知收尾；
			// OnEndCrouch 本身不扩张胶囊，通知发生时趴姿已经成立。
			const float CrouchHeightAdjust = StandingCapsuleHalfHeight - PreviousHalfHeight;
			OnEndCrouch(CrouchHeightAdjust, CrouchHeightAdjust * Capsule->GetShapeScale());
		}
		else
		{
			UpdateFirstPersonCamera(0.0f);
		}
		CMC->bForceNextFloorCheck = true;
		RefreshMovementSpeed();
		PlayStanceSound(CrouchStartSound);
		return true;
	}

	if (!CanExitProne(StandingCapsuleHalfHeight))
	{
		return false;
	}

	const float HalfHeightAdjust = StandingCapsuleHalfHeight - Capsule->GetUnscaledCapsuleHalfHeight();
	SetActorLocation(GetActorLocation() + FVector(0.0f, 0.0f, HalfHeightAdjust), false, nullptr,
		ETeleportType::TeleportPhysics);
	Capsule->SetCapsuleHalfHeight(StandingCapsuleHalfHeight, true);
	bIsProne = false;
	UpdateFirstPersonCamera(0.0f);
	RefreshMovementSpeed();
	PlayStanceSound(CrouchStopSound);
	return true;
}

bool AApecoxPlayerCharacter::CanExitProne(float TargetHalfHeight) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UWorld* World = GetWorld();
	if (!Capsule || !World)
	{
		return false;
	}

	const float HalfHeightAdjust = TargetHalfHeight - Capsule->GetUnscaledCapsuleHalfHeight();
	const FVector TargetLocation = GetActorLocation() + FVector(0.0f, 0.0f, HalfHeightAdjust);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ApecoxProneClearance), false, this);
	const FCollisionShape StandingShape = FCollisionShape::MakeCapsule(
		Capsule->GetUnscaledCapsuleRadius(), TargetHalfHeight);

	return !World->OverlapBlockingTestByProfile(
		TargetLocation, FQuat::Identity, Capsule->GetCollisionProfileName(), StandingShape, QueryParams);
}

float AApecoxPlayerCharacter::GetTargetViewHeight() const
{
	if (bIsProne)
	{
		return ProneViewHeight;
	}
	return IsCrouched() ? CrouchedViewHeight
		: StandingCapsuleHalfHeight + static_cast<float>(BaseFirstPersonCameraRelativeLocation.Z);
}

void AApecoxPlayerCharacter::UpdateFirstPersonCamera(float DeltaSeconds)
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!bStanceCameraInitialized || !FirstPersonCamera || !Capsule)
	{
		return;
	}

	// 只平滑相对胶囊底部的姿态眼高；正常移动、跳跃、传送仍立即跟随角色。
	// 指数衰减在不同帧率下具有一致的响应，反向切换直接从当前高度继续。
	const float BlendAlpha = StanceCameraInterpSpeed <= 0.0f ? 1.0f
		: 1.0f - FMath::Exp(-StanceCameraInterpSpeed * FMath::Max(DeltaSeconds, 0.0f));
	CurrentViewHeight = FMath::Lerp(CurrentViewHeight, GetTargetViewHeight(), BlendAlpha);
	FVector CameraBaseLocation = BaseFirstPersonCameraRelativeLocation;
	CameraBaseLocation.Z = CurrentViewHeight - Capsule->GetUnscaledCapsuleHalfHeight();
	CameraBaseLocation = GetCollisionLimitedCameraBase(CameraBaseLocation);
	// 低顶限制优先于平滑；从实际受限高度继续，离开障碍时不会突然弹回隐藏的目标。
	CurrentViewHeight = static_cast<float>(CameraBaseLocation.Z) + Capsule->GetUnscaledCapsuleHalfHeight();

	float DesiredLeanAmount = static_cast<float>(bLeanRightInputHeld) - static_cast<float>(bLeanLeftInputHeld);
	const UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (bIsProne || bIsSprinting || (CMC && CMC->IsFalling()))
	{
		DesiredLeanAmount = 0.0f;
	}

	DesiredLeanAmount = GetCollisionLimitedLeanTarget(DesiredLeanAmount, CameraBaseLocation);
	CurrentLeanAmount = FMath::FInterpTo(CurrentLeanAmount, DesiredLeanAmount, DeltaSeconds, LeanInterpSpeed);
	// 角色已探头时也可能移动到墙边。不能只限制目标后慢慢退回，否则插值期间仍会穿墙。
	CurrentLeanAmount = GetCollisionLimitedLeanTarget(CurrentLeanAmount, CameraBaseLocation);

	// RAR的Camera Socket在Lean Pose中保持不动；实际效果由完整上半身Pose与独立镜头侧移/Roll叠加。
	FirstPersonCamera->SetRelativeLocation(
		CameraBaseLocation + FVector(0.0f, CurrentLeanAmount * MaxLeanDistance, 0.0f));
	FirstPersonCamera->SetRelativeRotation(
		BaseFirstPersonCameraRelativeRotation + FRotator(0.0f, 0.0f, CurrentLeanAmount * MaxLeanAngle));
}

FVector AApecoxPlayerCharacter::GetCollisionLimitedCameraBase(const FVector& CameraBaseLocation) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const USceneComponent* CameraParent = FirstPersonCamera ? FirstPersonCamera->GetAttachParent() : nullptr;
	if (!Capsule || !CameraParent || !GetWorld())
	{
		return CameraBaseLocation;
	}

	// 起点位于胶囊底部内接球中心，入趴后仍在碰撞体内。
	// 从这里向实际视点探测，可限制低顶下尚未降完的镜头，也覆盖配置的 XY 偏移。
	const FVector SafeLocalLocation(0.0f, 0.0f,
		-Capsule->GetUnscaledCapsuleHalfHeight() + Capsule->GetUnscaledCapsuleRadius());
	const FTransform& ParentTransform = CameraParent->GetComponentTransform();
	const FVector Start = ParentTransform.TransformPosition(SafeLocalLocation);
	const FVector Target = ParentTransform.TransformPosition(CameraBaseLocation);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ApecoxStanceCameraProbe), false, this);
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, Target, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(LeanProbeRadius), QueryParams))
	{
		return ParentTransform.InverseTransformPosition(FMath::Lerp(Start, Target, FMath::Clamp(Hit.Time, 0.0f, 1.0f)));
	}
	return CameraBaseLocation;
}

float AApecoxPlayerCharacter::GetCollisionLimitedLeanTarget(float DesiredLeanAmount, const FVector& CameraBaseLocation) const
{
	if (FMath::IsNearlyZero(DesiredLeanAmount) || !FirstPersonCamera || !FirstPersonCamera->GetAttachParent())
	{
		return DesiredLeanAmount;
	}

	const USceneComponent* CameraParent = FirstPersonCamera->GetAttachParent();
	const FVector Start = CameraParent->GetComponentTransform().TransformPosition(
		CameraBaseLocation);
	const FVector Target = CameraParent->GetComponentTransform().TransformPosition(
		CameraBaseLocation + FVector(0.0f, DesiredLeanAmount * MaxLeanDistance, 0.0f));

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ApecoxLeanProbe), false, this);
	FHitResult Hit;
	if (GetWorld() && GetWorld()->SweepSingleByChannel(
		Hit, Start, Target, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(LeanProbeRadius), QueryParams))
	{
		return DesiredLeanAmount * FMath::Clamp(Hit.Time, 0.0f, 1.0f);
	}

	return DesiredLeanAmount;
}

// --- Pawn AbilitySet ---

void AApecoxPlayerCharacter::GrantPawnAbilitySets()
{
	if (!HasAuthority())
	{
		return;
	}

	if (GrantedPawnAbilitySetHandles.Num() > 0)
	{
		return;
	}

	for (const TObjectPtr<const UApecoxAbilitySet>& AbilitySet : PawnAbilitySets)
	{
		if (!AbilitySet)
		{
			continue;
		}

		FApecoxAbilitySetGrantedHandles Handles;
		AbilitySet->GrantToAbilitySystem(CachedAbilitySystemComponent, Handles, this);
		GrantedPawnAbilitySetHandles.Add(Handles);
	}
}

void AApecoxPlayerCharacter::RemovePawnAbilitySets()
{
	for (FApecoxAbilitySetGrantedHandles& Handles : GrantedPawnAbilitySetHandles)
	{
		Handles.RemoveFromAbilitySystem(CachedAbilitySystemComponent);
	}
	GrantedPawnAbilitySetHandles.Reset();
}

// --- 死亡处理 ---

void AApecoxPlayerCharacter::OnDeathStarted(AActor* OwningActor)
{
	bWeaponFireInputHeld = false;
	CancelWeaponReload();
	CancelInspect();
	ResetAimState();
	ResetFirstPersonRecoil();
	if (EquipmentComponent)
	{
		// 镭射宿主是独立的本地表现 Actor，不能依赖 Pawn 隐藏或稍后的 EndPlay 清理。
		// 死亡序列一开始就在所有端销毁，避免外部挂载组件消失后挂件漂浮在世界中。
		EquipmentComponent->HandleOwnerDeathPresentation();
	}
	// 各端均执行——由复制的 DeathState 触发
	// 停止角色移动，防止死亡动画期间角色滑行
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 关闭 Capsule 碰撞：死亡角色不再阻挡子弹、不再被推挤
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AApecoxPlayerCharacter::OnDeathFinished(AActor* OwningActor)
{
	// 各端立即隐藏旧 Pawn
	SetActorHiddenInGame(true);

	// Authority 安排下一 Tick 销毁与重生，避免在死亡委托栈内直接拆 Pawn
	if (HasAuthority())
	{
		// 通过弱引用调度 next-tick——如果在下一 Tick 前 Pawn 已被其他路径销毁，安全跳过
		TWeakObjectPtr<AApecoxPlayerCharacter> WeakThis(this);
		GetWorld()->GetTimerManager().SetTimerForNextTick(
			[WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->DestroyDueToDeath();
				}
			});
	}
}

void AApecoxPlayerCharacter::DestroyDueToDeath()
{
	// 所有端被 SetActorHiddenInGame 隐藏已在 OnDeathFinished 中处理
	// Authority 端额外执行解绑占有和重生请求
	if (HasAuthority())
	{
		// 在 DetachFromControllerPendingDestroy 前缓存 Controller 引用
		AController* CachedController = GetController();

		// DetachFromControllerPendingDestroy：解除 Controller 对此 Pawn 的占有，
		// 确保重生倒计时到期时 GetPawn() == nullptr
		DetachFromControllerPendingDestroy();

		// 短 LifeSpan 兜底销毁（0.1 秒）——重生倒计时是独立 Timer，此 Pawn 应当在此之前销毁
		SetLifeSpan(0.1f);

		RequestRespawnFromGameMode(CachedController);
	}
}

void AApecoxPlayerCharacter::RequestRespawnFromGameMode(AController* RespawnController)
{
	APlayerController* PlayerController = Cast<APlayerController>(RespawnController);
	if (!PlayerController)
	{
		return;
	}

	if (AApecoxGameMode* GM = GetWorld()->GetAuthGameMode<AApecoxGameMode>())
	{
		GM->RequestPlayerRespawn(PlayerController);
	}
}

// --- 交互（如拾取武器） ---

void AApecoxPlayerCharacter::HandleInteractStarted(const FInputActionValue& ActionValue)
{
	// 本地 Trace 选择候选拾取物——不信任客户端结论，Server RPC 重新验证全部条件
	AApecoxWeaponPickup* Candidate = FindWeaponPickupCandidate();
	if (!Candidate)
	{
		return;
	}

	// 通过 Controller 的 InventoryComponent 提交拾取请求
	AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	UApecoxInventoryComponent* Inventory = PC->GetInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	Inventory->RequestPickupWeapon(Candidate);
}

AApecoxWeaponPickup* AApecoxPlayerCharacter::FindWeaponPickupCandidate() const
{
	if (!FirstPersonCamera)
	{
		return nullptr;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 使用第一人称摄像机做短距离 Visibility Trace
	const FVector TraceStart = FirstPersonCamera->GetComponentLocation();
	const FVector TraceEnd = TraceStart + FirstPersonCamera->GetForwardVector() * 250.0f; // 与 Pickup InteractionDistance 默认一致

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = true;

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Cast<AApecoxWeaponPickup>(Hit.GetActor());
	}

	return nullptr;
}

// --- Pawn 初始化 GE ---

void AApecoxPlayerCharacter::ApplyPawnInitializationEffect()
{
	// 仅在 Authority 上执行——客户端通过属性复制获取出生值
	if (!HasAuthority())
	{
		return;
	}

	if (!CachedAbilitySystemComponent)
	{
		return;
	}

	if (!PawnInitializationEffect)
	{
		// 未配置初始化 GE——蓝图子类可后续配置，不算错误
		return;
	}

	// 安全性检查：初始化 GE 必须是 Instant
	// 非 Instant GE（如 Infinite）会产生未跟踪 Active GE 句柄，导致复活后属性污染
	const UGameplayEffect* EffectCDO = PawnInitializationEffect->GetDefaultObject<UGameplayEffect>();
	if (!ensureMsgf(EffectCDO,
		TEXT("[Apecox] PawnInitializationEffect '%s' GetDefaultObject returned null."),
		*PawnInitializationEffect->GetName()))
	{
		return;
	}

	if (EffectCDO->DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		ensureMsgf(false,
			TEXT("[Apecox] PawnInitializationEffect '%s' is not Instant (policy=%d). "
				"Only Instant GE is supported for pawn initialization to avoid untracked active effects."),
			*PawnInitializationEffect->GetName(),
			static_cast<int32>(EffectCDO->DurationPolicy));
		return;
	}

	// 构建 GE Context：SourceObject 设为当前 Character
	FGameplayEffectContextHandle ContextHandle = CachedAbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = CachedAbilitySystemComponent->MakeOutgoingSpec(
		PawnInitializationEffect, 1.0f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// ApplyGameplayEffectSpecToSelf 对 Instant GE 返回无效句柄是正常行为
	CachedAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
