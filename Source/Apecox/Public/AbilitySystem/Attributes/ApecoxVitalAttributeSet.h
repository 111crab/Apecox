// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ApecoxVitalAttributeSet.generated.h"

// 项目级宏：为标准 GAS 属性生成 Get/Set/Init 访问器，避免每个属性重复模板代码
#define APECOX_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * UApecoxVitalAttributeSet
 * - 只建立 Health 与 MaxHealth 的基础复制、访问器和数值钳制
 * - 本批不触发死亡、不广播 GameplayEvent、不生成 Cue
 * - 出生数值由后续初始化 GE 或英雄配置负责，不在此硬编码
 */
UCLASS()
class APECOX_API UApecoxVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// 构造函数不设硬编码出生值；初始值由后续初始化 GE 或英雄配置负责
	UApecoxVitalAttributeSet() = default;

	// 属性访问器
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Apecox|Vital")
	FGameplayAttributeData Health;
	APECOX_ATTRIBUTE_ACCESSORS(UApecoxVitalAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Apecox|Vital")
	FGameplayAttributeData MaxHealth;
	APECOX_ATTRIBUTE_ACCESSORS(UApecoxVitalAttributeSet, MaxHealth)

	// --- 复制 ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	// --- 数值约束（基础值与最终值使用同一钳制规则） ---
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	/**
	 * 统一钳制入口：PreAttributeBaseChange 与 PreAttributeChange 均调用此函数，
	 * 保证基础值和最终值遵守同一不变量：MaxHealth >= 0，0 <= Health <= MaxHealth。
	 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
