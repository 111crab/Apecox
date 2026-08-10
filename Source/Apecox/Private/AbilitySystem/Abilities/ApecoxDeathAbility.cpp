// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Abilities/ApecoxDeathAbility.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "Character/ApecoxHealthComponent.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "AbilitySystemComponent.h"

UApecoxDeathAbility::UApecoxDeathAbility()
{
	// 死亡由服务器权威触发——客户端通过 OnRep_DeathState 感知
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	ActivationPolicy = EApecoxAbilityActivationPolicy::OnInputTriggered;
	ActivationGroup = EApecoxAbilityActivationGroup::ExclusiveBlocking;

	// 通过 GameplayEvent.Death 触发（不是输入触发）
	AbilityTriggers.AddDefaulted_GetRef().TriggerTag = ApecoxGameplayTags::GameplayEvent_Death;
}

void UApecoxDeathAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 每次激活重置状态标志
	bDeathStarted = false;
	bDeathFinished = false;

	// 1. 严格验证 ActorInfo、ASC、Avatar 与 HealthComponent
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !ActorInfo->AvatarActor.IsValid())
	{
		ensureMsgf(false, TEXT("[Apecox] DeathAbility::ActivateAbility: ActorInfo, ASC, or Avatar is invalid."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UApecoxAbilitySystemComponent* ApecoxASC = Cast<UApecoxAbilitySystemComponent>(
		ActorInfo->AbilitySystemComponent.Get());
	if (!ApecoxASC)
	{
		ensureMsgf(false, TEXT("[Apecox] DeathAbility::ActivateAbility: ASC is not UApecoxAbilitySystemComponent."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	UApecoxHealthComponent* HealthComp = Avatar->FindComponentByClass<UApecoxHealthComponent>();
	if (!HealthComp)
	{
		ensureMsgf(false, TEXT("[Apecox] DeathAbility::ActivateAbility: '%s' has no UApecoxHealthComponent."),
			*GetNameSafe(Avatar));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 取消除自身外的所有活动 Ability，清空输入缓存
	// 使用 GAS 引擎接口 CancelAbilities(nullptr, nullptr, this) 而非暴露 ASC 内部 TFunction API
	CancelOtherAbilitiesAndClearInput();

	// 3. 自身不可取消，确保死亡流程不会被其他系统中断
	SetCanBeCanceled(false);

	// 4. 调用 HealthComponent::StartDeath
	HealthComp->StartDeath();
	bDeathStarted = true;

	// 5. 蓝图事件出口——未来在此播放死亡蒙太奇
	K2_OnDeathStarted();

	// 6. 首版自动完成死亡；未来关闭后由蒙太奇结束回调调用 FinishDeathAndEndAbility
	if (bAutoFinishDeath)
	{
		FinishDeathAndEndAbility();
	}
}

void UApecoxDeathAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 只在死亡已经成功开始但尚未完成时补调用 FinishDeath
	// 激活前验证失败不会进入此分支（bDeathStarted 仍为 false）
	if (bDeathStarted && !bDeathFinished)
	{
		AActor* Avatar = ActorInfo->AvatarActor.Get();
		if (Avatar)
		{
			UApecoxHealthComponent* HealthComp = Avatar->FindComponentByClass<UApecoxHealthComponent>();
			if (HealthComp)
			{
				HealthComp->FinishDeath();
			}
		}
		bDeathFinished = true;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UApecoxDeathAbility::FinishDeathAndEndAbility()
{
	if (bDeathFinished)
	{
		return; // 幂等
	}

	// BlueprintCallable 入口必须防护 CurrentActorInfo 和 Avatar 失效
	if (!CurrentActorInfo)
	{
		return;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return;
	}

	UApecoxHealthComponent* HealthComp = Avatar->FindComponentByClass<UApecoxHealthComponent>();
	if (HealthComp)
	{
		HealthComp->FinishDeath();
	}
	else
	{
		ensureMsgf(false,
			TEXT("[Apecox] DeathAbility::FinishDeathAndEndAbility: '%s' has no UApecoxHealthComponent."),
			*GetNameSafe(Avatar));
	}

	bDeathFinished = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UApecoxDeathAbility::CancelOtherAbilitiesAndClearInput()
{
	UApecoxAbilitySystemComponent* ApecoxASC = GetApecoxAbilitySystemComponentFromActorInfo();
	if (!ApecoxASC)
	{
		return;
	}

	// 清空输入缓存，防止死亡期间积压的输入在复活后意外触发
	ApecoxASC->ClearAbilityInput();

	// 使用 GAS 引擎接口 CancelAbilities：
	// - 第一个 nullptr WithTags：不按 Tag 过滤，所有活动 GA 均匹配
	// - 第二个 nullptr WithoutTags：不排除特定 Tag 的 GA
	// - 第三个 this：忽略自身实例
	// - bReplicateCancelAbility = true：同步取消到客户端
	ApecoxASC->CancelAbilities(
		nullptr,   // WithTags：不按 Tag 过滤
		nullptr,   // WithoutTags：不排除特定 Tag
		this       // Ignore：保留自身
	);
}
