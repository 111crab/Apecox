// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "ApecoxPlayerState.generated.h"

UENUM(BlueprintType)
enum class EApecoxCombatTeam : uint8
{
	Players,
	AI
};

UENUM(BlueprintType)
enum class EApecoxShieldTier : uint8
{
	White UMETA(DisplayName = "White"),
	Blue UMETA(DisplayName = "Blue"),
	Purple UMETA(DisplayName = "Purple")
};

class UApecoxAbilitySystemComponent;
class UApecoxVitalAttributeSet;

/**
 * AApecoxPlayerState
 * - ASC 和 VitalAttributeSet 的真正 Owner，保证它们可跨 Pawn 生命周期存在
 * - 实现 IAbilitySystemInterface，对外提供强类型 ASC 访问
 * - ASC 使用 Mixed 复制模式：拥有客户端收到完整 GE 信息，远端仅收到 Cue/公开属性
 */
UCLASS()
class APECOX_API AApecoxPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- IAbilitySystemInterface ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 项目强类型 ASC 访问
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

	// 只读 AttributeSet 访问；后续数值修改必须通过 ASC/GameplayEffect
	const UApecoxVitalAttributeSet* GetVitalAttributeSet() const;

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Match")
	EApecoxCombatTeam GetCombatTeam() const { return CombatTeam; }

	UFUNCTION(BlueprintPure, Category = "Apecox|Shield")
	float GetShield() const;

	UFUNCTION(BlueprintPure, Category = "Apecox|Shield")
	float GetMaxShield() const;

	UFUNCTION(BlueprintPure, Category = "Apecox|Shield")
	float GetShieldEvolutionPoints() const;

	UFUNCTION(BlueprintPure, Category = "Apecox|Shield")
	EApecoxShieldTier GetShieldTier() const;

	/** 紫色品质返回 0；其他品质返回到下一等级还需要的点数。 */
	UFUNCTION(BlueprintPure, Category = "Apecox|Shield")
	float GetShieldEvolutionPointsToNextTier() const;

	void AddKill();
	void AddDeath();
	void SetCombatTeam(EApecoxCombatTeam NewTeam);

	/** 每个新 Pawn 出生时调用。玩家保留进化点/等级并补满护盾，AI 清零全部护盾状态。 */
	void InitializeCombatAttributesForPawn(bool bEnablePlayerShield);

	/** Authority：按实际对敌伤害增加进化点，并在累计 500/1500 阈值升级护盾。 */
	float AddShieldEvolutionPoints(float Amount);

	/** Authority：不改变 MaxHealth，把当前生命恢复指定值；返回是否真的恢复。 */
	bool TryApplyHealthPickup(float RestoreAmount);

	/** Authority：增加进化点后补满升级后的护盾；返回是否真的改变状态。 */
	bool TryApplyShieldBattery(float EvolutionPoints = 300.0f);

	static constexpr float WhiteShieldValue = 25.0f;
	static constexpr float BlueShieldValue = 50.0f;
	static constexpr float PurpleShieldValue = 75.0f;
	/** 白色到蓝色需要 500；蓝色到紫色再需要 1000，因此紫色累计阈值为 1500。 */
	static constexpr float BlueEvolutionThreshold = 500.0f;
	static constexpr float PurpleEvolutionThreshold = 1500.0f;

private:
	// PlayerState 是 ASC 和 AttributeSet 的唯一 Owner：
	// 构造时创建默认子对象，生命周期由 PlayerState 管理，Character 只看不创建
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UApecoxVitalAttributeSet> VitalAttributeSet;

	UPROPERTY(Replicated)
	int32 Kills = 0;

	UPROPERTY(Replicated)
	int32 Deaths = 0;

	UPROPERTY(Replicated)
	EApecoxCombatTeam CombatTeam = EApecoxCombatTeam::Players;
};
