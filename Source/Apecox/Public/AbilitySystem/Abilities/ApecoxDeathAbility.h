// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ApecoxGameplayAbility.h"
#include "ApecoxDeathAbility.generated.h"

class UApecoxHealthComponent;

/**
 * UApecoxDeathAbility
 * - 响应 GameplayEvent.Death 的排他死亡 Ability
 * - ServerInitiated：死亡由服务器权威触发，客户端等待复制确认
 * - ExclusiveBlocking：阻止其他排他 Ability 在死亡期间激活
 * - 清空输入、取消其他活动 Ability、设置不可取消
 * - 首版 bAutoFinishDeath=true，未来可由死亡蒙太奇结束时调用 Finish
 * - Abstract + Blueprintable：以后创建 GA_Apecox_Death 蓝图子类
 */
UCLASS(Abstract, Blueprintable)
class APECOX_API UApecoxDeathAbility : public UApecoxGameplayAbility
{
	GENERATED_BODY()

public:
	UApecoxDeathAbility();

	// --- UGameplayAbility 覆写 ---
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * 蓝图可调用：完成死亡流程并结束此 Ability
	 * 当前 bAutoFinishDeath=true 时由 ActivateAbility 自动调用；
	 * 未来关闭后可由死亡蒙太奇结束回调调用
	 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Death")
	void FinishDeathAndEndAbility();

protected:
	/** 蓝图事件：死亡开始后立即调用，为未来死亡蒙太奇提供出口 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Apecox|Death", meta = (DisplayName = "OnDeathStarted"))
	void K2_OnDeathStarted();

	/** 内部：取消除自身外的所有活动 Ability，并清空 ASC 输入 */
	void CancelOtherAbilitiesAndClearInput();

private:
	// 首版 true：激活后立即完成死亡 -> EndAbility
	// 未来设为 false 时由 K2_OnDeathStarted 中播放的蒙太奇结束回调调用 Finish
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Death")
	bool bAutoFinishDeath = true;

	// 防止多次调用 FinishDeathAndEndAbility / EndAbility
	bool bDeathFinished = false;

	// 标记死亡流程已成功开始；EndAbility 只在 bDeathStarted 为真时才补调用 FinishDeath，
	// 避免激活前验证失败也伪装成一次完整死亡
	bool bDeathStarted = false;
};
