// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ApecoxInputConfig.generated.h"

class UInputAction;

/**
 * FApecoxInputAction
 * - InputAction 与 GameplayTag 的配对
 * - 用于 UApecoxInputConfig 中的 Native 和 Ability 输入数组
 */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * UApecoxInputConfig
 * - 不可变 DataAsset，映射 InputAction → GameplayTag
 * - NativeInputActions：移动/观察/交互等直接调用的输入
 * - AbilityInputActions：通过 ASC 路由到 AbilitySpec 的输入
 */
UCLASS(BlueprintType, Const)
class APECOX_API UApecoxInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// 使用 Exact Tag 匹配查找；失败返回 nullptr
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag) const;
	const UInputAction* FindAbilityInputActionForTag(const FGameplayTag& InputTag) const;

	// 直接调用的本地函数输入（移动、观察、交互等）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputAction"))
	TArray<FApecoxInputAction> NativeInputActions;

	// 通过 ASC 路由到 AbilitySpec 的输入
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (TitleProperty = "InputAction"))
	TArray<FApecoxInputAction> AbilityInputActions;
};
