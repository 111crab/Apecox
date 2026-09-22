// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ApecoxWeaponFireConfig.generated.h"

class AApecoxWeaponProjectile;
class UCurveFloat;
class UCurveVector;

/** 与RAR的Vector Spring Interp参数一一对应。 */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxRecoilSpringSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float Stiffness = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.0"))
	float CriticalDampingFactor = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "0.000001"))
	float Mass = 0.006f;

	bool IsValid() const
	{
		return FMath::IsFinite(Stiffness) && Stiffness >= 0.0f
			&& FMath::IsFinite(CriticalDampingFactor) && CriticalDampingFactor >= 0.0f
			&& FMath::IsFinite(Mass) && Mass > 0.0f;
	}
};

/**
 * FApecoxWeaponFireConfig
 * - 所有射击模型的公共配置基类。
 * - 使用 USTRUCT(BlueprintType) 嵌入 UApecoxWeaponDefinition 的 TInstancedStruct 中，
 *   提供类型安全的多态射击配置——不开放任意 Fragment/Step 数组。
 *
 * 为什么射击配置放在 WeaponDefinition 而非 WeaponInstance：
 * - 弹匣容量、射速、发射源偏移是武器类型的不可变属性（所有同款步枪共享），
 *   不是单把武器实例的可变状态。WeaponInstance 保存运行时弹匣和射速账本。
 */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxWeaponFireConfig
{
	GENERATED_BODY()

	/** 新建武器实例时的弹匣容量——Authority 创建路径从有效 FireConfig 初始化 CurrentMagazineAmmo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig",
		meta = (ClampMin = "1", UIMin = "1"))
	int32 MagazineCapacity = 30;

	/** 每分钟射速——客户端射速节奏与服务端权威射速门控的共同静态参数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float RoundsPerMinute = 600.0f;

	/**
	 * 相对角色权威视角旋转的玩法发射源偏移。
	 *
	 * 为什么这是 Gameplay Origin 而不是 Weapon Mesh Socket：
	 * - Dedicated Server 没有 FP/TP Weapon Mesh 或渲染 Socket；
	 * - 发射源必须是确定性、不依赖视觉组件的权威位置；
	 * - 这个偏移在角色 BaseAimRotation 空间中应用，产生一致的玩法结果。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig")
	FVector GameplayFireOriginOffset = FVector(50.0f, 10.0f, -10.0f);

	/** MagazineCapacity > 0 且 RoundsPerMinute > 0 才视为有效射击配置 */
	bool IsValidFireConfig() const
	{
		return MagazineCapacity > 0 && FMath::IsFinite(RoundsPerMinute) && RoundsPerMinute > 0.0f && !GameplayFireOriginOffset.ContainsNaN();
	}
};

