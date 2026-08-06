// Copyright Apecox. All Rights Reserved.

#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"

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
