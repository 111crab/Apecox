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
#include "GameplayEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

AApecoxPlayerCharacter::AApecoxPlayerCharacter()
{
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

		// 拥有者本地的第三人称 Mesh 因 OwnerNoSee 不渲染，
		// 但 ABP_FP_Copy 通过 Copy Pose From Mesh 从本组件读取组件空间骨骼姿态。
		// ACharacter 默认的 AlwaysTickPose 在未渲染时只更新动画 Pose，
		// 不保证刷新组件空间骨骼变换，可能导致第一人称手臂停在参考姿势。
		// 因此必须显式设为 AlwaysTickPoseAndRefreshBones。
		ThirdPersonMesh->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}

	// --- 第一人称 Mesh ---
	// 仅拥有者可见（OnlyOwnerSee），附着于第三人称 Mesh，共享骨架层级
	// 资产（SkeletalMesh/AnimClass）不写死在 C++ 中，由蓝图子类配置
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonMesh"));
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetupAttachment(ThirdPersonMesh);
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
		FirstPersonMesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}

	// --- 第一人称摄像机 ---
	// 附着于 FirstPersonMesh 的 head Socket，使用 Pawn Control Rotation 控制视角
	// FirstPersonFieldOfView/FirstPersonScale 复现了 UE 5.8 官方模板的 FOV 修正与视线遮挡比例
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetupAttachment(FirstPersonMesh, TEXT("head"));
		FirstPersonCamera->SetRelativeLocationAndRotation(
			FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
		FirstPersonCamera->bUsePawnControlRotation = true;
		FirstPersonCamera->bEnableFirstPersonFieldOfView = true;
		FirstPersonCamera->bEnableFirstPersonScale = true;
		FirstPersonCamera->FirstPersonFieldOfView = 70.0f;
		FirstPersonCamera->FirstPersonScale = 0.6f;
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
		// 使用 UE 5.8 第一人称模板的最小空中操控基线，
		// 不新增 Sprint、Crouch、Slide 或 GAS 移动 Ability
		CMC->BrakingDecelerationFalling = 1500.0f;
		CMC->AirControl = 0.5f;
	}

	// HealthComponent 生命周期由 Character 拥有；绑定死亡委托
	HealthComponent = CreateDefaultSubobject<UApecoxHealthComponent>(TEXT("HealthComponent"));
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

	// 绑定死亡状态转换委托；解绑在 UninitializeAbilitySystem 中处理
	OnDeathStartedHandle = HealthComponent->OnDeathStarted.AddUObject(
		this, &AApecoxPlayerCharacter::OnDeathStarted);
	OnDeathFinishedHandle = HealthComponent->OnDeathFinished.AddUObject(
		this, &AApecoxPlayerCharacter::OnDeathFinished);

	// 仅在 Authority 上应用 PawnInitializationEffect 恢复出生属性
	ApplyPawnInitializationEffect();
}

void AApecoxPlayerCharacter::UninitializeAbilitySystem()
{
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
		}

		CachedAbilitySystemComponent = nullptr;
	}
}

void AApecoxPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
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
	if (CachedAbilitySystemComponent)
	{
		CachedAbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AApecoxPlayerCharacter::HandleAbilityInputTagReleased(const FInputActionValue& ActionValue, FGameplayTag InputTag)
{
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
	if (MovementVector.IsNearlyZero())
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

	if (LookVector.X != 0.0f)
	{
		// Yaw 正负由用户在 IMC Modifier 中配置，C++ 不擅自反转
		AddControllerYawInput(LookVector.X);
	}

	if (LookVector.Y != 0.0f)
	{
		// Pitch 正负由用户在 IMC Modifier 中配置——通常是"鼠标上推=抬头"
		AddControllerPitchInput(LookVector.Y);
	}
}

void AApecoxPlayerCharacter::HandleJumpStarted(const FInputActionValue& ActionValue)
{
	Jump();
}

void AApecoxPlayerCharacter::HandleJumpCompleted(const FInputActionValue& ActionValue)
{
	StopJumping();
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
		APlayerController* PC = Cast<APlayerController>(CachedController);

		// DetachFromControllerPendingDestroy：解除 Controller 对此 Pawn 的占有，
		// 确保重生倒计时到期时 GetPawn() == nullptr
		DetachFromControllerPendingDestroy();

		// 短 LifeSpan 兜底销毁（0.1 秒）——重生倒计时是独立 Timer，此 Pawn 应当在此之前销毁
		SetLifeSpan(0.1f);

		// 请求 GameMode 为该 Controller 创建独立的延迟重生
		if (PC)
		{
			if (AApecoxGameMode* GM = GetWorld()->GetAuthGameMode<AApecoxGameMode>())
			{
				GM->RequestPlayerRespawn(PC);
			}
		}
	}
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
