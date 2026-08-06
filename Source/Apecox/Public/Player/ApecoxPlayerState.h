// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ApecoxPlayerState.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxVitalAttributeSet;

/**
 * AApecoxPlayerState
 * - ASC 和 VitalAttributeSet 的真正 Owner，保证它们可跨 Pawn 生命周期存在
 * - 实现 IAbilitySystemInterface，对外提供强类型 ASC 访问
 * - ASC 使用 Mixed 复制模式：拥有客户端收到完整 GE 信息，远端仅收到 Cue/公开属性
 */
UCLASS()
class APECOX_API AApecoxPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerState();

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 项目强类型 ASC 访问
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

	// 只读 AttributeSet 访问；后续数值修改必须通过 ASC/GameplayEffect
	const UApecoxVitalAttributeSet* GetVitalAttributeSet() const;

private:
	// PlayerState 是 ASC 和 AttributeSet 的唯一 Owner：
	// 构造时创建默认子对象，生命周期由 PlayerState 管理，Character 只看不创建
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxVitalAttributeSet> VitalAttributeSet;
};
