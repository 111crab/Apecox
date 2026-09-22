// Copyright Apecox. All Rights Reserved.

#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "Net/UnrealNetwork.h"

AApecoxPlayerState::AApecoxPlayerState()
{
	// 创建 ASC 作为 PlayerState 的默认子对象——PlayerState 是 ASC 的唯一 Owner
	AbilitySystemComponent = CreateDefaultSubobject<UApecoxAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Mixed 复制：拥有客户端收到完整 Active GE（用于 HUD/冷却/预测确认），
	// 远端仅收到 Cue 和公开 Attribute，降低带宽
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// VitalAttributeSet 同样由 PlayerState 拥有，确保属性数据跨 Pawn 生命周期持久
	VitalAttributeSet = CreateDefaultSubobject<UApecoxVitalAttributeSet>(TEXT("VitalAttributeSet"));

	// APlayerState 默认 NetUpdateFrequency=1 Hz，不足以承载放置在 PlayerState 上的
	// ASC/Attribute 复制需求；100 Hz 是当前小规模多人原型基线，不是玩法属性
	SetNetUpdateFrequency(100.0f);
}

void AApecoxPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApecoxPlayerState, Kills);
	DOREPLIFETIME(AApecoxPlayerState, Deaths);
	DOREPLIFETIME(AApecoxPlayerState, CombatTeam);
}

void AApecoxPlayerState::AddKill()
{
	if (HasAuthority())
	{
		++Kills;
		ForceNetUpdate();
	}
}

void AApecoxPlayerState::AddDeath()
{
	if (HasAuthority())
	{
		++Deaths;
		ForceNetUpdate();
	}
}

void AApecoxPlayerState::SetCombatTeam(EApecoxCombatTeam NewTeam)
{
	if (HasAuthority() && CombatTeam != NewTeam)
	{
		CombatTeam = NewTeam;
		if (CombatTeam == EApecoxCombatTeam::AI)
		{
			InitializeCombatAttributesForPawn(false);
		}
		ForceNetUpdate();
	}
}

float AApecoxPlayerState::GetShield() const
{
	return VitalAttributeSet ? VitalAttributeSet->GetShield() : 0.0f;
}

float AApecoxPlayerState::GetMaxShield() const
{
	return VitalAttributeSet ? VitalAttributeSet->GetMaxShield() : 0.0f;
}

float AApecoxPlayerState::GetShieldEvolutionPoints() const
{
	return VitalAttributeSet ? VitalAttributeSet->GetShieldEvolutionPoints() : 0.0f;
}

EApecoxShieldTier AApecoxPlayerState::GetShieldTier() const
{
	const float Points = GetShieldEvolutionPoints();
	if (Points >= PurpleEvolutionThreshold)
	{
		return EApecoxShieldTier::Purple;
	}
	if (Points >= BlueEvolutionThreshold)
	{
		return EApecoxShieldTier::Blue;
	}
	return EApecoxShieldTier::White;
}

float AApecoxPlayerState::GetShieldEvolutionPointsToNextTier() const
{
	switch (GetShieldTier())
	{
	case EApecoxShieldTier::White:
		return FMath::Max(BlueEvolutionThreshold - GetShieldEvolutionPoints(), 0.0f);
	case EApecoxShieldTier::Blue:
		return FMath::Max(PurpleEvolutionThreshold - GetShieldEvolutionPoints(), 0.0f);
	case EApecoxShieldTier::Purple:
	default:
		return 0.0f;
	}
}

void AApecoxPlayerState::InitializeCombatAttributesForPawn(bool bEnablePlayerShield)
{
	if (!HasAuthority() || !AbilitySystemComponent || !VitalAttributeSet)
	{
		return;
	}

	if (!bEnablePlayerShield || CombatTeam == EApecoxCombatTeam::AI)
	{
		AbilitySystemComponent->SetNumericAttributeBase(
			UApecoxVitalAttributeSet::GetShieldEvolutionPointsAttribute(), 0.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetMaxShieldAttribute(), 0.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetShieldAttribute(), 0.0f);
		ForceNetUpdate();
		return;
	}

	const float Points = FMath::Clamp(GetShieldEvolutionPoints(), 0.0f, PurpleEvolutionThreshold);
	AbilitySystemComponent->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetShieldEvolutionPointsAttribute(), Points);
	const float DesiredMaxShield = Points >= PurpleEvolutionThreshold ? PurpleShieldValue
		: (Points >= BlueEvolutionThreshold ? BlueShieldValue : WhiteShieldValue);
	AbilitySystemComponent->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetMaxShieldAttribute(), DesiredMaxShield);
	AbilitySystemComponent->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetShieldAttribute(), DesiredMaxShield);
	ForceNetUpdate();
}

