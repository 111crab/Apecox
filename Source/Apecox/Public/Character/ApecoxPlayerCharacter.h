// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "ApecoxPlayerCharacter.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxInputConfig;
class UApecoxAbilitySet;
struct FApecoxAbilitySetGrantedHandles;
class UInputMappingContext;

UCLASS()
class APECOX_API AApecoxPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

protected:
	void InitializeAbilitySystem();
	void UninitializeAbilitySystem();

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void GrantPawnAbilitySets();
	void RemovePawnAbilitySets();

	/** 修复 9：对称移除本地 IMC——Setup 重装前和 UnPossessed/EndPlay 清理时调用 */
	void RemoveDefaultInputMappingContext();

	// 回调签名：void(const FInputActionValue&, FGameplayTag)
	void HandleAbilityInputTagPressed(const FInputActionValue& ActionValue, FGameplayTag InputTag);
	void HandleAbilityInputTagReleased(const FInputActionValue& ActionValue, FGameplayTag InputTag);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UApecoxInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	int32 DefaultInputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Ability")
	TArray<TObjectPtr<const UApecoxAbilitySet>> PawnAbilitySets;

private:
	UPROPERTY(Transient)
	TObjectPtr<UApecoxAbilitySystemComponent> CachedAbilitySystemComponent;

	UPROPERTY(Transient)
	TArray<uint32> AbilityInputBindingHandles;

	UPROPERTY(Transient)
	TArray<FApecoxAbilitySetGrantedHandles> GrantedPawnAbilitySetHandles;
};
