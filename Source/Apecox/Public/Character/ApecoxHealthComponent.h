// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ApecoxHealthComponent.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxVitalAttributeSet;
struct FGameplayEffectSpec;

// 属性变化委托：六参数，直接转发 VitalAttributeSet 的同名委托，
// 接收方无需持有 AttributeSet 引用即可监听 Health/MaxHealth 变化
DECLARE_MULTICAST_DELEGATE_SixParams(FApecoxHealthAttributeEvent,
	AActor* /*EffectInstigator*/,
	AActor* /*EffectCauser*/,
	const FGameplayEffectSpec* /*EffectSpec*/,
	float /*EffectMagnitude*/,
	float /*OldValue*/,
	float /*NewValue*/
);

/**
 * EApecoxDeathState
 * - 死亡状态枚举，由 HealthComponent 管理和复制
 * - NotDead -> DeathStarted：生命归零，开始死亡流程
 * - DeathStarted -> DeathFinished：死亡流程结束，旧 Pawn 可以处理
 * - 不允许逆向；新 Pawn 使用新组件实例恢复 NotDead
 */
UENUM(BlueprintType)
enum class EApecoxDeathState : uint8
{
	NotDead			UMETA(DisplayName = "Not Dead"),
	DeathStarted	UMETA(DisplayName = "Death Started"),
	DeathFinished	UMETA(DisplayName = "Death Finished")
};

// 死亡状态变化委托，由 Character 绑定以处理物理身体
DECLARE_MULTICAST_DELEGATE_OneParam(FApecoxDeathStateEvent, AActor* /*OwningActor*/);

/**
 * UApecoxHealthComponent
 * - 管理当前 Pawn 的生命监听和死亡状态转换
 * - 不保存第二份 Health/MaxHealth——属性只存在于 ASC 的 VitalAttributeSet
 * - 将 AttributeSet 的数值事件翻译为 Pawn 死亡状态
 * - Authority 负责发送 GameplayEvent.Death，所有端通过复制 DeathState 同步
 */
UCLASS(Blueprintable, ClassGroup = (Apecox), meta = (BlueprintSpawnableComponent))
class APECOX_API UApecoxHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApecoxHealthComponent();

	// --- UActorComponent ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnUnregister() override;

	// --- 初始化/反初始化 ---

	/** 绑定 ASC 和 VitalAttributeSet 的委托；已绑定同一 ASC+VitalSet 时直接返回（幂等） */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	void InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC);

	/** 幂等解绑所有委托；只有在 ASC Avatar 仍是此 Owner 时才清理死亡 Tag */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	void UninitializeFromAbilitySystem();

	// --- 只读查询 ---

	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	float GetMaxHealth() const;

	/** 返回 0.0 ~ 1.0；各端可用（通过复制属性计算，不依赖本地 ASC 状态） */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	EApecoxDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	bool IsDeadOrDying() const { return DeathState != EApecoxDeathState::NotDead; }

	// --- 死亡状态转换（仅由 DeathAbility 调用） ---

	/** 设置 DeathStarted，添加 State.Death.Dying；仅 Authority 执行，幂等 */
	void StartDeath();

	/** 设置 DeathFinished，State.Death.Dying -> State.Death.Dead；仅 Authority 执行，幂等 */
	void FinishDeath();

	// --- 委托 ---

	/** DeathState 转换到 DeathStarted 时广播 */
	FApecoxDeathStateEvent OnDeathStarted;

	/** DeathState 转换到 DeathFinished 时广播 */
	FApecoxDeathStateEvent OnDeathFinished;

	/** 直接转发 VitalAttributeSet::OnHealthChanged 的六参数委托 */
	FApecoxHealthAttributeEvent OnHealthChanged;

	/** 直接转发 VitalAttributeSet::OnMaxHealthChanged 的六参数委托 */
	FApecoxHealthAttributeEvent OnMaxHealthChanged;

protected:
	// --- AttributeSet 回调 ---

	void HandleHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
		const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
		float OldValue, float NewValue);

	void HandleMaxHealthChanged(AActor* EffectInstigator, AActor* EffectCauser,
		const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
		float OldValue, float NewValue);

	/** 仅在 Authority 上发送 GameplayEvent.Death；Target 为当前 AvatarActor */
	void HandleOutOfHealth(AActor* EffectInstigator, AActor* EffectCauser,
		const FGameplayEffectSpec* EffectSpec, float EffectMagnitude,
		float OldValue, float NewValue);

	// --- 复制 ---

	UFUNCTION()
	void OnRep_DeathState(EApecoxDeathState OldDeathState);

	/**
	 * 根据当前 DeathState 在各端本地重建/清理死亡 Tag
	 * Tag 不额外独立复制——由复制的 DeathState 驱动
	 */
	void ClearDeathTags();
	void ApplyDeathTagsForState();

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EApecoxDeathState DeathState = EApecoxDeathState::NotDead;

private:
	// ASC 和 VitalSet 引用不由组件拥有；它们属于 PlayerState
	UPROPERTY()
	TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UApecoxVitalAttributeSet> VitalAttributeSet;

	// 追踪已绑定的 ASC，用于 InitializeWithAbilitySystem 判断是否已完整初始化
	UPROPERTY()
	TObjectPtr<UApecoxAbilitySystemComponent> BoundASC;
};
