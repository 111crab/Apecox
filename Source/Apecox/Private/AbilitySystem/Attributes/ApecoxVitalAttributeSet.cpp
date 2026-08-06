// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"

void UApecoxVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None：无条件复制给所有相关客户端
	// REPNOTIFY_Always：即使值未变化也触发 OnRep，保证新连接客户端收到正确初始值
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UApecoxVitalAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, Health, OldHealth);
}

void UApecoxVitalAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, MaxHealth, OldMaxHealth);
}

// --- 统一钳制：基础值和最终值共享同一不变量 ---

void UApecoxVitalAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetMaxHealthAttribute())
	{
		// MaxHealth 不能为负
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetHealthAttribute())
	{
		// Health 受当前 MaxHealth 上限约束，不低于 0
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UApecoxVitalAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UApecoxVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

// MaxHealth 降低后，确保现有 Health 不超出新上限
void UApecoxVitalAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxHealthAttribute())
	{
		const float CurrentHealth = GetHealth();
		if (CurrentHealth > NewValue)
		{
			// MaxHealth 降低了，且当前 Health 超出新上限：
			// 通过 ASC 的标准属性修改入口把 Health 基础值压回新上限。
			// SetNumericAttributeBase 会重新评估活跃 GE，PreAttributeBaseChange/PreAttributeChange
			// 会再次钳制——但此时 Health 最终值一定不会超过新 MaxHealth，不会形成无限循环。
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ensure(ASC))
			{
				ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
			}
		}
	}
}

void UApecoxVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// GE 执行后再次钳制：确保 Attribute 实际值不超出合理范围
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// 不调用 SetMaxHealth(GetMaxHealth())：
		// GetMaxHealth() 是包含活跃 Modifier 的当前聚合值，把它写回 Base Value
		// 会在持续 MaxHealth Buff 存在时重复计入 Modifier，造成数值膨胀。
		// MaxHealth Base 钳制和最终值约束已由 PreAttributeBaseChange/PreAttributeChange 负责，
		// MaxHealth 下降后 Health 跟随逻辑已由 PostAttributeChange 处理。

		// 防御检查：GE 执行后若 Health 仍超出当前 MaxHealth，通过 ASC 标准入口压低
		const float CurrentHealth = GetHealth();
		const float CurrentMaxHealth = GetMaxHealth();
		if (CurrentHealth > CurrentMaxHealth)
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ensure(ASC))
			{
				ASC->SetNumericAttributeBase(GetHealthAttribute(), CurrentMaxHealth);
			}
		}
	}
}
