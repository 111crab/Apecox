// Copyright Apecox. All Rights Reserved.

#include "Character/ApecoxPlayerCharacter.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/ApecoxAbilitySet.h"
#include "Input/ApecoxInputConfig.h"
#include "Input/ApecoxInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

AApecoxPlayerCharacter::AApecoxPlayerCharacter()
{
	CachedAbilitySystemComponent = nullptr;
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
}

void AApecoxPlayerCharacter::UninitializeAbilitySystem()
{
	if (CachedAbilitySystemComponent)
	{
		if (CachedAbilitySystemComponent->GetAvatarActor() == this)
		{
			CachedAbilitySystemComponent->ClearAbilityInput();
			CachedAbilitySystemComponent->CancelAllAbilities();
			RemovePawnAbilitySets();

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
			if (GetLocalRole() == ROLE_Authority)
			{
				RemovePawnAbilitySets();
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
