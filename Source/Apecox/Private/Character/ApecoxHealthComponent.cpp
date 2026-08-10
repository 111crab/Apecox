// Copyright Apecox. All Rights Reserved.

#include "Character/ApecoxHealthComponent.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

UApecoxHealthComponent::UApecoxHealthComponent()
{
	// 默认复制：DeathState 需要在所有相关端同步
	SetIsReplicatedByDefault(true);

	// 不 Tick——所有工作由委托驱动
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UApecoxHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UApecoxHealthComponent, DeathState);
}

void UApecoxHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	Super::OnUnregister();
}

// --- 初始化/反初始化 ---

void UApecoxHealthComponent::InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	// 幂等：同一个 ASC 且 VitalSet 已完整绑定时直接返回，避免重复 AddUObject
	if (BoundASC == InASC && VitalAttributeSet != nullptr)
	{
		return;
	}

	// 其他已有绑定先完全反初始化
	if (BoundASC)
	{
		UninitializeFromAbilitySystem();
	}

	const UApecoxVitalAttributeSet* VitalSet = InASC->GetSet<UApecoxVitalAttributeSet>();
	if (!VitalSet)
	{
		// 找不到 VitalSet——不留下 ASC 非空但未完成初始化的半状态
		return;
	}

	BoundASC = InASC;
	AbilitySystemComponent = InASC;
	VitalAttributeSet = VitalSet;

	// 绑定 AttributeSet 委托，接收生命值变化
	VitalAttributeSet->OnHealthChanged.AddUObject(this, &UApecoxHealthComponent::HandleHealthChanged);
	VitalAttributeSet->OnMaxHealthChanged.AddUObject(this, &UApecoxHealthComponent::HandleMaxHealthChanged);
	VitalAttributeSet->OnOutOfHealth.AddUObject(this, &UApecoxHealthComponent::HandleOutOfHealth);
}

void UApecoxHealthComponent::UninitializeFromAbilitySystem()
{
	if (VitalAttributeSet)
	{
		VitalAttributeSet->OnHealthChanged.RemoveAll(this);
		VitalAttributeSet->OnMaxHealthChanged.RemoveAll(this);
		VitalAttributeSet->OnOutOfHealth.RemoveAll(this);
		VitalAttributeSet = nullptr;
	}

	// 只有在 ASC Avatar 仍是本组件 Owner 时才清理死亡 Tag
	// 防止旧 Pawn 晚解绑时错误清除新 Pawn 已建立的死亡状态
	if (AbilitySystemComponent)
	{
		const AActor* Owner = GetOwner();
		if (Owner && AbilitySystemComponent->GetAvatarActor() == Owner)
		{
			ClearDeathTags();
		}
		AbilitySystemComponent = nullptr;
	}

	BoundASC = nullptr;
}

// --- 只读查询 ---

float UApecoxHealthComponent::GetHealth() const
{
	if (VitalAttributeSet)
	{
		return VitalAttributeSet->GetHealth();
	}
	return 0.0f;
}

float UApecoxHealthComponent::GetMaxHealth() const
{
	if (VitalAttributeSet)
	{
		return VitalAttributeSet->GetMaxHealth();
	}
	return 0.0f;
}

float UApecoxHealthComponent::GetHealthNormalized() const
{
	const float MaxH = GetMaxHealth();
	if (MaxH > 0.0f)
	{
		return FMath::Clamp(GetHealth() / MaxH, 0.0f, 1.0f);
	}
	return 0.0f;
}

// --- 死亡状态转换 ---

void UApecoxHealthComponent::StartDeath()
{
	// 仅 Authority 驱动死亡状态转换
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// 幂等：已经进入死亡流程，不重复广播
	if (DeathState != EApecoxDeathState::NotDead)
	{
		return;
	}

	DeathState = EApecoxDeathState::DeathStarted;
	ApplyDeathTagsForState();

	OnDeathStarted.Broadcast(GetOwner());

	// 状态修改后强制网络更新——首版中 NotDead→DeathStarted→DeathFinished
	// 可能在同一帧完成，没有强制更新时客户端可能完全错过 DeathState
	GetOwner()->ForceNetUpdate();
}

