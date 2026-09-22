// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Kismet/KismetMathLibrary.h"
#include "ApecoxPlayerCharacter.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxInputConfig;
class UApecoxAbilitySet;
class UApecoxHealthComponent;
class UApecoxEquipmentComponent;
struct FApecoxAbilitySetGrantedHandles;
class UInputMappingContext;
class UGameplayEffect;
class USkeletalMeshComponent;
class UCameraComponent;
class USoundBase;
class AApecoxWeaponPickup;
class UApecoxRangedWeaponInstance;
class UApecoxWeaponDefinition;
class UApecoxWeaponPresentationDefinition;
struct FApecoxRecoilSpringSettings;

UCLASS()
class APECOX_API AApecoxPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetPawnViewLocation() const override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

	bool IsSprinting() const { return bIsSprinting; }
	bool IsProne() const { return bIsProne; }
	bool IsAiming() const { return bIsAiming; }
	bool IsInspecting() const { return bIsInspecting; }
	bool IsReloading() const;
	float GetAimAlpha() const { return AimAlpha; }
	/** 当前装备与姿态是否允许 ADS；供输入和 Authority 射击意图校验共用。 */
	bool CanAim() const;
	/** 当前本地动作是否允许显示或主动切换镭射；冲刺、换弹、检视和装备表现期间关闭。 */
	bool CanUseWeaponLaser() const;
	/** 只判断运动是否阻止新一发；弹匣、射速与装备校验仍由武器能力负责。 */
	bool IsWeaponFireBlockedByMovement() const;
	float GetLeanAmount() const { return CurrentLeanAmount; }

	/** 合法本地预测射击生成TargetData后调用：按当前连发索引推进RAR分层后坐。 */
	void NotifyLocalWeaponShot(const UApecoxRangedWeaponInstance* Weapon, int32 BurstShotIndex);
	FVector GetFirstPersonWeaponRecoilLocation() const { return CurrentWeaponRecoilLocation; }
	FRotator GetFirstPersonWeaponRecoilRotation() const;

	// 第一/第三人称表现访问器——为后续武器附着、ADS 和镜头系统提供稳定入口
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	/** 只读访问装备组件 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment")
	UApecoxEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	/** 只读访问生命值组件 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Health")
	UApecoxHealthComponent* GetHealthComponent() const { return HealthComponent; }

protected:
	virtual void BeginPlay() override;

	void InitializeAbilitySystem();
	void UninitializeAbilitySystem();
	/** Authority：让新 Pawn 复用现有主武器，或通过库存正式事务创建默认步枪。 */
	void TryEquipStartingWeapon();

	virtual void PossessedBy(AController* NewController) override;
	virtual void PawnClientRestart() override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void GrantPawnAbilitySets();
	void RemovePawnAbilitySets();

	/** 修复 9：对称移除本地 IMC——Setup 重装前和 UnPossessed/EndPlay 清理时调用 */
	void RemoveDefaultInputMappingContext();

	// 回调签名：void(const FInputActionValue&, FGameplayTag)
	void HandleAbilityInputTagPressed(const FInputActionValue& ActionValue, FGameplayTag InputTag);
	void HandleAbilityInputTagReleased(const FInputActionValue& ActionValue, FGameplayTag InputTag);

	// --- Native 输入函数（第一/第三人称共用） ---
	// 第一人称输入不需要单独网络 RPC——移动和观察全部走 CMC 原生客户端预测和服务器校正。
	// 角色 Yaw 跟随 Controller（bUseControllerRotationYaw=true），
	// 因此 HandleMoveInput 的 Actor Forward/Right 方向与第一人称视角方向一致。
	void HandleMoveInput(const FInputActionValue& ActionValue);
	void HandleLookInput(const FInputActionValue& ActionValue);
	void HandleJumpStarted(const FInputActionValue& ActionValue);
	void HandleJumpCompleted(const FInputActionValue& ActionValue);
	void HandleCrouchStarted(const FInputActionValue& ActionValue);
	void HandleCrouchCompleted(const FInputActionValue& ActionValue);
	void HandleSprintStarted(const FInputActionValue& ActionValue);
	void HandleSprintCompleted(const FInputActionValue& ActionValue);
	void HandleProneStarted(const FInputActionValue& ActionValue);
	void HandleAimStarted(const FInputActionValue& ActionValue);
	void HandleInspectStarted(const FInputActionValue& ActionValue);
	void HandleReloadStarted(const FInputActionValue& ActionValue);
	void HandleLaserToggleStarted(const FInputActionValue& ActionValue);
	void HandleLeanLeftStarted(const FInputActionValue& ActionValue);
	void HandleLeanLeftCompleted(const FInputActionValue& ActionValue);
	void HandleLeanRightStarted(const FInputActionValue& ActionValue);
	void HandleLeanRightCompleted(const FInputActionValue& ActionValue);

	void UpdateLocomotionState(float DeltaSeconds);
	void UpdateMovementAudio();
	float GetCurrentFootstepDistance() const;
	bool AccumulateFootstepDistance(float AddedDistance, float RequiredDistance);
	void PlayMovementSoundAtLocation(USoundBase* Sound) const;
	void PlayStanceSound(USoundBase* Sound) const;
	void ApplyWeaponFireMovementPriority();
	void RefreshMovementSpeed();
	bool CanSprint() const;
	bool CanInspect() const;
	void CancelEquipPresentation();
	void CancelInspect();
	void OnInspectPresentationEnded(bool bInterrupted);
	void UpdateWeaponReloadState();
	bool TryStartWeaponReloadAuthority();
	void CommitWeaponReload();
	void FinishWeaponReload();
	void CancelWeaponReload();
	void TryPlayEmptyFireFeedback();

	UFUNCTION(Server, Reliable)
	void ServerRequestReload();
	bool SetProne(bool bNewProne);
	bool CanExitProne(float TargetHalfHeight) const;
	void UpdateFirstPersonCamera(float DeltaSeconds);
	void UpdateAimState(float DeltaSeconds);
	void ResetAimState();
	const UApecoxWeaponPresentationDefinition* GetEquippedPresentationDefinition() const;
	void UpdateFirstPersonLookSway(float DeltaSeconds);
	void UpdateFirstPersonRecoil(float DeltaSeconds);
	void StartCameraRecoilKick(const FVector& NewTarget, bool bStartsNewBurst);
	void ResetFirstPersonRecoil();
	float GetTargetViewHeight() const;
	FVector GetCollisionLimitedCameraBase(const FVector& CameraBaseLocation) const;
	float GetCollisionLimitedLeanTarget(float DesiredLeanAmount, const FVector& CameraBaseLocation) const;

	/** 交互输入 —— E 键拾取等 */
	void HandleInteractStarted(const FInputActionValue& ActionValue);

	/** 使用 FirstPersonCamera 做本地短距离 Visibility Trace——仅用于体验和候选选择 */
	AApecoxWeaponPickup* FindWeaponPickupCandidate() const;

	// --- 死亡回调（由 HealthComponent 委托触发） ---
	/** 死亡开始：停止移动、关闭 Capsule 碰撞 */
	void OnDeathStarted(AActor* OwningActor);

	/** 死亡完成：所有端隐藏；Authority 安排下一 Tick 销毁并请求重生 */
	void OnDeathFinished(AActor* OwningActor);

	/** 下一 Tick 执行：隐藏旧 Pawn、缓存 Controller、DetachFromControllerPendingDestroy、
	 *  短 LifeSpan 兜底、向 GameMode 请求独立延迟重生 */
	void DestroyDueToDeath();

	/**
	 * Pawn 类型自己的重生入口。玩家复用原 Controller；AI 子类可销毁旧 Controller，
	 * 并让新 Pawn 通过 AutoPossessAI 建立一套新的 Controller/PlayerState/ASC。
	 */
	virtual void RequestRespawnFromGameMode(AController* RespawnController);

	// --- Pawn 初始化 GE ---
	/** Authority 上 ASC ActorInfo 建立后应用 PawnInitializationEffect（必须 Instant） */
	void ApplyPawnInitializationEffect();

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UApecoxInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	int32 DefaultInputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement", meta = (ClampMin = "0.0"))
	float SprintSpeed = 650.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement", meta = (ClampMin = "0.0"))
	float CrouchedSpeed = 230.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement", meta = (ClampMin = "0.0"))
	float ProneSpeed = 170.0f;

	/** RAR式距离驱动脚步；默认步距与当前 400cm/s 普通移动基线配套。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement")
	TObjectPtr<USoundBase> FootstepSound;

	/** 真实Landed事件触发的落地Cue。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement")
	TObjectPtr<USoundBase> LandingSound;

	/** 实际进入/退出蹲伏或趴姿后播放；趴姿复用RAR的同一对布料Cue。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Stance")
	TObjectPtr<USoundBase> CrouchStartSound;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Stance")
	TObjectPtr<USoundBase> CrouchStopSound;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float WalkFootstepDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float AimFootstepDistance = 170.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float SprintFootstepDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Audio|Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float LowStanceFootstepDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement", meta = (ClampMin = "1.0"))
	float ProneCapsuleHalfHeight = 34.0f;

	/** 视点距胶囊底部的未缩放高度（cm），与碰撞半高独立。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Stance", meta = (ClampMin = "0.0", Units = "cm"))
	float CrouchedViewHeight = 129.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Stance", meta = (ClampMin = "0.0", Units = "cm"))
	float ProneViewHeight = 50.0f;

	/** 指数插值速度；12 约在 0.25 秒内完成 95% 的高度变化，0 表示立即切换。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Stance", meta = (ClampMin = "0.0"))
	float StanceCameraInterpSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Look Sway")
	bool bEnableFirstPersonLookSway = true;

	/** 镜头空间的最大滞后角度（度），不修改真实瞄准方向。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Look Sway")
	FRotator LookSwayMaxRotation = FRotator(1.5f, 2.0f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Look Sway", meta = (ClampMin = "1.0"))
	float LookSwayReferenceTurnSpeed = 90.0f;

	/** 镜头空间位移上限（cm）：Y左右、Z上下，随转向输入反向滞后。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Look Sway", meta = (Units = "cm"))
	FVector LookSwayMaxLocation = FVector(0.0f, 1.5f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|FirstPerson|Look Sway", meta = (ClampMin = "0.02", Units = "s"))
	float LookSwaySmoothingTime = 0.10f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement|Lean", meta = (ClampMin = "0.0"))
	float MaxLeanDistance = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement|Lean", meta = (ClampMin = "0.0"))
	float MaxLeanAngle = 15.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement|Lean", meta = (ClampMin = "0.0"))
	float LeanInterpSpeed = 9.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Movement|Lean", meta = (ClampMin = "0.0"))
	float LeanProbeRadius = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Ability")
	TArray<TObjectPtr<const UApecoxAbilitySet>> PawnAbilitySets;

	/**
	 * 玩家出生时自动放入 Primary 并装备的武器。
	 * 走 Inventory -> Equipment 正式事务，因此弹药、GAS、复制摘要和 FP/TP 表现保持同一事实来源。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Equipment")
	TObjectPtr<const UApecoxWeaponDefinition> StartingWeaponDefinition;

	/** 新 Pawn 出生时应用的 Instant GE（恢复 MaxHealth 和 Health 等）。
	 *  不是 Instant 的 GE 将被拒绝（ensureMsgf），防止留下未跟踪 Active GE。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Health")
	TSubclassOf<UGameplayEffect> PawnInitializationEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Health")
	TObjectPtr<UApecoxHealthComponent> HealthComponent;

	// --- 第一/第三人称表现分离 ---
	// 同一个 ACharacter、同一个 Capsule、同一个 CMC 负责唯一的移动模拟与网络预测。
	// 第一/第三人称只拆分视觉层：FirstPersonMesh 仅拥有者可见，
	// GetMesh()（第三人称全身）仅非拥有者可见（OwnerNoSee）。
	// 这样远端玩家看到的始终是第三人称表现，本地玩家始终是第一人称表现，
	// 但移动状态、碰撞和网络校正全部共享。

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|FirstPerson")
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|FirstPerson")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apecox|Equipment")
	TObjectPtr<UApecoxEquipmentComponent> EquipmentComponent;

private:
	// 让行为回归测试调用既有输入/姿态入口，不为测试新增蓝图或玩法 API。
	friend class FApecoxProneTransitionTest;
	friend class FApecoxStanceCameraTest;
	friend class FApecoxMovementCombatTest;
	friend class FApecoxLookSwayTest;
	friend class FApecoxTurningTest;
	friend class FApecoxWeaponRecoilTest;
	friend class FApecoxWeaponReloadTest;

	FTransform BaseFirstPersonMeshRelativeTransform = FTransform::Identity;
	FRotator PreviousLookSwayControlRotation = FRotator::ZeroRotator;
	FVector LookSwayAngles = FVector::ZeroVector;
	FVector LookSwayAngularVelocity = FVector::ZeroVector;
	FVector LookSwayLocation = FVector::ZeroVector;
	FVector LookSwayLocationVelocity = FVector::ZeroVector;
	TWeakObjectPtr<AController> LookSwayController;
	bool bLookSwayActive = false;

	/** RAR三条并行弹簧：枪械位置、枪械旋转、真实镜头旋转。 */
	TWeakObjectPtr<const UApecoxRangedWeaponInstance> RecoilWeapon;
	TWeakObjectPtr<AController> RecoilController;
	FVector TargetWeaponRecoilLocation = FVector::ZeroVector;
	FVector CurrentWeaponRecoilLocation = FVector::ZeroVector;
	FVector TargetWeaponRecoilRotation = FVector::ZeroVector;
	FVector CurrentWeaponRecoilRotation = FVector::ZeroVector;
	FVector TargetCameraRecoilRotation = FVector::ZeroVector;
	FVector CurrentCameraRecoilRotation = FVector::ZeroVector;
	FVector AppliedCameraRecoilRotation = FVector::ZeroVector;
	/** 本发镜头后坐尚未到达曲线目标；即使最后一发打空，也要先完成这次抬枪。 */
	bool bCameraRecoilKickActive = false;
	FVectorSpringState WeaponRecoilLocationSpringState;
	FVectorSpringState WeaponRecoilRotationSpringState;
	FVectorSpringState CameraRecoilRotationSpringState;
	/** X=Stiffness，Y=CriticalDampingFactor，Z=Mass。 */
	FVector WeaponRecoilLocationSpringParameters = FVector(1.0, 0.5, 0.002);
	FVector WeaponRecoilRotationSpringParameters = FVector(1.0, 0.5, 0.006);
	FVector CameraRecoilRotationSpringParameters = FVector(0.6, 0.5, 0.002);

	bool bSprintInputHeld = false;
	bool bWeaponFireInputHeld = false;
	/** 单击 ADS 的持久意图；再次单击或被冲刺/换弹/死亡等高优先级动作清除。 */
	bool bAimInputRequested = false;
	bool bLeanLeftInputHeld = false;
	bool bLeanRightInputHeld = false;
	bool bIsSprinting = false;
	bool bIsProne = false;
	bool bIsAiming = false;
	bool bIsInspecting = false;
	/** 上一帧本地观察到的 EApecoxWeaponReloadState 数值，用于复制状态到表现的边沿检测。 */
	uint8 ObservedReloadState = 0;
	float LastEmptyFireFeedbackTime = -BIG_NUMBER;
	TWeakObjectPtr<UApecoxRangedWeaponInstance> ReloadingWeapon;
	FTimerHandle ReloadCommitTimerHandle;
	FTimerHandle ReloadFinishTimerHandle;
	float AimAlpha = 0.0f;
	float BaseCameraFieldOfView = 90.0f;
	float BaseFirstPersonFieldOfView = 90.0f;

	float StandingCapsuleHalfHeight = 96.0f;
	bool bStanceCameraInitialized = false;
	bool bFootstepLocationInitialized = false;
	bool bSuppressNextCrouchStartSound = false;
	float AccumulatedFootstepDistance = 0.0f;
	FVector LastFootstepLocation = FVector::ZeroVector;
	float CurrentViewHeight = 0.0f;
	float CurrentLeanAmount = 0.0f;
	FVector BaseFirstPersonCameraRelativeLocation = FVector::ZeroVector;
	FRotator BaseFirstPersonCameraRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	TObjectPtr<UApecoxAbilitySystemComponent> CachedAbilitySystemComponent;

	UPROPERTY(Transient)
	TArray<uint32> AbilityInputBindingHandles;

	UPROPERTY(Transient)
	TArray<FApecoxAbilitySetGrantedHandles> GrantedPawnAbilitySetHandles;

	// 绑定 HealthComponent 死亡委托的句柄，UninitializeAbilitySystem 时解绑
	FDelegateHandle OnDeathStartedHandle;
	FDelegateHandle OnDeathFinishedHandle;
};
