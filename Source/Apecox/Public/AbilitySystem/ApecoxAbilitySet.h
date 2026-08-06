// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "ApecoxAbilitySet.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxGameplayAbility;
class UGameplayEffect;
class UAttributeSet;

USTRUCT(BlueprintType)
struct APECOX_API FApecoxAbilitySetAbility
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UApecoxGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

USTRUCT(BlueprintType)
struct APECOX_API FApecoxAbilitySetGameplayEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float EffectLevel = 1.0f;
};

USTRUCT(BlueprintType)
struct APECOX_API FApecoxAbilitySetAttributeSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeSet> AttributeSet = nullptr;
};

USTRUCT(BlueprintType)
struct APECOX_API FApecoxAbilitySetGrantedHandles
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> ActiveGameplayEffectHandles;

	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;

	/** 仅在 Authority 执行；ASC 无效时安全返回；重复调用安全 */
	void RemoveFromAbilitySystem(UApecoxAbilitySystemComponent* ASC);
};

UCLASS(BlueprintType, Const)
class APECOX_API UApecoxAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 仅在 Authority 执行；ASC 无效/客户端时安全返回 */
	void GrantToAbilitySystem(UApecoxAbilitySystemComponent* ASC,
		FApecoxAbilitySetGrantedHandles& OutGrantedHandles,
		UObject* SourceObject = nullptr) const;

	UPROPERTY(EditDefaultsOnly, Category = "AbilitySet")
	TArray<FApecoxAbilitySetAbility> GrantedAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "AbilitySet")
	TArray<FApecoxAbilitySetGameplayEffect> GrantedGameplayEffects;

	UPROPERTY(EditDefaultsOnly, Category = "AbilitySet")
	TArray<FApecoxAbilitySetAttributeSet> GrantedAttributeSets;
};
