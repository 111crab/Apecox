// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "ApecoxPlayerCharacter.generated.h"

class UApecoxAbilitySystemComponent;
class UApecoxInputConfig;
class UApecoxAbilitySet;
class UApecoxHealthComponent;
struct FApecoxAbilitySetGrantedHandles;
class UInputMappingContext;
class UGameplayEffect;
class USkeletalMeshComponent;
class UCameraComponent;

UCLASS()
class APECOX_API AApecoxPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

	// 第一/第三人称表现访问器——为后续武器附着、ADS 和镜头系统提供稳定入口
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

protected:
	void InitializeAbilitySystem();
	void UninitializeAbilitySystem();

	virtual void PossessedBy(AController* NewController) override;
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

	// --- 死亡回调（由 HealthComponent 委托触发） ---
	/** 死亡开始：停止移动、关闭 Capsule 碰撞 */
	void OnDeathStarted(AActor* OwningActor);

	/** 死亡完成：所有端隐藏；Authority 安排下一 Tick 销毁并请求重生 */
	void OnDeathFinished(AActor* OwningActor);

	/** 下一 Tick 执行：隐藏旧 Pawn、缓存 Controller、DetachFromControllerPendingDestroy、
	 *  短 LifeSpan 兜底、向 GameMode 请求独立延迟重生 */
	void DestroyDueToDeath();

	// --- Pawn 初始化 GE ---
	/** Authority 上 ASC ActorInfo 建立后应用 PawnInitializationEffect（必须 Instant） */
	void ApplyPawnInitializationEffect();

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UApecoxInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	TObjectPtr<const UInputMappingContext> DefaultInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Input")
	int32 DefaultInputMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Ability")
	TArray<TObjectPtr<const UApecoxAbilitySet>> PawnAbilitySets;

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

private:
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
