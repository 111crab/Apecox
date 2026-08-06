// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Abilities/ApecoxGameplayAbility.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Player/ApecoxPlayerController.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UApecoxGameplayAbility::UApecoxGameplayAbility()
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	ActivationPolicy = EApecoxAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EApecoxAbilityActivationGroup::Independent;
}

// --- 项目强类型 Getter：空指针安全 ---

UApecoxAbilitySystemComponent* UApecoxGameplayAbility::GetApecoxAbilitySystemComponentFromActorInfo() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}
	return Cast<UApecoxAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
}

AApecoxPlayerController* UApecoxGameplayAbility::GetApecoxPlayerControllerFromActorInfo() const
{
	if (!CurrentActorInfo || !CurrentActorInfo->PlayerController.IsValid())
	{
		return nullptr;
	}
	return Cast<AApecoxPlayerController>(CurrentActorInfo->PlayerController.Get());
}

AApecoxPlayerCharacter* UApecoxGameplayAbility::GetApecoxPlayerCharacterFromActorInfo() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}
	return Cast<AApecoxPlayerCharacter>(CurrentActorInfo->AvatarActor.Get());
}

// --- 运行时并发组切换 ---

namespace
{
	FORCEINLINE bool IsValidActivationGroupGA(const EApecoxAbilityActivationGroup Group)
	{
		const int32 Index = static_cast<int32>(Group);
		return Index >= 0 && Index < static_cast<int32>(EApecoxAbilityActivationGroup::MAX);
	}
}

bool UApecoxGameplayAbility::CanChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup) const
{
	// 修复 1：拒绝无效组（包括 MAX 和越界 C++ cast）
	if (!IsValidActivationGroupGA(NewGroup))
	{
		return false;
	}

	// 只有已实例化且 Active 的 Ability 才能运行时切换
	if (!IsActive() || !IsInstantiated())
	{
		return false;
	}

	// 同组无需切换
	if (NewGroup == ActivationGroup)
	{
		return true;
	}

	// 当前不是 Blocking 时，目标组被 Blocking 阻止则不可切换
	if (ActivationGroup != EApecoxAbilityActivationGroup::ExclusiveBlocking)
	{
		UApecoxAbilitySystemComponent* ApecoxASC = GetApecoxAbilitySystemComponentFromActorInfo();
		if (ApecoxASC && ApecoxASC->IsActivationGroupBlocked(NewGroup))
		{
			return false;
		}
	}

	// 不可取消的 Ability 不能切换到 Replaceable（切换后必须可被替换）
	if (NewGroup == EApecoxAbilityActivationGroup::ExclusiveReplaceable && !CanBeCanceled())
	{
		return false;
	}

	return true;
}

bool UApecoxGameplayAbility::ChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup)
{
	if (!CanChangeActivationGroup(NewGroup))
	{
		return false;
	}

	// 同组无操作
	if (NewGroup == ActivationGroup)
	{
		return true;
	}

	UApecoxAbilitySystemComponent* ApecoxASC = GetApecoxAbilitySystemComponentFromActorInfo();
	if (!ensure(ApecoxASC))
	{
		return false;
	}

	// 从旧组移除计数 → 加入新组计数 → 更新实例字段
	ApecoxASC->RemoveAbilityFromActivationGroup(ActivationGroup, this);
	ApecoxASC->AddAbilityToActivationGroup(NewGroup, this);
	ActivationGroup = NewGroup;

	return true;
}

// --- 覆写 ---

bool UApecoxGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 修复 3：使用传入的 ActorInfo 而非 CurrentActorInfo 的 Getter——
	// 激活前检查可能在 CDO 或尚未建立实例上下文的路径上运行
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		// 修复 2：ActorInfo/ASC 无效时激活必须失败（Super 通常已拒绝，但语义必须一致）
		return false;
	}

	UApecoxAbilitySystemComponent* ApecoxASC = Cast<UApecoxAbilitySystemComponent>(
		ActorInfo->AbilitySystemComponent.Get());
	if (!ApecoxASC)
	{
		// 项目 GA 运行在非 Apecox ASC 上是配置错误
		ensureMsgf(false, TEXT("[Apecox] GA '%s' is running on a non-Apecox ASC. "
			"ActivationGroup check will be bypassed."), *GetName());
		return false;
	}

	return !ApecoxASC->IsActivationGroupBlocked(ActivationGroup);
}

void UApecoxGameplayAbility::SetCanBeCanceled(bool bCanBeCanceled)
{
	if (!bCanBeCanceled && ActivationGroup == EApecoxAbilityActivationGroup::ExclusiveReplaceable)
	{
		ensureMsgf(false, TEXT("[Apecox] Cannot make ExclusiveReplaceable GA '%s' uncancelable."), *GetName());
		return;
	}

	Super::SetCanBeCanceled(bCanBeCanceled);
}

void UApecoxGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	TryActivateAbilityOnAvatarSet(ActorInfo, Spec);
}

bool UApecoxGameplayAbility::TryActivateAbilityOnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec) const
{
	if (ActivationPolicy != EApecoxAbilityActivationPolicy::OnAvatarSet)
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	if (Spec.IsActive())
	{
		return false;
	}

	const AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (Avatar->GetTearOff() || Avatar->GetLifeSpan() > 0.0f)
	{
		return false;
	}

	switch (NetExecutionPolicy)
	{
	case EGameplayAbilityNetExecutionPolicy::LocalPredicted:
	case EGameplayAbilityNetExecutionPolicy::LocalOnly:
		if (!ActorInfo->IsLocallyControlled())
		{
			return false;
		}
		break;

	case EGameplayAbilityNetExecutionPolicy::ServerOnly:
	case EGameplayAbilityNetExecutionPolicy::ServerInitiated:
		if (!ActorInfo->IsNetAuthority())
		{
			return false;
		}
		break;

	default:
		break;
	}

	// 修复 2：条件全部通过后，真正调用 TryActivateAbility
	return ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
}

void UApecoxGameplayAbility::OnPawnAvatarSet()
{
	K2_OnPawnAvatarSet();
}
