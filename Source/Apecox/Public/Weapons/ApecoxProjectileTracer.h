// Copyright Apecox. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ApecoxProjectileTracer.generated.h"

class UProjectileMovementComponent;
class UStaticMeshComponent;

/**
 * Local cosmetic tracer spawned by the existing replicated weapon-fire GameplayCue.
 * It never replicates, collides, applies damage, or changes the authoritative projectile.
 */
UCLASS(NotBlueprintable, Transient)
class APECOX_API AApecoxProjectileTracer : public AActor
{
	GENERATED_BODY()

public:
	AApecoxProjectileTracer();

	/** Starts one local tracer. MaxDistance is the cosmetic sight distance, in centimetres. */
	void InitializeTracer(const FVector& Direction, float Speed, float GravityScale, float MaxDistance);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracer")
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tracer")
	TObjectPtr<UProjectileMovementComponent> Movement;

	/** The RAR mesh is 2.807 cm long on X; 72x produces an approximately 2 m visible streak. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracer|Visual")
	FVector TracerVisualScale = FVector(72.0f, 4.0f, 4.0f);

	/** Raised from the source material's 100 so the streak remains legible during fast flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracer|Visual", meta = (ClampMin = "0.0"))
	float EmissiveIntensity = 250.0f;

	/** Hard safety cap for a cosmetic actor if its configured range or speed is invalid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tracer|Visual", meta = (ClampMin = "0.01", Units = "s"))
	float MaximumLifeSeconds = 0.75f;

private:
	float TravelLimit = 0.0f;
	float DistanceTravelled = 0.0f;
	FVector PreviousLocation = FVector::ZeroVector;
};