void UApecoxHealthComponent::FinishDeath()
{
	// 仅 Authority 驱动死亡状态转换
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	// 幂等：只有 DeathStarted 才能转入 DeathFinished
	if (DeathState != EApecoxDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EApecoxDeathState::DeathFinished;
	ApplyDeathTagsForState();

	OnDeathFinished.Broadcast(GetOwner());

	GetOwner()->ForceNetUpdate();
}

// --- AttributeSet 回调 ---

void UApecoxHealthComponent::HandleHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
	float OldValue, float NewValue)
{
	// 原样转发 AttributeSet 的六参数委托，作为组件级监听入口
	OnHealthChanged.Broadcast(EffectInstigator, EffectCauser, EffectSpec, EffectMagnitude, OldValue, NewValue);
}

void UApecoxHealthComponent::HandleMaxHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
	float OldValue, float NewValue)
{
	// 原样转发 AttributeSet 的六参数委托
	OnMaxHealthChanged.Broadcast(EffectInstigator, EffectCauser, EffectSpec, EffectMagnitude, OldValue, NewValue);
}

void UApecoxHealthComponent::HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser,
	const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
	float OldValue, float NewValue)
{
	// 只有 Authority 发送权威死亡 GameplayEvent
	// 客户端通过 OnRep_DeathState 感知死亡状态，不在此生成事件
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	// 发送 GameplayEvent.Death 触发 DeathAbility
	// Target 使用当前 AvatarActor（死亡角色），不使用 OwnerActor（PlayerState）
	FGameplayEventData EventData;
	EventData.Instigator = EffectInstigator;
	EventData.Target = AbilitySystemComponent->GetAvatarActor();
	EventData.EventTag = ApecoxGameplayTags::GameplayEvent_Death;
	EventData.EventMagnitude = EffectMagnitude;

	AbilitySystemComponent->HandleGameplayEvent(
		ApecoxGameplayTags::GameplayEvent_Death,
		&EventData);
}

// --- 复制 ---

void UApecoxHealthComponent::OnRep_DeathState(EApecoxDeathState OldDeathState)
{
	if (DeathState == OldDeathState)
	{
		return;
	}

	// 先重建本地 Tag，客户端委托回调观察到的 Tag 状态与 Authority 一致
	ApplyDeathTagsForState();

	// OnRep 必须处理复制合并导致的 NotDead -> DeathFinished 跳跃
	if (OldDeathState == EApecoxDeathState::NotDead)
	{
		if (DeathState == EApecoxDeathState::DeathStarted)
		{
			// 正常序列：NotDead -> DeathStarted
			OnDeathStarted.Broadcast(GetOwner());
		}
		else if (DeathState == EApecoxDeathState::DeathFinished)
		{
			// 合并复制：NotDead -> DeathFinished
			// 依次广播 Started 和 Finished，另一端看到完整死亡序列
			OnDeathStarted.Broadcast(GetOwner());
			OnDeathFinished.Broadcast(GetOwner());
		}
	}
	else if (OldDeathState == EApecoxDeathState::DeathStarted &&
		DeathState == EApecoxDeathState::DeathFinished)
	{
		OnDeathFinished.Broadcast(GetOwner());
	}
}

void UApecoxHealthComponent::ClearDeathTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 使用 SetLooseGameplayTagCount(Tag, 0) 明确计数为 0，
	// 避免 Add/Remove 计数被重复路径污染
	AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dying, 0);
	AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dead, 0);
}

void UApecoxHealthComponent::ApplyDeathTagsForState()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	switch (DeathState)
	{
	case EApecoxDeathState::DeathStarted:
		// State.Death.Dying count=1 → 父标签 State.Death 自动生效
		// 所有普通 GA 的 ActivationBlockedTags 包含 State.Death，因此被阻止
		AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dead, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dying, 1);
		break;

	case EApecoxDeathState::DeathFinished:
		// State.Death.Dead count=1 → Dying/Dead 互斥
		AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(ApecoxGameplayTags::State_Death_Dead, 1);
		break;

	case EApecoxDeathState::NotDead:
	default:
		// 两者均清零
		ClearDeathTags();
		break;
	}
}
