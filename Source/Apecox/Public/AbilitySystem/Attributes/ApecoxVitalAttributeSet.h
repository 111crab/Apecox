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

struct FGameplayEffectModCallbackData;

/**
 * FApecoxAttributeEvent
 * - 原生多播委托，用于广播属性变化和 OutOfHealth
 * - 携带 EffectInstigator/Causer、GE Spec、Magnitude 和新旧值，
 *   接收方（HealthComponent）无需再反向查询最后一次 GE
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FApecoxAttributeEvent,
	AActor* /*EffectInstigator*/,
	AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* /*EffectSpec*/,
	float /*EffectMagnitude*/,
	float /*OldValue*/,
	float /*NewValue*/
);

/**
 * UApecoxVitalAttributeSet
 * - 建立生命、玩家护盾与护盾进化点的基础复制、访问器和数值钳制
 * - 广播 OnOutOfHealth（首次跨入 Health <= 0），但不销毁 Pawn、不请求重生
 * - 出生数值由初始化 GE 或英雄配置负责，不在此硬编码
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

	/** 当前护盾。AI 保持为 0；玩家伤害先结算护盾，再结算 Health。 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Apecox|Shield")
	FGameplayAttributeData Shield;
	APECOX_ATTRIBUTE_ACCESSORS(UApecoxVitalAttributeSet, Shield)

	/** 白/蓝/紫护盾上限分别为 25/50/75。 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxShield, Category = "Apecox|Shield")
	FGameplayAttributeData MaxShield;
	APECOX_ATTRIBUTE_ACCESSORS(UApecoxVitalAttributeSet, MaxShield)

	/** 本局跨 Pawn 生命周期保留的累计进化点；500/1500 分别升级蓝/紫。 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldEvolutionPoints, Category = "Apecox|Shield")
	FGameplayAttributeData ShieldEvolutionPoints;
	APECOX_ATTRIBUTE_ACCESSORS(UApecoxVitalAttributeSet, ShieldEvolutionPoints)

	// --- 复制 ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);

	UFUNCTION()
	virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	virtual void OnRep_ShieldEvolutionPoints(const FGameplayAttributeData& OldPoints);

	// --- 数值约束（基础值与最终值使用同一钳制规则） ---
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	// --- GE 前后值链路 ---
	/** 在 GE Modifier 执行前保存 Health/MaxHealth 旧值；
	 *  不在 PreAttributeChange 中保存，避免 PostGameplayEffectExecute 内
	 *  SetHealth() 二次经过 PreAttributeChange 覆盖已存旧值 */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// --- 属性变化委托（由 HealthComponent 绑定，不保存第二份数值） ---
	// GE 执行后广播；参数覆盖 Instigator、Causer、Spec、Magnitude、旧值、新值
	mutable FApecoxAttributeEvent OnHealthChanged;
	mutable FApecoxAttributeEvent OnMaxHealthChanged;
	mutable FApecoxAttributeEvent OnShieldChanged;
	mutable FApecoxAttributeEvent OnMaxShieldChanged;
	mutable FApecoxAttributeEvent OnShieldEvolutionPointsChanged;

	// 仅在首次跨入 Health <= 0 时广播一次；Health 恢复后（PostAttributeChange）
	// 重置 bOutOfHealth，允许下一次死亡再次广播
	mutable FApecoxAttributeEvent OnOutOfHealth;

private:
	/**
	 * 统一钳制入口：PreAttributeBaseChange 与 PreAttributeChange 均调用此函数，
	 * 保证基础值和最终值遵守同一不变量：MaxHealth >= 0，0 <= Health <= MaxHealth。
	 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	// 防止同一段持续低血量反复广播 OutOfHealth；
	// 仅在首次从正数跨入 <=0 时广播，Health 回到正数后重置
	bool bOutOfHealth = false;

	// 在 GE 执行前暂存旧值，用于 PostGameplayEffectExecute 中的变化广播
	float HealthBeforeAttributeChange = 0.0f;
	float MaxHealthBeforeAttributeChange = 0.0f;
	float ShieldBeforeAttributeChange = 0.0f;
	float MaxShieldBeforeAttributeChange = 0.0f;
	float ShieldEvolutionPointsBeforeAttributeChange = 0.0f;
};
