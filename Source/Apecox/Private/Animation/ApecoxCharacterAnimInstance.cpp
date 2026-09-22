// Copyright Apecox. All Rights Reserved.

#include "Animation/ApecoxCharacterAnimInstance.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace ApecoxThirdPersonAim
{
	// 与 Lyra ABP_Mannequin_Base 的默认值一致。
	constexpr float StandingRootYawMin = -120.0f;
	constexpr float StandingRootYawMax = 100.0f;
	constexpr float CrouchedRootYawMin = -90.0f;
	constexpr float CrouchedRootYawMax = 80.0f;
	// Lyra 默认 50 度在快速镜头输入下可能先撞 Clamp 并导致脚滑；Apecox 提前到 35 度触发。
	constexpr float TurnInPlaceThreshold = 35.0f;
	constexpr float TurnYawCurveWeightEpsilon = 0.001f;
	constexpr float TurnYawCurveRestartEpsilon = 2.0f;
	constexpr float TurnReverseInputSpeedThreshold = 30.0f;
	// 90 度曲线的有效旋转约在 0.6 秒内完成，即基础消费能力约 150 度/秒。
	constexpr float TurnCurveDegreesPerSecond = 150.0f;
	constexpr float MaxTurnInPlacePlayRate = 1.6f;
	constexpr float TurnPlayRateBlendDownSpeed = 10.0f;
	constexpr float RootYawSpringStiffness = 80.0f;
	constexpr float RootYawSpringCriticalDamping = 1.0f;
	constexpr float RootYawSpringMass = 1.0f;
	constexpr float RootYawSpringTargetVelocityAmount = 0.5f;
}

void UApecoxCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CacheCharacterReferences();
}

void UApecoxCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 缓存失效或 Pawn 已变更时重新缓存（TryGetPawnOwner 失败时由 CacheCharacterReferences 清空）
	if (!OwningCharacter || !CharacterMovement || TryGetPawnOwner() != OwningCharacter)
	{
		CacheCharacterReferences();
	}

	// 没有合法角色或移动组件（如编辑器预览）时恢复到中性状态并返回
	if (!OwningCharacter || !CharacterMovement)
	{
		ResetAnimationState();
		return;
	}

	UpdateLocomotionState();
	UpdateEquipmentState();
	UpdateAimState(DeltaSeconds);
}

void UApecoxCharacterAnimInstance::CacheCharacterReferences()
{
	// 通过 Pawn Owner Cast 到项目角色；编辑器预览没有合法 Pawn 时安全清空缓存
	AApecoxPlayerCharacter* NewOwningCharacter = Cast<AApecoxPlayerCharacter>(TryGetPawnOwner());
	if (OwningCharacter != NewOwningCharacter)
	{
		// AnimInstance 重绑 Pawn/重生时不能沿用旧 Pawn 的世界 Yaw，否则首帧会被解释成转身输入。
		bHasPreviousActorYaw = false;
		PreviousActorYaw = 0.0f;
		RootYawOffset = 0.0f;
		AimYaw = 0.0f;
		bShouldTurnInPlace = false;
		bTurnInPlaceLeft = false;
		bShouldInterruptTurnInPlace = false;
		TurnInPlacePlayRate = 1.0f;
		PreviousTurnYawCurveValue = 0.0f;
		bHasPreviousTurnYawCurveValue = false;
		bHasActiveTurnDirection = false;
		bActiveTurnLeft = false;
		RootYawOffsetSpringState.Reset();
	}

	OwningCharacter = NewOwningCharacter;
	if (OwningCharacter)
	{
		CharacterMovement = OwningCharacter->GetCharacterMovement();
	}
	else
	{
		CharacterMovement = nullptr;
	}
}

