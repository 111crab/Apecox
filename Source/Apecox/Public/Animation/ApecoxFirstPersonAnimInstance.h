// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/ApecoxCharacterAnimInstance.h"
#include "ApecoxFirstPersonAnimInstance.generated.h"

/**
 * 第一人称 Arms 专属 AnimInstance。
 * 在游戏线程把当前武器的左手握点转换到 Arms Component Space，AnimGraph 只读取缓存结果。
 */
UCLASS()
class APECOX_API UApecoxFirstPersonAnimInstance : public UApecoxCharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	void UpdateLeftHandIKState();
	void ResetLeftHandIKState();
	void UpdateTurningState(float DeltaSeconds);
	void UpdateWeaponRecoilState();
	void UpdateFirstPersonAimState();

	/** ADS 的立即状态与平滑权重；AnimGraph用bIsAiming和0.20秒Pose Blend，AimAlpha用于镜头/HUD同步。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	bool bIsAiming = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	float AimAlpha = 0.0f;

	/** 只在地面移动时播放 RAR Aimed Walk additive；空中保持稳定的 Aimed Idle。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	bool bUseAimedWalkAnimation = false;

	/** AnimGraph在ik_hand_gun上以Component Space / Additive应用，并放在左手FABRIK之前。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Recoil")
	FVector WeaponRecoilLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Recoil")
	FRotator WeaponRecoilRotation = FRotator::ZeroRotator;

	/** 供普通Apply Additive使用：只在地面静止转向时叠加RAR Turning循环。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Turning")
	float TurningAlpha = 0.0f;

	/**
	 * 实测RAR的SOCKET_Camera在该Pose中没有位置或旋转Delta；探头由完整上半身Pose与
	 * Character侧独立的50cm镜头侧移、15度Roll共同组成。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Animation|Lean", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FirstPersonLeanPoseScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Animation|Turning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TurningMaxAlpha = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Animation|Turning", meta = (ClampMin = "1.0"))
	float TurningReferenceTurnSpeed = 90.0f;

	/** FABRIK Effector Transform，空间为当前第一人称 Arms 的 Component Space */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|IK")
	FTransform LeftHandIKEffectorTransform = FTransform::Identity;

	/** 当前存在有效握点时为 1，否则为 0；后续可与 Montage 曲线共同控制 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|IK")
	float LeftHandIKAlpha = 0.0f;

private:
	friend class FApecoxTurningTest;
	TWeakObjectPtr<AController> TurningController;
	float PreviousTurningYaw = 0.0f;
	bool bTurningInitialized = false;
};
