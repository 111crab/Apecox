// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/Pawn.h"

// ====================================================================
//  输入接口
// ====================================================================

void UApecoxAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			// 修复 1：Pressed 同时进入 Pressed 和 Held
			InputPressedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.AddUnique(Spec.Handle);
		}
	}
}

void UApecoxAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			// 修复 1：无条件 AddUnique 到 Released；从 Held 中移除
			InputReleasedSpecHandles.AddUnique(Spec.Handle);
			InputHeldSpecHandles.Remove(Spec.Handle);
		}
	}
}

void UApecoxAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	if (HasMatchingGameplayTag(ApecoxGameplayTags::State_Input_AbilityBlocked))
	{
		ClearAbilityInput();
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	// 2. Held → WhileInputActive 未激活 Spec
	for (const FGameplayAbilitySpecHandle& Handle : InputHeldSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec || Spec->IsActive())
		{
			continue;
		}

		UApecoxGameplayAbility* AbilityCDO = Cast<UApecoxGameplayAbility>(Spec->Ability.Get());
		if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EApecoxAbilityActivationPolicy::WhileInputActive)
		{
			AbilitiesToActivate.AddUnique(Handle);
		}
	}

	// 3. Pressed
	for (const FGameplayAbilitySpecHandle& Handle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

		Spec->InputPressed = true;

		if (Spec->IsActive())
		{
			AbilitySpecInputPressed(*Spec);
		}
		else
		{
			UApecoxGameplayAbility* AbilityCDO = Cast<UApecoxGameplayAbility>(Spec->Ability.Get());
			if (AbilityCDO && AbilityCDO->GetActivationPolicy() == EApecoxAbilityActivationPolicy::OnInputTriggered)
			{
				AbilitiesToActivate.AddUnique(Handle);
			}
		}
	}

	// 4. 统一激活
	for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToActivate)
	{
		TryActivateAbility(Handle);
	}

	// 5. Released
	for (const FGameplayAbilitySpecHandle& Handle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

		Spec->InputPressed = false;

		if (Spec->IsActive())
		{
			AbilitySpecInputReleased(*Spec);
		}
	}

	// 6. 清空 Pressed/Released
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UApecoxAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

// ====================================================================
//  Generic Replicated Event
// ====================================================================

void UApecoxAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (Spec.IsActive())
	{
		FPredictionKey OriginalPredictionKey;
		if (UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance())
		{
			OriginalPredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
		}
		else
		{
			// 修复 8：无实例时回退到 Spec.ActivationInfo（兼容非预期/旧 Spec），
			// 仅在弃用字段访问处局部禁用警告
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			OriginalPredictionKey = Spec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}

		// InvokeReplicatedEvent 是本地 delegate 派发——不是向所有客户端广播。
		// 对 LocalPredicted GA：客户端到服务器的上行由 WaitInputPress/Release Task
		// 在其 delegate 回调中按 SpecHandle + PredictionKey 完成。
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, OriginalPredictionKey);
	}
}

void UApecoxAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (Spec.IsActive())
	{
		FPredictionKey OriginalPredictionKey;
		if (UGameplayAbility* PrimaryInstance = Spec.GetPrimaryInstance())
		{
			OriginalPredictionKey = PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
		}
		else
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
			OriginalPredictionKey = Spec.ActivationInfo.GetActivationPredictionKey();
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}

		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, OriginalPredictionKey);
	}
}

// ====================================================================
//  ActorInfo / Avatar
// ====================================================================

void UApecoxAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	const bool bNewAvatar = (InAvatarActor != GetAvatarActor());

	if (bNewAvatar)
	{
		ClearAbilityInput();
	}

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (bNewAvatar)
	{
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
		{
			if (!Spec.Ability || !Spec.Ability->IsA<UApecoxGameplayAbility>())
			{
				continue;
			}

			UGameplayAbility* AbilityCDO = Spec.Ability.Get();
			if (AbilityCDO && AbilityCDO->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced)
			{
				ensureMsgf(false, TEXT("[Apecox] Non-Instanced Ability '%s' found."), *GetNameSafe(Spec.Ability));
				continue;
			}

			TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
			for (UGameplayAbility* Instance : Instances)
			{
				if (UApecoxGameplayAbility* ApecoxGA = Cast<UApecoxGameplayAbility>(Instance))
				{
					// friend 访问 protected OnPawnAvatarSet
					ApecoxGA->OnPawnAvatarSet();
				}
			}
		}

		TryActivateAbilitiesOnAvatarSet();
	}
}

bool UApecoxAbilitySystemComponent::TryActivateAbilitiesOnAvatarSet()
{
	bool bActivatedAny = false;

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.Ability)
		{
			continue;
		}

		UApecoxGameplayAbility* AbilityCDO = Cast<UApecoxGameplayAbility>(Spec.Ability.Get());
		if (!AbilityCDO)
		{
			continue;
		}

		// 修复 2：helper 内部已调用 TryActivateAbility，这里只累计返回值
		if (AbilityCDO->TryActivateAbilityOnAvatarSet(AbilityActorInfo.Get(), Spec))
		{
			bActivatedAny = true;
		}
	}

	return bActivatedAny;
}

// ====================================================================
//  并发组
// ====================================================================

namespace
{
	// 统一范围检查：0 <= Index < MAX，覆盖 C++ cast 可能传入大于 MAX 的底层值
	FORCEINLINE bool IsValidActivationGroup(const EApecoxAbilityActivationGroup Group)
	{
		const int32 Index = static_cast<int32>(Group);
		return Index >= 0 && Index < static_cast<int32>(EApecoxAbilityActivationGroup::MAX);
	}
}