void UApecoxCharacterAnimInstance::UpdateLocomotionState()
{
	const FVector Velocity = CharacterMovement->Velocity;

	GroundSpeed = Velocity.Size2D();
	VerticalSpeed = Velocity.Z;
	bIsMoving = GroundSpeed > 3.0f;
	bIsFalling = CharacterMovement->IsFalling();
	bIsCrouching = CharacterMovement->IsCrouching();
	bIsSprinting = OwningCharacter->IsSprinting();
	bIsProne = OwningCharacter->IsProne();
	LeanAmount = OwningCharacter->GetLeanAmount();

	if (bIsMoving)
	{
		// 只有移动时才更新方向，避免把上一帧的朝向残留到静止状态
		MovementDirection = FRotator::NormalizeAxis(
			Velocity.ToOrientationRotator().Yaw - OwningCharacter->GetActorRotation().Yaw);
	}
	else
	{
		// 静止时把方向归零，不保留上一帧值
		MovementDirection = 0.0f;
	}
}

void UApecoxCharacterAnimInstance::UpdateEquipmentState()
{
	// 每帧先回到中性值，再沿装备链路安全读取；任一环节为空都保持 Unarmed。
	// 不缓存或复制第二份装备真相——动画族只从 EquipmentComponent 的公开摘要派生。
	AnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;
	ThirdPersonLeftHandIKAlpha = FMath::Clamp(
		1.0f - GetCurveValue(FName(TEXT("DisableLHandIK"))), 0.0f, 1.0f);

	if (const UApecoxEquipmentComponent* Equipment = OwningCharacter->GetEquipmentComponent())
	{
		if (const UApecoxWeaponDefinition* WeaponDef = Equipment->GetEquippedWeaponDefinition())
		{
			if (const UApecoxWeaponPresentationDefinition* Pres = WeaponDef->PresentationDefinition)
			{
				AnimationFamily = Pres->EquippedAnimationFamily;
			}
		}
	}
}

