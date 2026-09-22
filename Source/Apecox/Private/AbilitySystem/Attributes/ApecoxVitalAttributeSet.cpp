// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "Player/ApecoxPlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

namespace
{
	AApecoxPlayerState* ResolveApecoxPlayerState(AActor* Actor)
	{
		if (AApecoxPlayerState* PlayerState = Cast<AApecoxPlayerState>(Actor))
		{
			return PlayerState;
		}
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetPlayerState<AApecoxPlayerState>();
		}
		if (const AController* Controller = Cast<AController>(Actor))
		{
			return Controller->GetPlayerState<AApecoxPlayerState>();
		}
		return nullptr;
	}
}

void UApecoxVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// COND_None：无条件复制给所有相关客户端
	// REPNOTIFY_Always：即使值未变化也触发 OnRep，保证新连接客户端收到正确初始值
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UApecoxVitalAttributeSet, ShieldEvolutionPoints, COND_None, REPNOTIFY_Always);
}

void UApecoxVitalAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, Health, OldHealth);

	// 客户端通过 OnRep 感知 Health 变化，广播本地表现事件
	// 客户端不得由此生成权威死亡 GameplayEvent——该职责仅属于 Authority 的 PostGameplayEffectExecute
	const float OldValue = OldHealth.GetCurrentValue();
	const float NewValue = Health.GetCurrentValue();
	OnHealthChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);

	// 客户端同步 bOutOfHealth：Health > 0 时必须恢复 false，
	// 否则复活后（未经过 PostAttributeChange Authority 路径）仍显示死亡状态
	bOutOfHealth = (NewValue <= 0.0f);
}

void UApecoxVitalAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, MaxHealth, OldMaxHealth);

	const float OldValue = OldMaxHealth.GetCurrentValue();
	const float NewValue = MaxHealth.GetCurrentValue();
	OnMaxHealthChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);
}

void UApecoxVitalAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, Shield, OldShield);
	const float OldValue = OldShield.GetCurrentValue();
	const float NewValue = Shield.GetCurrentValue();
	OnShieldChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);
}

void UApecoxVitalAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, MaxShield, OldMaxShield);
	const float OldValue = OldMaxShield.GetCurrentValue();
	const float NewValue = MaxShield.GetCurrentValue();
	OnMaxShieldChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);
}

void UApecoxVitalAttributeSet::OnRep_ShieldEvolutionPoints(const FGameplayAttributeData& OldPoints)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UApecoxVitalAttributeSet, ShieldEvolutionPoints, OldPoints);
	const float OldValue = OldPoints.GetCurrentValue();
	const float NewValue = ShieldEvolutionPoints.GetCurrentValue();
	OnShieldEvolutionPointsChanged.Broadcast(nullptr, nullptr, nullptr, NewValue - OldValue, OldValue, NewValue);
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
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
	}
	else if (Attribute == GetShieldEvolutionPointsAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
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

	// 只负责数值钳制；GE 前的旧值快照由 PreGameplayEffectExecute 统一处理，
	// 避免 PostGameplayEffectExecute 内 SetHealth() 再次进入此函数覆盖已保存的旧值
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
			// 通过 ASC 的标准属性修改入口把 Health 基础值压回新上限
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ensure(ASC))
			{
				ASC->SetNumericAttributeBase(GetHealthAttribute(), NewValue);
			}
		}
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		const float CurrentShield = GetShield();
		if (CurrentShield > NewValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->SetNumericAttributeBase(GetShieldAttribute(), NewValue);
			}
		}
	}

	// Health 恢复到正数后重置 bOutOfHealth，允许下一次死亡再次广播
	// 此路径在 PostGameplayEffectExecute 的委托广播后方被调用，
	// 因此不会在 GE 回调内提前清除刚设置的 bOutOfHealth
	if (Attribute == GetHealthAttribute() && NewValue > 0.0f)
	{
		bOutOfHealth = false;
	}
}

// --- GE 前后值链路 ---

bool UApecoxVitalAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	// 在 GE Modifier 执行前统一保存 Health 和 MaxHealth 旧值。
	// 不使用 PreAttributeChange 保存是因为 PostGameplayEffectExecute 内的 SetHealth()
	// 会再次经过 PreAttributeChange，覆盖已保存的旧值。
	HealthBeforeAttributeChange = GetHealth();
	MaxHealthBeforeAttributeChange = GetMaxHealth();
	ShieldBeforeAttributeChange = GetShield();
	MaxShieldBeforeAttributeChange = GetMaxShield();
	ShieldEvolutionPointsBeforeAttributeChange = GetShieldEvolutionPoints();

	return true;
}

void UApecoxVitalAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 1. 针对当前被修改属性完成必要钳制。
	//    MaxHealth 已在 PreAttributeChange 中通过 ClampAttribute 完成钳制。
	float AppliedDamage = 0.0f;
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && Data.EvaluatedData.Magnitude < 0.0f)
	{
		// 正式伤害 GE 仍修改 Health；在唯一权威结算点把负变化重新分配为护盾优先。
		// 使用 Modifier 原始幅度，而不是已经被 0 下限截断的 Health 差，才能正确处理护盾和溢出伤害。
		const float RequestedDamage = -Data.EvaluatedData.Magnitude;
		const float ShieldDamage = FMath::Min(ShieldBeforeAttributeChange, RequestedDamage);
		const float HealthDamage = FMath::Min(
			HealthBeforeAttributeChange, FMath::Max(RequestedDamage - ShieldDamage, 0.0f));
		SetShield(ShieldBeforeAttributeChange - ShieldDamage);
		SetHealth(HealthBeforeAttributeChange - HealthDamage);
		AppliedDamage = ShieldDamage + HealthDamage;
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float ClampedHealth = FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth());
		if (GetHealth() != ClampedHealth)
		{
			SetHealth(ClampedHealth);
		}
	}

	// 2. 用 GE 前快照与当前最终值统一比较，按实际变化广播。
	//    MaxHealth 降低连带压低 Health 已由 PostAttributeChange 的
	//    SetNumericAttributeBase 处理；进入此处时 Health 已是压低后的最终值。
	//    快照比较能正确检测这一连带变化，不再依赖 CurrentHealth > NewMaxHealth 推断。

	AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
	AActor* Causer = Data.EffectSpec.GetEffectContext().GetEffectCauser();

	const float OldPoints = ShieldEvolutionPointsBeforeAttributeChange;
	const float NewPoints = GetShieldEvolutionPoints();
	if (NewPoints != OldPoints)
	{
		OnShieldEvolutionPointsChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, NewPoints - OldPoints, OldPoints, NewPoints);
	}

	const float OldMaxShield = MaxShieldBeforeAttributeChange;
	const float NewMaxShield = GetMaxShield();
	if (NewMaxShield != OldMaxShield)
	{
		OnMaxShieldChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, NewMaxShield - OldMaxShield, OldMaxShield, NewMaxShield);
	}

	const float OldShield = ShieldBeforeAttributeChange;
	const float NewShield = GetShield();
	if (NewShield != OldShield)
	{
		OnShieldChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, NewShield - OldShield, OldShield, NewShield);
	}

	const float OldMax = MaxHealthBeforeAttributeChange;
	const float NewMax = GetMaxHealth();
	if (NewMax != OldMax)
	{
		OnMaxHealthChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, NewMax - OldMax, OldMax, NewMax);
	}

	const float OldHealth = HealthBeforeAttributeChange;
	const float NewHealth = GetHealth();
	if (NewHealth != OldHealth)
	{
		OnHealthChanged.Broadcast(
			Instigator, Causer, &Data.EffectSpec, NewHealth - OldHealth, OldHealth, NewHealth);
	}

	// 只有玩家伤害敌对目标时获得进化点。AI 仍有 PlayerState/ASC 以复用伤亡链路，
	// 但 CombatTeam=AI 会在这里明确排除其成长。
	if (AppliedDamage > 0.0f)
	{
		AApecoxPlayerState* SourcePlayerState = ResolveApecoxPlayerState(Instigator);
		AApecoxPlayerState* TargetPlayerState = nullptr;
		if (const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			TargetPlayerState = Cast<AApecoxPlayerState>(ASC->GetOwnerActor());
		}
		if (SourcePlayerState && TargetPlayerState && SourcePlayerState != TargetPlayerState
			&& SourcePlayerState->GetCombatTeam() == EApecoxCombatTeam::Players
			&& SourcePlayerState->GetCombatTeam() != TargetPlayerState->GetCombatTeam())
		{
			SourcePlayerState->AddShieldEvolutionPoints(AppliedDamage);
		}
	}

	// 3. OutOfHealth 判断在 OnHealthChanged 回调后重新读取实时 Health，
	//    防止回调中实现免死/治疗/最后机会效果已使 Health 恢复正数仍触发死亡。
	//    只在从正数跨入 <=0 时广播，避免新 Pawn 初始 0 属性误触发。
	const float RealTimeHealth = GetHealth();
	const bool bShouldBeOutOfHealth = (RealTimeHealth <= 0.0f);

	if (bShouldBeOutOfHealth && OldHealth > 0.0f && !bOutOfHealth)
	{
		OnOutOfHealth.Broadcast(
			Instigator, Causer, &Data.EffectSpec,
			RealTimeHealth - OldHealth, OldHealth, RealTimeHealth);
	}

	// 4. 所有委托回调完成后，用实时最终值更新门控。
	//    若回调已使 Health 恢复到正数，后续再次归零仍可正常广播死亡。
	bOutOfHealth = bShouldBeOutOfHealth;
}