bool UApecoxAbilitySystemComponent::IsActivationGroupBlocked(EApecoxAbilityActivationGroup Group) const
{
	if (!IsValidActivationGroup(Group))
	{
		ensureMsgf(false, TEXT("[Apecox] IsActivationGroupBlocked called with invalid group %d"),
			static_cast<int32>(Group));
		return true; // 无效组保守视为被阻止
	}

	if (Group == EApecoxAbilityActivationGroup::Independent)
	{
		return false;
	}

	// Blocking 存在时，Replaceable 和 Blocking 都被阻止
	const int32 BlockingIndex = static_cast<int32>(EApecoxAbilityActivationGroup::ExclusiveBlocking);
	if (ActivationGroupCounts[BlockingIndex] > 0)
	{
		return true;
	}

	return false;
}

void UApecoxAbilitySystemComponent::AddAbilityToActivationGroup(EApecoxAbilityActivationGroup Group,
	const UApecoxGameplayAbility* Ability)
{
	if (!IsValidActivationGroup(Group))
	{
		ensureMsgf(false, TEXT("[Apecox] AddAbilityToActivationGroup called with invalid group %d"),
			static_cast<int32>(Group));
		return;
	}

	const int32 Index = static_cast<int32>(Group);

	// 修复 3：递增前检查溢出
	if (!ensureMsgf(ActivationGroupCounts[Index] < INT32_MAX,
		TEXT("[Apecox] ActivationGroup count overflow for group %d"), Index))
	{
		return;
	}
	ActivationGroupCounts[Index]++;

	// Independent 不取消任何东西
	if (Group == EApecoxAbilityActivationGroup::Independent)
	{
		return;
	}

	// 修复 4：新的 Replaceable 或 Blocking 都只取消旧的 ExclusiveReplaceable，
	// 不主动取消 Blocking——Blocking 在 CanActivate 阶段阻止新排他 Ability
	CancelActivationGroupAbilities(EApecoxAbilityActivationGroup::ExclusiveReplaceable, Ability, false);

	// 修复 4：Replaceable + Blocking 总数不得大于 1
	if (Group == EApecoxAbilityActivationGroup::ExclusiveReplaceable ||
		Group == EApecoxAbilityActivationGroup::ExclusiveBlocking)
	{
		const int32 ReplaceableIndex = static_cast<int32>(EApecoxAbilityActivationGroup::ExclusiveReplaceable);
		const int32 BlockingIndex = static_cast<int32>(EApecoxAbilityActivationGroup::ExclusiveBlocking);
		const int32 ExclusiveTotal = ActivationGroupCounts[ReplaceableIndex] + ActivationGroupCounts[BlockingIndex];
		ensureMsgf(ExclusiveTotal <= 1,
			TEXT("[Apecox] Multiple exclusive abilities active: %d Replaceable + %d Blocking"),
			ActivationGroupCounts[ReplaceableIndex], ActivationGroupCounts[BlockingIndex]);
	}
}

void UApecoxAbilitySystemComponent::RemoveAbilityFromActivationGroup(EApecoxAbilityActivationGroup Group,
	const UApecoxGameplayAbility* Ability)
{
	if (!IsValidActivationGroup(Group))
	{
		ensureMsgf(false, TEXT("[Apecox] RemoveAbilityFromActivationGroup called with invalid group %d"),
			static_cast<int32>(Group));
		return;
	}

	const int32 Index = static_cast<int32>(Group);
	ensureMsgf(ActivationGroupCounts[Index] > 0,
		TEXT("[Apecox] ActivationGroup count underflow for group %d"), Index);
	ActivationGroupCounts[Index] = FMath::Max(ActivationGroupCounts[Index] - 1, 0);
}

void UApecoxAbilitySystemComponent::CancelActivationGroupAbilities(EApecoxAbilityActivationGroup Group,
	const UApecoxGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility)
{
	CancelAbilitiesByFunc(
		[Group, IgnoreAbility](const UApecoxGameplayAbility* Ability) -> bool
		{
			if (Ability == IgnoreAbility)
			{
				return false;
			}
			if (Ability->GetActivationGroup() != Group)
			{
				return false;
			}
			return true;
		},
		bReplicateCancelAbility
	);
}

void UApecoxAbilitySystemComponent::CancelAbilitiesByFunc(
	const TFunction<bool(const UApecoxGameplayAbility*)>& Predicate, bool bReplicateCancelAbility)
{
	ABILITYLIST_SCOPE_LOCK();

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (!Spec.IsActive())
		{
			continue;
		}

		TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
		for (UGameplayAbility* Instance : Instances)
		{
			UApecoxGameplayAbility* ApecoxGA = Cast<UApecoxGameplayAbility>(Instance);
			if (!ApecoxGA)
			{
				continue;
			}

			// 修复 4：Predicate 决定是否匹配；CanBeCanceled 额外检查；
			// bReplicateCancelAbility 原样传给 CancelAbility
			if (Predicate(ApecoxGA) && ApecoxGA->CanBeCanceled())
			{
				ApecoxGA->CancelAbility(Spec.Handle, AbilityActorInfo.Get(),
					ApecoxGA->GetCurrentActivationInfo(), bReplicateCancelAbility);
			}
		}
	}
}

void UApecoxAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);

	if (UApecoxGameplayAbility* ApecoxGA = Cast<UApecoxGameplayAbility>(Ability))
	{
		AddAbilityToActivationGroup(ApecoxGA->GetActivationGroup(), ApecoxGA);
	}
}

void UApecoxAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle,
	UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);

	if (UApecoxGameplayAbility* ApecoxGA = Cast<UApecoxGameplayAbility>(Ability))
	{
		RemoveAbilityFromActivationGroup(ApecoxGA->GetActivationGroup(), ApecoxGA);
	}
}
