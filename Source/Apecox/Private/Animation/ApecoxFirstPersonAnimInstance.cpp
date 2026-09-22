// Copyright Apecox. All Rights Reserved.

#include "Animation/ApecoxFirstPersonAnimInstance.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UApecoxFirstPersonAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (OwningCharacter && GetSkelMeshComponent() == OwningCharacter->GetFirstPersonMesh())
	{
		// 保留RAR完整上半身Lean Pose；镜头的侧移与Roll由Character独立叠加。
		// 属性仍可用于独立调试，但当前正式值为1；第三人称也继续读取完整[-1, 1]值。
		LeanAmount = FMath::Clamp(LeanAmount * FirstPersonLeanPoseScale, -1.0f, 1.0f);
	}
	UpdateFirstPersonAimState();
	UpdateWeaponRecoilState();
	UpdateLeftHandIKState();
	UpdateTurningState(DeltaSeconds);
}

void UApecoxFirstPersonAnimInstance::UpdateFirstPersonAimState()
{
	bIsAiming = false;
	AimAlpha = 0.0f;
	bUseAimedWalkAnimation = false;
	if (!OwningCharacter || !OwningCharacter->IsLocallyControlled()
		|| GetSkelMeshComponent() != OwningCharacter->GetFirstPersonMesh())
	{
		return;
	}

	bIsAiming = OwningCharacter->IsAiming();
	AimAlpha = OwningCharacter->GetAimAlpha();
	const UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement();
	bUseAimedWalkAnimation = bIsAiming && Movement && Movement->IsMovingOnGround()
		&& Movement->Velocity.SizeSquared2D() > 9.0;
}

void UApecoxFirstPersonAnimInstance::UpdateWeaponRecoilState()
{
	WeaponRecoilLocation = FVector::ZeroVector;
	WeaponRecoilRotation = FRotator::ZeroRotator;
	if (!OwningCharacter || !OwningCharacter->IsLocallyControlled()
		|| GetSkelMeshComponent() != OwningCharacter->GetFirstPersonMesh())
	{
		return;
	}

	WeaponRecoilLocation = OwningCharacter->GetFirstPersonWeaponRecoilLocation();
	WeaponRecoilRotation = OwningCharacter->GetFirstPersonWeaponRecoilRotation();
}

void UApecoxFirstPersonAnimInstance::UpdateTurningState(float DeltaSeconds)
{
	AController* Controller = OwningCharacter ? OwningCharacter->GetController() : nullptr;
	const float Yaw = OwningCharacter ? OwningCharacter->GetControlRotation().Yaw : 0.0f;
	const float DeltaYaw = FRotator::NormalizeAxis(Yaw - PreviousTurningYaw);
	const bool bReset = !bTurningInitialized || !Controller || TurningController.Get() != Controller
		|| DeltaSeconds > 0.1f || FMath::Abs(DeltaYaw) > 45.0f;
	PreviousTurningYaw = Yaw;
	TurningController = Controller;
	bTurningInitialized = Controller != nullptr;
	const UCharacterMovementComponent* Movement = OwningCharacter ? OwningCharacter->GetCharacterMovement() : nullptr;
	const UApecoxEquipmentComponent* Equipment = OwningCharacter ? OwningCharacter->GetEquipmentComponent() : nullptr;
	if (bReset || !OwningCharacter || !OwningCharacter->IsLocallyControlled()
		|| !Equipment || !Equipment->IsArmed() || !Movement || !Movement->IsMovingOnGround()
		|| OwningCharacter->IsProne() || OwningCharacter->IsSprinting()
		|| Equipment->IsEquipPresentationActive()
		|| OwningCharacter->IsInspecting() || OwningCharacter->IsReloading() || bIsAiming
		|| Movement->Velocity.SizeSquared2D() > 9.0
		|| !OwningCharacter->GetPendingMovementInputVector().IsNearlyZero())
	{
		// 空中、移动或机械瞄准时立即撤掉这一层，避免普通走路Additive扰动跳跃、爬行或瞄具对齐。
		TurningAlpha = 0.0f;
		return;
	}
	if (DeltaSeconds <= UE_SMALL_NUMBER) { return; }
	const float TurnSpeed = FMath::Abs(DeltaYaw) / DeltaSeconds;
	const float Target = FMath::Clamp(TurnSpeed / FMath::Max(TurningReferenceTurnSpeed, 1.0f), 0.0f, 1.0f)
		* FMath::Clamp(TurningMaxAlpha, 0.0f, 1.0f) * (Movement->IsCrouching() ? 0.7f : 1.0f);
	// 原工程Turning层用7的插值速度；这里用指数响应避免帧率改变混合时间。
	TurningAlpha = FMath::Lerp(TurningAlpha, Target, 1.0f - FMath::Exp(-7.0f * DeltaSeconds));
}

void UApecoxFirstPersonAnimInstance::UpdateLeftHandIKState()
{
	ResetLeftHandIKState();

	USkeletalMeshComponent* ArmsMesh = GetSkelMeshComponent();
	if (!OwningCharacter || !OwningCharacter->IsLocallyControlled()
		|| !ArmsMesh || ArmsMesh != OwningCharacter->GetFirstPersonMesh())
	{
		return;
	}
	const UApecoxEquipmentComponent* Equipment = OwningCharacter->GetEquipmentComponent();
	if (OwningCharacter->IsInspecting() || OwningCharacter->IsReloading()
		|| (Equipment && Equipment->IsEquipPresentationActive()))
	{
		// RAR检视/装备动作会主动移动左手；否则FABRIK会把它强行拉回护木。
		return;
	}

	const USceneComponent* GripPoint = Equipment
		? Equipment->GetFirstPersonLeftHandGripPoint()
		: nullptr;
	if (!GripPoint)
	{
		return;
	}

	LeftHandIKEffectorTransform = GripPoint->GetComponentTransform().GetRelativeTransform(
		ArmsMesh->GetComponentTransform());
	LeftHandIKAlpha = 1.0f;
}

void UApecoxFirstPersonAnimInstance::ResetLeftHandIKState()
{
	LeftHandIKEffectorTransform = FTransform::Identity;
	LeftHandIKAlpha = 0.0f;
}
