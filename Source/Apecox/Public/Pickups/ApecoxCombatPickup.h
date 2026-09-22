// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ApecoxCombatPickup.generated.h"

class UPointLightComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EApecoxCombatPickupKind : uint8
{
	HealthPack UMETA(DisplayName = "Health Pack"),
	ShieldBattery UMETA(DisplayName = "Shield Battery")
};

/**
 * 地图可放置的服务器权威补给。
 * - HealthPack：恢复生命，默认 100（当前规则等同补满）。
 * - ShieldBattery：增加 300 进化点，再补满升级后的护盾。
 * - AI 不可拾取；可见性与重生由一个复制布尔值驱动。
 *
 * Mesh 是纯表现插槽。默认基础形状用于无美术资产时验证，后续可直接在实例上替换 Fab 网格。
 */
UCLASS(Blueprintable)
class APECOX_API AApecoxCombatPickup : public AActor
{
	GENERATED_BODY()

public:
	AApecoxCombatPickup();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Apecox|Pickup")
	bool IsPickupAvailable() const { return bPickupAvailable; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_PickupAvailable();

	void SetPickupAvailable(bool bNewAvailable);
	void RespawnPickup();
	void RefreshPresentation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	TObjectPtr<UTextRenderComponent> PickupLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	TObjectPtr<UPointLightComponent> AccentLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	TObjectPtr<URotatingMovementComponent> RotatingMovement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup")
	EApecoxCombatPickupKind PickupKind = EApecoxCombatPickupKind::HealthPack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup", meta = (ClampMin = "0.0"))
	float HealthRestoreAmount = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup", meta = (ClampMin = "0.0"))
	float ShieldEvolutionPoints = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup", meta = (ClampMin = "0.0", Units = "s"))
	float RespawnDelay = 20.0f;

	/** 若网格材质暴露 Emissive_Color，实例会自动按补给类型着色，不会改写原始材质资产。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup|Visual")
	FLinearColor HealthVisualColor = FLinearColor(1.0f, 0.02f, 0.01f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup|Visual")
	FLinearColor ShieldVisualColor = FLinearColor(0.01f, 0.18f, 1.0f);

	/** SciFi_Props 的 M_Master 使用该参数控制发光强度。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|Pickup|Visual",
		meta = (ClampMin = "0.0"))
	float EmissivePower = 20.0f;

	UPROPERTY(ReplicatedUsing = OnRep_PickupAvailable)
	bool bPickupAvailable = true;

private:
	FTimerHandle RespawnTimerHandle;
};
