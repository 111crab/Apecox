// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ApecoxGameplayAbility.generated.h"

class UApecoxAbilitySystemComponent;
class AApecoxPlayerController;
class AApecoxPlayerCharacter;

/**
 * EApecoxAbilityActivationPolicy
 * - 描述 Ability 如何响应输入/系统事件
 */
UENUM(BlueprintType)
enum class EApecoxAbilityActivationPolicy : uint8
{
	OnInputTriggered	UMETA(DisplayName = "On Input Triggered"),
	WhileInputActive	UMETA(DisplayName = "While Input Active"),
	OnAvatarSet			UMETA(DisplayName = "On Avatar Set")
};

/**
 * EApecoxAbilityActivationGroup
 * - 并发运行规则——封闭枚举，MAX 为哨兵值用于固定计数数组
 */
UENUM(BlueprintType)
enum class EApecoxAbilityActivationGroup : uint8
{
	Independent				UMETA(DisplayName = "Independent"),
	ExclusiveReplaceable	UMETA(DisplayName = "Exclusive Replaceable"),
	ExclusiveBlocking		UMETA(DisplayName = "Exclusive Blocking"),
	MAX						UMETA(Hidden)
};

/**
 * UApecoxGameplayAbility
 * - Apecox 项目所有 GameplayAbility 的抽象基类
 */
UCLASS(Abstract, Blueprintable)
class APECOX_API UApecoxGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UApecoxGameplayAbility();

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	EApecoxAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	EApecoxAbilityActivationGroup GetActivationGroup() const { return ActivationGroup; }

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	AApecoxPlayerController* GetApecoxPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	AApecoxPlayerCharacter* GetApecoxPlayerCharacterFromActorInfo() const;

	/** 运行时切换并发组：仅已实例化且 Active 时可用 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	bool CanChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup) const;

	UFUNCTION(BlueprintCallable, Category = "Apecox|Ability")
	bool ChangeActivationGroup(EApecoxAbilityActivationGroup NewGroup);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void SetCanBeCanceled(bool bCanBeCanceled) override;

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

protected:
	/** 条件检查 + 实际调用 ASC->TryActivateAbility；OnGiveAbility 和 ASC AvatarSet 路径共用。
	 *  protected + friend UApecoxAbilitySystemComponent——不对外暴露 ActorInfo 生命周期入口 */
	bool TryActivateAbilityOnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) const;
	UFUNCTION(BlueprintImplementableEvent, Category = "Apecox|Ability", meta = (DisplayName = "OnPawnAvatarSet"))
	void K2_OnPawnAvatarSet();

	/** ASC 通过 friend 访问——不对外暴露 ActorInfo 变更入口 */
	virtual void OnPawnAvatarSet();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Ability")
	EApecoxAbilityActivationPolicy ActivationPolicy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Ability")
	EApecoxAbilityActivationGroup ActivationGroup;

	friend class UApecoxAbilitySystemComponent;
};
