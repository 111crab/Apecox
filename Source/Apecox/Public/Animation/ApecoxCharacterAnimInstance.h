// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/ApecoxAnimationTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "ApecoxCharacterAnimInstance.generated.h"

class AApecoxPlayerCharacter;
class UCharacterMovementComponent;

/**
 * UApecoxCharacterAnimInstance
 * - 共享 AnimInstance 基类：只读取并缓存角色状态，不选择动画资产、不编写状态机、不播放 Montage。
 * - FP Arms AnimBP 与 TP Manny AnimBP 都继承本类，从同一份状态缓存读取，保证双视角一致。
 *
 * 为什么状态来自 Character / CMC / Equipment 而不是动画侧自行维护：
 * - CMC 是移动、跳跃、蹲伏的唯一状态来源，自带客户端预测、服务器校正与复制；
 * - EquipmentComponent 的公开装备摘要是动画族的唯一来源；
 * - 动画只解释这些玩法真相，避免动画侧维护一份会随时间漂移的重复状态。
 */
UCLASS()
class APECOX_API UApecoxCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// ========================================================================
	// 缓存与内部更新
	// ========================================================================

	/** 通过 TryGetPawnOwner() 缓存 OwningCharacter 与 CharacterMovement；无合法 Pawn 时安全清空 */
	void CacheCharacterReferences();

	/** 把全部暴露状态恢复到中性默认值；不清理仍合法的 UObject 缓存 */
	void ResetAnimationState();

	/** 从 CharacterMovement 读取移动、坠落、蹲伏状态 */
	void UpdateLocomotionState();

	/** 从 EquipmentComponent 公开装备摘要读取动画族 */
	void UpdateEquipmentState();

	/**
	 * 更新垂直 AimPitch，并维护第三人称表现专用的 RootYawOffset/AimYaw。
	 * Actor 继续立即跟随 Controller Yaw；这里只抵消 Mesh 的静止转角，不修改玩法旋转。
	 */
	void UpdateAimState(float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<AApecoxPlayerCharacter> OwningCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	/** RootYawOffset 回零使用的临界阻尼弹簧状态，不参与复制或玩法判定。 */
	FFloatSpringState RootYawOffsetSpringState;

	/** 上一动画帧的 Actor Yaw；只用于计算观察端本地的表现增量。 */
	float PreviousActorYaw = 0.0f;

	/** 首帧或 Pawn 更换后先建立 Yaw 基线，避免冷启动产生大角度跳变。 */
	bool bHasPreviousActorYaw = false;

	/** 上一有效帧中解除混合权重后的 RemainingTurnYaw，用于消费转身动画曲线增量。 */
	float PreviousTurnYawCurveValue = 0.0f;

	/** 首个有效曲线帧只建立基线，避免状态混入时把整段 90 度一次性应用。 */
	bool bHasPreviousTurnYawCurveValue = false;

	/** 当前正在播放的转身资产方向；用于识别玩家在动作中反向输入。 */
	bool bHasActiveTurnDirection = false;

	/** True 表示当前有效的转身曲线来自左转资产。 */
	bool bActiveTurnLeft = false;

	// ========================================================================
	// 暴露给 AnimBP 的状态
	// ========================================================================

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	float GroundSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	float VerticalSpeed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	float MovementDirection = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	bool bIsMoving = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	bool bIsFalling = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	bool bIsCrouching = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	bool bIsSprinting = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	bool bIsProne = false;

	/** -1 为完全左探，0 为居中，1 为完全右探 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Locomotion")
	float LeanAmount = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	float AimPitch = 0.0f;

	/**
	 * 水平 Aim Offset 输入。角色本身已经随控制器旋转，因此该值来自视觉根偏移的反值，
	 * 而不是 BaseAimRotation 与 ActorRotation 的水平差。
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	float AimYaw = 0.0f;

	/**
	 * 静止时抵消 Actor Yaw 变化的表现角度；连接到 AnimGraph 的 Rotate Root Bone/Yaw。
	 * 该值不复制、不影响胶囊、射击方向或第三人称镭射的玩法真相。
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	float RootYawOffset = 0.0f;

	/** 静止接地且视觉根偏移超过 35 度时为真；用于进入第三人称 90 度原地转身状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	bool bShouldTurnInPlace = false;

	/** 当前应播放左转序列。正 RootYawOffset 对应左转；负值对应右转。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	bool bTurnInPlaceLeft = false;

	/** 玩家输入方向已经与当前转身资产相反；返回转换应立即中断旧方向动作。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	bool bShouldInterruptTurnInPlace = false;

	/**
	 * 原地转身序列播放速率。慢速观察保持 1.0；积压角度较大时适度提高，
	 * 上限刻意保持自然，避免脚步呈现高频小碎步。
	 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Aim")
	float TurnInPlacePlayRate = 1.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Equipment")
	EApecoxCharacterAnimationFamily AnimationFamily = EApecoxCharacterAnimationFamily::Unarmed;

	/** Lyra 的 DisableLHandIK 动画曲线反值；连接到 TP 左手骨骼约束 Alpha。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Apecox|Animation|Equipment")
	float ThirdPersonLeftHandIKAlpha = 1.0f;
};
