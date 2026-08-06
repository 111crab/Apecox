// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/ApecoxGameplayAbility.h"
#include "ApecoxAbilitySystemComponent.generated.h"

/**
 * UApecoxAbilitySystemComponent
 * - Apecox 项目级强类型 ASC，统一管理：
 *   1) 输入缓存（Pressed 同时进入 Held，Released 无条件产生，逐帧统一调度）
 *   2) Generic Replicated Event（覆写 InputPressed/Released → InvokeReplicatedEvent 本地派发）
 *   3) Avatar 变更（清旧输入、通知 GA 实例 OnPawnAvatarSet、OnAvatarSet 自动激活）
 *   4) 并发组（固定计数数组来自 MAX 哨兵、排他替换/阻塞、check/ensure 对称）
 */
UCLASS()
class APECOX_API UApecoxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// ====================================================================
	//  输入接口
	// ====================================================================

	/** Pressed：同时 AddUnique 到 InputPressedSpecHandles 和 InputHeldSpecHandles */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** Released：无条件 AddUnique 到 InputReleasedSpecHandles，并从 Held 中 Remove */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/** 六步逐帧统一处理 */
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	/** 清空三组输入缓存 */
	void ClearAbilityInput();

	// ====================================================================
	//  Generic Replicated Event
	// ====================================================================

	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	// ====================================================================
	//  ActorInfo / Avatar
	// ====================================================================

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	// ====================================================================
	//  并发组
	// ====================================================================

	bool IsActivationGroupBlocked(EApecoxAbilityActivationGroup Group) const;

	void AddAbilityToActivationGroup(EApecoxAbilityActivationGroup Group, const UApecoxGameplayAbility* Ability);
	void RemoveAbilityFromActivationGroup(EApecoxAbilityActivationGroup Group, const UApecoxGameplayAbility* Ability);

	/** 取消组内匹配谓词的 Ability；bReplicateCancelAbility 传递给 CancelAbility */
	void CancelActivationGroupAbilities(EApecoxAbilityActivationGroup Group,
		const UApecoxGameplayAbility* IgnoreAbility, bool bReplicateCancelAbility);

	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability,
		bool bWasCancelled) override;

private:
	void CancelAbilitiesByFunc(const TFunction<bool(const UApecoxGameplayAbility*)>& Predicate,
		bool bReplicateCancelAbility);

	bool TryActivateAbilitiesOnAvatarSet();

	// ====================================================================
	//  输入缓存
	// ====================================================================

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	// ====================================================================
	//  并发组计数（大小 = static_cast<int32>(EApecoxAbilityActivationGroup::MAX)，零初始化）
	// ====================================================================

	int32 ActivationGroupCounts[static_cast<int32>(EApecoxAbilityActivationGroup::MAX)] = {};
};