/**
 * FApecoxRangedFireConfig
 * - 当前Projectile模型使用的瞄准距离、查询通道、散布、后坐与伤害配置公共层。
 * - WeaponDefinition仍需选择具体的派生模型，不能只配置这一共享层。
 */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxRangedFireConfig : public FApecoxWeaponFireConfig
{
	GENERATED_BODY()

	FApecoxRangedFireConfig();

	/** 瞄准检测距离（厘米）；Projectile飞行距离由弹速、重力与寿命决定 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Ranged",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxRange = 10000.0f;

	/** 权威世界检测通道——不依赖客户端 CameraComponent 或 Weapon Mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Ranged")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel1;

	/** 单发基础伤害。Authority 写入原生 Instant GE 的 SetByCaller.Damage。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Ranged",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BaseDamage = 13.0f;

	/** RAR自动射击散布曲线：输入为BurstShotIndex / MagazineCapacity，输出为连发倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Spread")
	TObjectPtr<const UCurveFloat> AutomaticSpreadCurve;

	/** 椭圆锥水平方向最大半角；实际角度还要乘动态SpreadMultiplier。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Spread", meta = (ClampMin = "0.0"))
	float SpreadYawDegrees = 12.0f;

	/** 椭圆锥垂直方向最大半角；实际角度还要乘动态SpreadMultiplier。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Spread", meta = (ClampMin = "0.0"))
	float SpreadPitchDegrees = 12.0f;

	/** ADS接入后仅缩放连发曲线项；瞄准状态不叠加移动散布。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Spread", meta = (ClampMin = "0.0"))
	float AimingSpreadMultiplier = 0.3f;

	/** 存在主动移动加速度时叠加的固定倍率，复现RAR的二值移动项。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Spread", meta = (ClampMin = "0.0"))
	float MovementSpreadMultiplier = 0.3f;

	/** RAR站姿枪械平移后坐；输入是从1开始的当前连发序号。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing")
	TObjectPtr<const UCurveVector> StandingWeaponRecoilLocationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing", meta = (ClampMin = "0.0"))
	float StandingWeaponRecoilLocationMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing")
	FApecoxRecoilSpringSettings StandingWeaponRecoilLocationSpring;

	/** RAR站姿枪械旋转后坐；Vector的X/Y/Z会转换为Rotator的Roll/Pitch/Yaw。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing")
	TObjectPtr<const UCurveVector> StandingWeaponRecoilRotationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing", meta = (ClampMin = "0.0"))
	float StandingWeaponRecoilRotationMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Standing")
	FApecoxRecoilSpringSettings StandingWeaponRecoilRotationSpring;

	/** 为后续ADS预留的RAR枪械平移曲线；本轮尚未切换到此分支。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming")
	TObjectPtr<const UCurveVector> AimingWeaponRecoilLocationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming", meta = (ClampMin = "0.0"))
	float AimingWeaponRecoilLocationMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming")
	FApecoxRecoilSpringSettings AimingWeaponRecoilLocationSpring;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming")
	TObjectPtr<const UCurveVector> AimingWeaponRecoilRotationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming", meta = (ClampMin = "0.0"))
	float AimingWeaponRecoilRotationMultiplier = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Aiming")
	FApecoxRecoilSpringSettings AimingWeaponRecoilRotationSpring;

	/** 站姿与ADS共享RAR镜头曲线，只切换倍率。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Camera")
	TObjectPtr<const UCurveVector> CameraRecoilRotationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Camera", meta = (ClampMin = "0.0"))
	float StandingCameraRecoilRotationMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Camera", meta = (ClampMin = "0.0"))
	float AimingCameraRecoilRotationMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireConfig|Recoil|Camera")
	FApecoxRecoilSpringSettings CameraRecoilRotationSpring;

	/** 通用射击参数有效且MaxRange为正的有限数 */
	bool IsValidRangedConfig() const
	{
		return IsValidFireConfig() && FMath::IsFinite(MaxRange) && MaxRange > 0.0f
			&& FMath::IsFinite(BaseDamage) && BaseDamage >= 0.0f
			&& FMath::IsFinite(SpreadYawDegrees) && SpreadYawDegrees >= 0.0f
			&& FMath::IsFinite(SpreadPitchDegrees) && SpreadPitchDegrees >= 0.0f
			&& FMath::IsFinite(AimingSpreadMultiplier) && AimingSpreadMultiplier >= 0.0f
			&& FMath::IsFinite(MovementSpreadMultiplier) && MovementSpreadMultiplier >= 0.0f
			&& FMath::IsFinite(StandingWeaponRecoilLocationMultiplier) && StandingWeaponRecoilLocationMultiplier >= 0.0f
			&& FMath::IsFinite(StandingWeaponRecoilRotationMultiplier) && StandingWeaponRecoilRotationMultiplier >= 0.0f
			&& FMath::IsFinite(AimingWeaponRecoilLocationMultiplier) && AimingWeaponRecoilLocationMultiplier >= 0.0f
			&& FMath::IsFinite(AimingWeaponRecoilRotationMultiplier) && AimingWeaponRecoilRotationMultiplier >= 0.0f
			&& FMath::IsFinite(StandingCameraRecoilRotationMultiplier) && StandingCameraRecoilRotationMultiplier >= 0.0f
			&& FMath::IsFinite(AimingCameraRecoilRotationMultiplier) && AimingCameraRecoilRotationMultiplier >= 0.0f
			&& StandingWeaponRecoilLocationSpring.IsValid() && StandingWeaponRecoilRotationSpring.IsValid()
			&& AimingWeaponRecoilLocationSpring.IsValid() && AimingWeaponRecoilRotationSpring.IsValid()
			&& CameraRecoilRotationSpring.IsValid();
	}

	/** 曲线为空时连发项为0，移动项仍然有效。 */
	float EvaluateSpreadMultiplier(int32 BurstShotIndex, bool bIsAiming, bool bHasMovementInput) const;

	/** ShotId作为确定性随机种子；相同输入始终返回同一椭圆锥方向。 */
	FVector ApplySpread(const FVector& BaseDirection, uint32 ShotId, int32 BurstShotIndex,
		bool bIsAiming, bool bHasMovementInput) const;

	/** 后坐曲线按1-based Shot Count采样；曲线为空时对应输出为零。 */
	void EvaluateRecoilTargets(int32 BurstShotIndex, bool bIsAiming,
		FVector& OutWeaponLocation, FVector& OutWeaponRotation, FVector& OutCameraRotation) const;
};

/** Ballistic defaults use RAR's standard magazine speed range. Units: cm, seconds. */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxProjectileFireConfig : public FApecoxRangedFireConfig
{
    GENERATED_BODY()
    FApecoxProjectileFireConfig();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile")
    TSubclassOf<AApecoxWeaponProjectile> ProjectileClass;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile", meta=(ClampMin="1"))
    float MinSpeed = 15000.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile", meta=(ClampMin="1"))
    float MaxSpeed = 20000.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile", meta=(ClampMin="0"))
    float GravityScale = 1.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile", meta=(ClampMin="0.1"))
    float CollisionRadius = 1.0f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FireConfig|Projectile", meta=(ClampMin="0.01"))
    float LifeSeconds = 5.0f;
    bool IsValidProjectileConfig() const;
};
