// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Weapons/ApecoxWeaponGameplayAbility.h"
#include "Weapons/ApecoxWeaponStateComponent.h"
#include "ApecoxRangedFireAbility.generated.h"

struct FApecoxRangedShotTargetData;
class UApecoxRangedWeaponInstance;

/** One activation = one shot. Shared input, prediction, validation and delegate cleanup.
 * Authority dispatches to a model only after verifying the equipped weapon and aim.
 */
UCLASS(Abstract)
class APECOX_API UApecoxRangedFireAbility : public UApecoxWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	UApecoxRangedFireAbility();

	// ========================================================================
	// 生命周期
	// ========================================================================

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// ========================================================================
	// TargetData 回调
	// ========================================================================

	/** TargetData 到达回调——客户端和服务器各自处理 */
	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	// ========================================================================
	// 客户端
	// ========================================================================

	/**
	 * Owning Local Player 堆分配并填充 FApecoxRangedShotTargetData。
	 * 包含本地候选 Trace 结果——仅供预测/诊断，不参与权威命中结算。
	 * @return 堆分配对象，调用方编入 TargetDataHandle 以移交生命周期。
	 */
	FApecoxRangedShotTargetData* BuildLocalShotTargetData(float WorldTimeSeconds) const;

	// ========================================================================
	// 共享 Cue 辅助
	// ========================================================================

	/**
	 * 空指针安全的 Fire Cue 执行辅助函数。
	 * - Owning Client 在 FScopedPredictionWindow 内预测执行。
	 * - Authority 在 CommitServerShot 成功后执行一次以广播给 Remote Client。
	 * - 两端使用同一 PredictionKey，GAS 自动为 Owning Client 去重。
	 * - Listen Host 只走 Authority 执行，不走客户端预测。
	 */
	void ExecuteWeaponFireCue(const FGameplayAbilityActorInfo* ActorInfo,
		const UApecoxRangedWeaponInstance* RangedWeaponInstance) const;

	// ========================================================================
	// 服务器验证与结算
	// ========================================================================

	/**
	 * Authority验证装备、ShotId、RPM、瞄准意图与具体模型配置。
	 * 通过后交给ExecuteValidatedShot，模型负责扣弹并发射/结算。
	 * 所有拒绝路径通过 UE_LOG + SendShotConfirmation 记录，不扣弹。
	 */
	void ProcessAuthoritativeShot(const FApecoxRangedShotTargetData& ShotData,
		UApecoxRangedWeaponInstance* RangedWeaponInstance);

    virtual bool IsFireConfigValid(const UApecoxRangedWeaponInstance* Weapon) const PURE_VIRTUAL(UApecoxRangedFireAbility::IsFireConfigValid, return false;);
    virtual void ExecuteValidatedShot(const FApecoxRangedShotTargetData& ShotData,
        UApecoxRangedWeaponInstance* Weapon) PURE_VIRTUAL(UApecoxRangedFireAbility::ExecuteValidatedShot, );

	/** 通过 WeaponStateComponent 发送射击确认 */
	void SendShotConfirmation(uint32 ShotId, EApecoxShotConfirmation Result) const;

	/** 获取 WeaponStateComponent——空指针安全 */
	UApecoxWeaponStateComponent* GetWeaponStateComponent(const FGameplayAbilityActorInfo* ActorInfo) const;

private:
	/** TargetData 回调委托句柄——EndAbility 必须对称移除 */
	FDelegateHandle TargetDataDelegateHandle;

	/** 本次激活的 Shot ID——用于日志和诊断 */
	uint32 CurrentShotId = 0;
};