void UApecoxCharacterAnimInstance::UpdateAimState(float DeltaSeconds)
{
	// 垂直方向继续使用玩法 AimRotation 与 ActorRotation 的差值。
	const FRotator AimDelta =
		(OwningCharacter->GetBaseAimRotation() - OwningCharacter->GetActorRotation()).GetNormalized();
	AimPitch = AimDelta.Pitch;

	const float CurrentActorYaw = OwningCharacter->GetActorRotation().Yaw;
	if (!bHasPreviousActorYaw)
	{
		// 冷启动只建立基线。若直接与默认 0 比较，任意出生朝向都会制造一次错误根偏移。
		PreviousActorYaw = CurrentActorYaw;
		bHasPreviousActorYaw = true;
		RootYawOffset = 0.0f;
		AimYaw = 0.0f;
		bShouldTurnInPlace = false;
		bTurnInPlaceLeft = false;
		bShouldInterruptTurnInPlace = false;
		TurnInPlacePlayRate = 1.0f;
		PreviousTurnYawCurveValue = 0.0f;
		bHasPreviousTurnYawCurveValue = false;
		bHasActiveTurnDirection = false;
		bActiveTurnLeft = false;
		RootYawOffsetSpringState.Reset();
		return;
	}

	const float ActorYawDelta = FRotator::NormalizeAxis(CurrentActorYaw - PreviousActorYaw);
	PreviousActorYaw = CurrentActorYaw;

	// 静止接地时，Actor 仍立即随 Controller 转动。把相反增量累加到根骨，视觉上暂时保持脚部朝向。
	// 趴姿目前没有第三人称配套资产，因此不让该未完成姿态进入水平扭转。
	const bool bCanHoldFeet = !bIsMoving && !bIsFalling && !bIsProne;
	if (bCanHoldFeet)
	{
		RootYawOffset = FRotator::NormalizeAxis(RootYawOffset - ActorYawDelta);
		bShouldInterruptTurnInPlace = false;

		// 固定 1.0 倍的 90 度动作只能以约 150 度/秒交接 RootYawOffset。快速鼠标输入若高于
		// 该速度，会先撞到根偏移上限并出现脚部滑动。提高动画播放率以追上当前输入；加速立即生效，
		// 减速则平滑回落，避免快转结束后动作速率突然跳变。
		const float SafeDeltaSeconds = FMath::Max(DeltaSeconds, UE_SMALL_NUMBER);
		const float ActorYawSpeed = FMath::Abs(ActorYawDelta) / SafeDeltaSeconds;
		const float SpeedDrivenPlayRate = FMath::Clamp(
			ActorYawSpeed / ApecoxThirdPersonAim::TurnCurveDegreesPerSecond,
			1.0f,
			ApecoxThirdPersonAim::MaxTurnInPlacePlayRate);
		const float OffsetDrivenPlayRate = FMath::GetMappedRangeValueClamped(
			FVector2D(ApecoxThirdPersonAim::TurnInPlaceThreshold, 100.0f),
			FVector2D(1.0f, 1.4f),
			FMath::Abs(RootYawOffset));
		const float TargetTurnPlayRate = FMath::Max(
			SpeedDrivenPlayRate, OffsetDrivenPlayRate);
		if (TargetTurnPlayRate > TurnInPlacePlayRate)
		{
			TurnInPlacePlayRate = TargetTurnPlayRate;
		}
		else
		{
			TurnInPlacePlayRate = FMath::FInterpTo(
				TurnInPlacePlayRate,
				TargetTurnPlayRate,
				SafeDeltaSeconds,
				ApecoxThirdPersonAim::TurnPlayRateBlendDownSpeed);
		}

		// TurnYawWeight 等于转身序列当前混合权重。RemainingTurnYaw 也会被同一权重缩放，
		// 因而先除以权重还原资产中的完整曲线值，再用相邻帧差值逐步交接根部角度。
		// 首个有效帧只记录基线，防止状态刚混入时把约 90 度一次性扣除。
		const float TurnYawWeight = GetCurveValue(FName(TEXT("TurnYawWeight")));
		if (TurnYawWeight > ApecoxThirdPersonAim::TurnYawCurveWeightEpsilon)
		{
			const float CurrentTurnYawCurveValue =
				GetCurveValue(FName(TEXT("RemainingTurnYaw"))) / TurnYawWeight;
			const bool bCurveChangedDirection = bHasPreviousTurnYawCurveValue
				&& CurrentTurnYawCurveValue * PreviousTurnYawCurveValue < 0.0f;
			const bool bCurveRestarted = bHasPreviousTurnYawCurveValue
				&& FMath::Abs(CurrentTurnYawCurveValue)
					> FMath::Abs(PreviousTurnYawCurveValue)
						+ ApecoxThirdPersonAim::TurnYawCurveRestartEpsilon;

			// 同一个状态在旧权重尚未归零时重入，RemainingTurnYaw 会从接近 0 跳回 +/-90。
			// 该跳变表示一轮新动作的起点，不是本帧真实旋转；只重建基线，不能把 90 度
			// 再次写入 RootYawOffset。反向重入同样以曲线符号变化识别。
			if (!bHasPreviousTurnYawCurveValue || bCurveChangedDirection || bCurveRestarted)
			{
				PreviousTurnYawCurveValue = CurrentTurnYawCurveValue;
				bHasPreviousTurnYawCurveValue = true;
				if (!FMath::IsNearlyZero(CurrentTurnYawCurveValue,
					ApecoxThirdPersonAim::TurnYawCurveWeightEpsilon))
				{
					bActiveTurnLeft = CurrentTurnYawCurveValue < 0.0f;
					bHasActiveTurnDirection = true;
				}
			}
			else
			{
				const float TurnYawCurveDelta =
					CurrentTurnYawCurveValue - PreviousTurnYawCurveValue;
				RootYawOffset = FRotator::NormalizeAxis(RootYawOffset - TurnYawCurveDelta);
				PreviousTurnYawCurveValue = CurrentTurnYawCurveValue;
			}
		}
		else
		{
			PreviousTurnYawCurveValue = 0.0f;
			bHasPreviousTurnYawCurveValue = false;
		}

		// 转身资产方向在曲线首次生效时锁定；明显的相反鼠标输入只发出本帧中断脉冲，
		// 让状态机先回到 Grounded/Crouched，再按新的 RootYawOffset 选择另一方向。
		if (bHasActiveTurnDirection
			&& ActorYawSpeed > ApecoxThirdPersonAim::TurnReverseInputSpeedThreshold)
		{
			const bool bInputTurnsLeft = ActorYawDelta < 0.0f;
			bShouldInterruptTurnInPlace = bInputTurnsLeft != bActiveTurnLeft;
		}

		if (bIsCrouching)
		{
			RootYawOffset = FMath::Clamp(
				RootYawOffset,
				ApecoxThirdPersonAim::CrouchedRootYawMin,
				ApecoxThirdPersonAim::CrouchedRootYawMax);
		}
		else
		{
			RootYawOffset = FMath::Clamp(
				RootYawOffset,
				ApecoxThirdPersonAim::StandingRootYawMin,
				ApecoxThirdPersonAim::StandingRootYawMax);
		}

		// Accumulate/Hold 阶段不保留上一次 BlendOut 的速度，避免重新静止时发生回弹。
		RootYawOffsetSpringState.Reset();
		bShouldTurnInPlace = FMath::Abs(RootYawOffset)
			> ApecoxThirdPersonAim::TurnInPlaceThreshold;
		bTurnInPlaceLeft = RootYawOffset > 0.0f;
	}
	else
	{
		// 移动、腾空或趴姿会中断原地转身；下次静止转身重新建立曲线基线。
		PreviousTurnYawCurveValue = 0.0f;
		bHasPreviousTurnYawCurveValue = false;
		bShouldTurnInPlace = false;
		bTurnInPlaceLeft = false;
		bShouldInterruptTurnInPlace = false;
		TurnInPlacePlayRate = 1.0f;
		bHasActiveTurnDirection = false;
		bActiveTurnLeft = false;

		// 开始移动或腾空后释放视觉根偏移。临界阻尼参数与 Lyra 默认保持一致。
		RootYawOffset = UKismetMathLibrary::FloatSpringInterp(
			RootYawOffset,
			0.0f,
			RootYawOffsetSpringState,
			ApecoxThirdPersonAim::RootYawSpringStiffness,
			ApecoxThirdPersonAim::RootYawSpringCriticalDamping,
			FMath::Max(DeltaSeconds, 0.0f),
			ApecoxThirdPersonAim::RootYawSpringMass,
			ApecoxThirdPersonAim::RootYawSpringTargetVelocityAmount);

		if (FMath::IsNearlyZero(RootYawOffset, 0.01f))
		{
			RootYawOffset = 0.0f;
			RootYawOffsetSpringState.Reset();
		}
	}

	// Rotate Root Bone 抵消多少角度，Aim Offset 就向相反方向补回多少，保持枪械对准玩家视角。
	AimYaw = -RootYawOffset;
}

void UApecoxCharacterAnimInstance::ResetAnimationState()
{
	// 把暴露状态恢复到默认中性值；不清理仍合法的 UObject 缓存（缓存失效由 CacheCharacterReferences 负责）
	GroundSpeed = 0.0f;
	VerticalSpeed = 0.0f;
	MovementDirection = 0.0f;
	bIsMoving = false;
	bIsFalling = false;
	bIsCrouching = false;
	bIsSprinting = false;
	bIsProne = false;
	LeanAmount = 0.0f;
	AimPitch = 0.0f;
	AimYaw = 0.0f;
	RootYawOffset = 0.0f;
	bShouldTurnInPlace = false;
	bTurnInPlaceLeft = false;
	bShouldInterruptTurnInPlace = false;
	TurnInPlacePlayRate = 1.0f;
	PreviousActorYaw = 0.0f;
	bHasPreviousActorYaw = false;
	PreviousTurnYawCurveValue = 0.0f;
	bHasPreviousTurnYawCurveValue = false;
	bHasActiveTurnDirection = false;
	bActiveTurnLeft = false;
	RootYawOffsetSpringState.Reset();
	AnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;
	ThirdPersonLeftHandIKAlpha = 1.0f;
}
