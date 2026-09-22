// Copyright Apecox. All Rights Reserved.
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxWeaponProjectile.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

FApecoxRangedFireConfig::FApecoxRangedFireConfig()
{
	// RAR Default行的原始Vector Spring Interp参数。
	StandingWeaponRecoilLocationSpring.Mass = 0.002f;
	StandingWeaponRecoilRotationSpring.Mass = 0.006f;
	AimingWeaponRecoilLocationSpring.Mass = 0.004f;
	AimingWeaponRecoilRotationSpring.Mass = 0.006f;
	CameraRecoilRotationSpring.Stiffness = 0.6f;
	CameraRecoilRotationSpring.Mass = 0.002f;
}

float FApecoxRangedFireConfig::EvaluateSpreadMultiplier(int32 BurstShotIndex,
	bool bIsAiming, bool bHasMovementInput) const
{
	const float NormalizedShotCount = FMath::Clamp(
		static_cast<float>(FMath::Max(BurstShotIndex, 0)) / FMath::Max(MagazineCapacity, 1), 0.0f, 1.0f);
	const float CurveValue = AutomaticSpreadCurve
		? FMath::Max(AutomaticSpreadCurve->GetFloatValue(NormalizedShotCount), 0.0f) : 0.0f;
	const float ContinuousTerm = CurveValue * (bIsAiming ? AimingSpreadMultiplier : 1.0f);
	const float MovementTerm = (!bIsAiming && bHasMovementInput) ? MovementSpreadMultiplier : 0.0f;
	return ContinuousTerm + MovementTerm;
}

FVector FApecoxRangedFireConfig::ApplySpread(const FVector& BaseDirection, uint32 ShotId,
	int32 BurstShotIndex, bool bIsAiming, bool bHasMovementInput) const
{
	const FVector Direction = BaseDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return FVector::ForwardVector;
	}
	const float Multiplier = EvaluateSpreadMultiplier(BurstShotIndex, bIsAiming, bHasMovementInput);
	const float YawRadians = FMath::DegreesToRadians(SpreadYawDegrees * Multiplier);
	const float PitchRadians = FMath::DegreesToRadians(SpreadPitchDegrees * Multiplier);
	if (YawRadians <= KINDA_SMALL_NUMBER && PitchRadians <= KINDA_SMALL_NUMBER)
	{
		return Direction;
	}
	const uint32 Seed = HashCombineFast(ShotId, static_cast<uint32>(FMath::Max(BurstShotIndex, 0)) + 0x9E3779B9u);
	FRandomStream Random(static_cast<int32>(Seed));
	return Random.VRandCone(Direction, YawRadians, PitchRadians).GetSafeNormal();
}

void FApecoxRangedFireConfig::EvaluateRecoilTargets(int32 BurstShotIndex, bool bIsAiming,
	FVector& OutWeaponLocation, FVector& OutWeaponRotation, FVector& OutCameraRotation) const
{
	const float ShotCount = static_cast<float>(FMath::Max(BurstShotIndex, 0) + 1);
	const UCurveVector* LocationCurve = bIsAiming
		? AimingWeaponRecoilLocationCurve.Get() : StandingWeaponRecoilLocationCurve.Get();
	const UCurveVector* RotationCurve = bIsAiming
		? AimingWeaponRecoilRotationCurve.Get() : StandingWeaponRecoilRotationCurve.Get();
	const float LocationMultiplier = bIsAiming
		? AimingWeaponRecoilLocationMultiplier : StandingWeaponRecoilLocationMultiplier;
	const float RotationMultiplier = bIsAiming
		? AimingWeaponRecoilRotationMultiplier : StandingWeaponRecoilRotationMultiplier;
	const float CameraMultiplier = bIsAiming
		? AimingCameraRecoilRotationMultiplier : StandingCameraRecoilRotationMultiplier;

	OutWeaponLocation = LocationCurve ? LocationCurve->GetVectorValue(ShotCount) * LocationMultiplier : FVector::ZeroVector;
	OutWeaponRotation = RotationCurve ? RotationCurve->GetVectorValue(ShotCount) * RotationMultiplier : FVector::ZeroVector;
	OutCameraRotation = CameraRecoilRotationCurve
		? CameraRecoilRotationCurve->GetVectorValue(ShotCount) * CameraMultiplier : FVector::ZeroVector;
}
FApecoxProjectileFireConfig::FApecoxProjectileFireConfig()
{
    ProjectileClass = AApecoxWeaponProjectile::StaticClass();
}
bool FApecoxProjectileFireConfig::IsValidProjectileConfig() const
{
    return IsValidRangedConfig() && ProjectileClass && !ProjectileClass->HasAnyClassFlags(CLASS_Abstract)
        && FMath::IsFinite(MinSpeed) && FMath::IsFinite(MaxSpeed) && MinSpeed > 0 && MaxSpeed >= MinSpeed
        && FMath::IsFinite(GravityScale) && GravityScale >= 0
        && FMath::IsFinite(CollisionRadius) && CollisionRadius >= 0.1f
        && FMath::IsFinite(LifeSeconds) && LifeSeconds > 0;
}
