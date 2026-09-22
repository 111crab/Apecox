// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ApecoxWanderAIController.generated.h"

/**
 * Apecox PvE 的最小服务器战斗 AI。
 * 类名为兼容已经保存的 BP_ApecoxBotCharacter 序列化路径保留。
 */
UCLASS()
class APECOX_API AApecoxWanderAIController : public AAIController
{
	GENERATED_BODY()

public:
	AApecoxWanderAIController();
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Apecox|AI")
	float GetAcquireRange() const { return AcquireRange; }

	UFUNCTION(BlueprintPure, Category = "Apecox|AI")
	float GetFireRange() const { return FireRange; }

protected:
	void UpdateCombatDecision();
	void UpdateCombatMovement(class AApecoxBotCharacter* Bot,
		class AApecoxPlayerCharacter* Target);
	void UpdateAimFocus(const class AApecoxBotCharacter* Bot,
		const class AApecoxPlayerCharacter* Target);
	void BeginFireBurst(class AApecoxBotCharacter* Bot,
		class AApecoxPlayerCharacter* Target);
	void EndFireBurst(class AApecoxBotCharacter* Bot, bool bStartPause);
	class AApecoxPlayerCharacter* FindNearestLivingPlayer();
	void UpdateObjectiveMovement(class AApecoxBotCharacter* Bot);
	class AApecoxAIObjectivePoint* ChooseObjectivePoint();
	void ClearCombatTarget(class AApecoxBotCharacter* Bot);
	void StopCombat();

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI", meta = (ClampMin = "100.0"))
	float AcquireRange = 2400.0f;

	/** 已锁定目标超过此距离立即脱战，防止 AI 追遍整张地图。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI", meta = (ClampMin = "100.0"))
	float LoseTargetRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI", meta = (ClampMin = "100.0"))
	float FireRange = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI", meta = (ClampMin = "0.05"))
	float DecisionInterval = 0.15f;

	/** 已锁定目标短暂离开视线时允许追踪的时间，超时后返回争夺点。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI", meta = (ClampMin = "0.0", Units = "s"))
	float LostSightGraceSeconds = 2.0f;

	/** 到达争夺点后改变环视方向的间隔。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Objective", meta = (ClampMin = "0.1", Units = "s"))
	float ObjectiveLookInterval = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Objective", meta = (ClampMin = "100.0", Units = "cm"))
	float ObjectiveLookDistance = 1000.0f;

	/** 每个新目标独立随机反应时间，避免出生后同帧齐射。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.0"))
	float ReactionDelayMin = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.0"))
	float ReactionDelayMax = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.05"))
	float FireBurstDurationMin = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.05"))
	float FireBurstDurationMax = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.0"))
	float FirePauseMin = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Fire", meta = (ClampMin = "0.0"))
	float FirePauseMax = 0.8f;

	/** 每轮点射保持一个稳定偏差，下一轮重新取样；单位是目标位置附近的厘米。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Accuracy", meta = (ClampMin = "0.0", Units = "cm"))
	float AimErrorHorizontal = 85.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Accuracy", meta = (ClampMin = "0.0", Units = "cm"))
	float AimErrorVertical = 45.0f;

	/** 射程内仍随机换位，避免持枪 AI 退化为同步横移的炮台。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "50.0", Units = "cm"))
	float CombatMoveDistanceMin = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "50.0", Units = "cm"))
	float CombatMoveDistanceMax = 600.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "0.25"))
	float CombatRepositionIntervalMin = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "0.25"))
	float CombatRepositionIntervalMax = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "100.0", Units = "cm"))
	float PreferredCombatDistanceMin = 900.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|AI|Movement", meta = (ClampMin = "100.0", Units = "cm"))
	float PreferredCombatDistanceMax = 1500.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<class AApecoxPlayerCharacter> CombatTarget;

	UPROPERTY(Transient)
	TObjectPtr<class AApecoxAIObjectivePoint> CurrentObjective;

	float DecisionTimeRemaining = 0.0f;
	float CombatRepositionTimeRemaining = 0.0f;
	float ReactionTimeRemaining = 0.0f;
	float FireBurstTimeRemaining = 0.0f;
	float FirePauseTimeRemaining = 0.0f;
	float LostSightTime = 0.0f;
	float ObjectiveSearchTimeRemaining = 0.0f;
	float ObjectiveLookTimeRemaining = 0.0f;
	bool bFireBurstActive = false;
	bool bSearchingObjective = false;
	FVector CurrentAimOffset = FVector::ZeroVector;
	FRandomStream CombatRandom;
};