float AApecoxPlayerState::AddShieldEvolutionPoints(float Amount)
{
	if (!HasAuthority() || CombatTeam != EApecoxCombatTeam::Players || !AbilitySystemComponent
		|| !VitalAttributeSet || !FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return 0.0f;
	}

	const float OldPoints = FMath::Clamp(GetShieldEvolutionPoints(), 0.0f, PurpleEvolutionThreshold);
	const float NewPoints = FMath::Clamp(OldPoints + Amount, 0.0f, PurpleEvolutionThreshold);
	if (NewPoints <= OldPoints)
	{
		return 0.0f;
	}

	const float OldMaxShield = GetMaxShield();
	const float NewMaxShield = NewPoints >= PurpleEvolutionThreshold ? PurpleShieldValue
		: (NewPoints >= BlueEvolutionThreshold ? BlueShieldValue : WhiteShieldValue);
	AbilitySystemComponent->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetShieldEvolutionPointsAttribute(), NewPoints);
	if (NewMaxShield > OldMaxShield)
	{
		// 升级时新增的 25 点容量立即可用；已有受损部分不会被伤害成长自动补满。
		const float AddedCapacity = NewMaxShield - OldMaxShield;
		AbilitySystemComponent->SetNumericAttributeBase(
			UApecoxVitalAttributeSet::GetMaxShieldAttribute(), NewMaxShield);
		AbilitySystemComponent->SetNumericAttributeBase(
			UApecoxVitalAttributeSet::GetShieldAttribute(),
			FMath::Min(GetShield() + AddedCapacity, NewMaxShield));
	}
	ForceNetUpdate();
	return NewPoints - OldPoints;
}

bool AApecoxPlayerState::TryApplyHealthPickup(float RestoreAmount)
{
	if (!HasAuthority() || CombatTeam != EApecoxCombatTeam::Players || !AbilitySystemComponent
		|| !VitalAttributeSet || !FMath::IsFinite(RestoreAmount) || RestoreAmount <= 0.0f)
	{
		return false;
	}
	const float OldHealth = VitalAttributeSet->GetHealth();
	const float NewHealth = FMath::Min(OldHealth + RestoreAmount, VitalAttributeSet->GetMaxHealth());
	if (NewHealth <= OldHealth)
	{
		return false;
	}
	AbilitySystemComponent->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetHealthAttribute(), NewHealth);
	ForceNetUpdate();
	return true;
}

bool AApecoxPlayerState::TryApplyShieldBattery(float EvolutionPoints)
{
	if (!HasAuthority() || CombatTeam != EApecoxCombatTeam::Players || !AbilitySystemComponent
		|| !VitalAttributeSet || !FMath::IsFinite(EvolutionPoints) || EvolutionPoints < 0.0f)
	{
		return false;
	}

	const float OldPoints = GetShieldEvolutionPoints();
	const float OldShield = GetShield();
	AddShieldEvolutionPoints(EvolutionPoints);
	const float NewMaxShield = GetMaxShield();
	if (NewMaxShield > 0.0f)
	{
		AbilitySystemComponent->SetNumericAttributeBase(
			UApecoxVitalAttributeSet::GetShieldAttribute(), NewMaxShield);
	}
	const bool bChanged = GetShieldEvolutionPoints() > OldPoints || GetShield() > OldShield;
	if (bChanged)
	{
		ForceNetUpdate();
	}
	return bChanged;
}

UAbilitySystemComponent* AApecoxPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UApecoxAbilitySystemComponent* AApecoxPlayerState::GetApecoxAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

const UApecoxVitalAttributeSet* AApecoxPlayerState::GetVitalAttributeSet() const
{
	return VitalAttributeSet;
}
