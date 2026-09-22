// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "ApecoxWeaponImpactGameplayCue.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

/**
 * 步枪实体子弹的瞬时命中表现。
 * 玩法伤害仍由 Projectile/ASC 结算；本类只消费已有 HitResult，播放粒子、声音和短寿命弹孔。
 */
UCLASS(Blueprintable)
class APECOX_API UApecoxWeaponImpactGameplayCue : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UApecoxWeaponImpactGameplayCue();

	virtual bool OnExecute_Implementation(
		AActor* MyTarget,
		const FGameplayCueParameters& Parameters) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact")
	TObjectPtr<UNiagaraSystem> ImpactSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact")
	TObjectPtr<USoundBase> ImpactSound;

	/** 每次命中随机挑选一个贴花材质，避免墙面重复图案过于明显。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact")
	TArray<TObjectPtr<UMaterialInterface>> ImpactDecalMaterials;

	/** X 是贴花投射深度，Y/Z 是弹孔宽高。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact",
		meta = (ClampMin = "0.1", Units = "cm"))
	FVector ImpactDecalSize = FVector(4.0f, 5.0f, 5.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact",
		meta = (ClampMin = "0.0", Units = "s"))
	float DecalFadeStartDelay = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apecox|Impact",
		meta = (ClampMin = "0.0", Units = "s"))
	float DecalFadeDuration = 3.0f;
};
