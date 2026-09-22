// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystem/ApecoxAbilitySet.h"
#include "Inventory/ApecoxInventoryComponent.h"
#include "ApecoxEquipmentComponent.generated.h"

class UApecoxWeaponInstance;
class UApecoxWeaponDefinition;
class UApecoxWeaponPresentationDefinition;
class UApecoxAbilitySystemComponent;
class USkeletalMeshComponent;
class UAnimMontage;
class UAudioComponent;
class USceneComponent;
class AApecoxWeaponPresentationActor;

DECLARE_MULTICAST_DELEGATE_OneParam(FApecoxInspectPresentationEnded, bool /*bInterrupted*/);

// ============================================================================
// FApecoxEquippedWeaponState — 公开复制摘要
// ============================================================================

/**
 * 公开复制摘要——驱动 Owner 与 Simulated Proxy 各自的表现。
 *
 * 为什么私有 WeaponInstance 与公开 EquippedWeaponState 分离：
 * - Inventory 中的 WeaponInstance 是私有复制对象，仅 Owner 可见（COND_OwnerOnly）。
 * - 其他玩家看不到私人库存，但他们需要知道对方当前装备了什么武器。
 * - EquippedWeaponState 是一个精简的公开摘要，复制给所有相关客户端。
 * - 两个字段放入同一结构体通过一个 RepNotify 更新，避免分开发送导致的中间非法状态
 *   （如 Slot=Primary 但 WeaponDefinition 还是上一把枪）。
 */
USTRUCT()
struct FApecoxEquippedWeaponState
{
	GENERATED_BODY()

	/** 当前装备的武器槽位——None 表示空手 */
	UPROPERTY()
	EApecoxWeaponSlot Slot = EApecoxWeaponSlot::None;

	/** 当前装备的武器定义——nullptr 表示空手 */
	UPROPERTY()
	TObjectPtr<const UApecoxWeaponDefinition> WeaponDefinition = nullptr;

	/** 空手必须同时满足 Slot=None 和 WeaponDefinition=nullptr */
	bool IsUnarmed() const { return Slot == EApecoxWeaponSlot::None && WeaponDefinition == nullptr; }
};

// ============================================================================
// UApecoxEquipmentComponent
// ============================================================================

/**
 * UApecoxEquipmentComponent
 * - 由 AApecoxPlayerCharacter 创建，默认复制。
 * - Authority 管理装备真相：授予武器 AbilitySet、保存撤销句柄。
 * - 公开 EquippedWeaponState 摘要复制给所有客户端，驱动远端 TP 表现。
 * - 本地动态创建 FP 完整武器表现 Actor 与 TP Weapon Mesh Component 用于表现。
 *
 * 为什么 Equipment 在 Character：
 * - 装备与当前 Pawn 的移动、渲染和输入生命周期绑定。
 * - 死亡时 Pawn 被销毁，装备自动清理，重生后新 Pawn 从库存重新装备。
 * - 这是玩家"当前持有的武器"表现入口，不是持久持有状态。
 */
UCLASS(ClassGroup = (Apecox), meta = (BlueprintSpawnableComponent))
class APECOX_API UApecoxEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UApecoxEquipmentComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========================================================================
	// ASC 生命周期
	// ========================================================================

	void InitializeWithAbilitySystem(UApecoxAbilitySystemComponent* InASC);
	void UninitializeFromAbilitySystem();

	// ========================================================================
	// 装备与卸下（Authority Only）
	// ========================================================================

	/**
	 * Authority 装备武器：
	 * 1. 先验证 ASC、实例、Definition 和 Slot（全部在卸下前完成）。
	 * 2. 卸下当前旧装备（如果有）。
	 * 3. 使用 WeaponInstance 作为 SourceObject 授予 EquippedAbilitySets。
	 * 4. 写入 CurrentWeaponInstance 和 EquippedWeaponState 复制摘要。
	 * 5. 在非 Dedicated Server 环境刷新本地表现。
	 * @return true 装备成功；false 表示验证失败，未修改已有装备。
	 */
	bool EquipWeapon(EApecoxWeaponSlot Slot, UApecoxWeaponInstance* WeaponInstance);

	/**
	 * Authority 卸下当前武器（幂等）：
	 * 1. 用保存的 AbilitySetGrantedHandles 撤销武器 AbilitySet。
	 *    被移除的武器 Spec 通过 ASC::OnRemoveAbility 精确清理输入句柄，
	 *    不会误伤英雄技能。
	 * 2. 清空 CurrentWeaponInstance 和 EquippedWeaponState。
	 * 3. 销毁 FP 表现 Actor 和 TP 动态 Mesh Component。
	 */
	void UnequipCurrentWeapon();

	// ========================================================================
	// 访问器
	// ========================================================================

	const UApecoxWeaponDefinition* GetEquippedWeaponDefinition() const
	{
		return EquippedWeaponState.WeaponDefinition;
	}

	UApecoxWeaponInstance* GetCurrentWeaponInstance() const { return CurrentWeaponInstance; }

	bool IsArmed() const { return !EquippedWeaponState.IsUnarmed(); }

	/** 当前本地 FP 武器的左手握持锚点；未生成 FP 表现时返回 nullptr */
	USceneComponent* GetFirstPersonLeftHandGripPoint() const;

	// ========================================================================
	// 表现入口
	// ========================================================================

	/**
	 * 播放一次开火表现——由用户后续手工创建的 GameplayCue Blueprint（GCN_Weapon_Fire）调用。
	 *
	 * 为什么 Fire Cue 只触发表现：
	 * - 一次射击的合法性、扣弹、Trace 和伤害已经在 GA/服务器完成；
	 * - 本函数只把"已经成立的一发"翻译成当前视角的动画/枪口/音效，
	 *   不修改弹匣、命中结果或任何复制状态。
	 *
	 * 为什么用 IsLocallyControlled 选择 FP/TP：
	 * - 本地控制 Pawn 看到第一人称 Arms/武器；远端 Simulated Proxy 看到第三人称；
	 * - 不需要复制两个动画状态，本地角色关系本身就是稳定的分流依据。
	 */
	UFUNCTION(BlueprintCallable, Category = "Apecox|Equipment|Presentation")
	void PlayFirePresentation();

	/** Authority broadcasts one exact accepted projectile path as an unreliable cosmetic event. */
	void BroadcastProjectileTracer(const FVector& Origin, const FVector& Direction,
		float Speed, float GravityScale, float MaxDistance);

	/** 同帧启动本地第一人称 Arms/Weapon 检视 Montage 与声音；任一必要 Montage 失败则整体失败。 */
	bool PlayInspectPresentation();

	/** 停止当前检视的两条 Montage 和声音；可安全重复调用。 */
	void StopInspectPresentation(float BlendOutTime);

	/** 当前是否正在播放本地第一人称装备动作；AnimInstance 用它暂停左手握持 IK。 */
	bool IsEquipPresentationActive() const { return bEquipPresentationActive; }

	/** 结束本地装备动作和声音；用于开火、ADS、冲刺与姿态输入的响应式打断。 */
	void StopEquipPresentation(float BlendOutTime);

	/** 同帧启动普通/空仓的 Arms、Weapon 和声音换弹表现。 */
	bool PlayReloadPresentation(bool bEmptyReload);

	/** 中断换弹表现；自然完成由 Arms Montage 委托自行收口。 */
	void StopReloadPresentation(float BlendOutTime);

	/** Authority 写入公开换弹表现状态：0=无，1=普通，2=空仓。 */
	void SetReloadPresentationState(uint8 NewState);

	bool IsReloadPresentationActive() const { return bReloadPresentationActive; }
	bool IsReloading() const { return ReloadPresentationState != 0; }

	/** 空仓扣扳机反馈：只播放 Arms Montage 与空击声，不改变玩法状态。 */
	void PlayEmptyFirePresentation();

	/** 用户主动切换镭射意图；只有当前状态允许且配置完整时才改变并播放切换声。 */
	bool ToggleLaserPresentation();

	/** 根据用户保留的开启意图和当前动作许可刷新实际光束/光点显示。 */
	void RefreshLaserPresentation(bool bPresentationAllowed);

	bool IsLaserRequestedOn() const { return bLaserRequestedOn; }
	bool IsLaserPresentationEnabled() const { return bLaserPresentationEnabled; }

	/** 死亡序列开始时在所有端立即关闭并销毁独立武器表现，防止外部挂载组件消失后残留。 */
	void HandleOwnerDeathPresentation();

	/**
	 * Owning Client 完成本地占有后重建一次 FP/TP 表现。
	 * EquippedWeaponState 与 Controller 的复制顺序不固定；此入口保证摘要先到时误建的
	 * TP 表现会被移除，并按最终本地控制关系生成 FP Arms/Weapon。
	 */
	void RebuildPresentationAfterClientRestart();

	/** Arms Montage 自然结束或表现层被外部销毁时通知 Character 收口检视状态。 */
	FApecoxInspectPresentationEnded OnInspectPresentationEnded;

protected:
	// ========================================================================
	// 表现
	// ========================================================================

	/** EquippedWeaponState 复制回调——刷新本地和远端武器表现 */
	UFUNCTION()
	void OnRep_EquippedWeaponState();

	UFUNCTION()
	void OnRep_ReloadPresentationState();

	/** 远端可见镭射状态复制回调；只刷新第三人称纯表现。 */
	UFUNCTION()
	void OnRep_LaserPresentationEnabled();

	/** Autonomous Proxy 提交实际可见状态；服务器只接受当前已装备且支持镭射的武器。 */
	UFUNCTION(Server, Reliable)
	void ServerSetLaserPresentationEnabled(bool bEnabled);

	/** Each relevant rendered machine simulates its own lightweight tracer; gameplay stays server-only. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayProjectileTracer(FVector_NetQuantize10 Origin,
		FVector_NetQuantizeNormal Direction, float Speed, float GravityScale, float MaxDistance);

	void SetLaserPresentationEnabledAuthority(bool bEnabled);
	void ApplyThirdPersonLaserPresentation();

	/** 根据 EquippedWeaponState 创建/更新 FP 表现 Actor 和 TP 武器 Mesh */
	void RefreshWeaponPresentation();

	/** 销毁动态创建的 FP 表现 Actor 和 TP 武器 Mesh Component */
	void DestroyWeaponPresentation();

	/**
	 * 在指定 Mesh 上播放 Montage（空指针安全，不选择视角）。
	 * Character/Arms 已有 AnimBP，走 UAnimInstance::Montage_Play；
	 * 独立 Weapon Mesh（FP Presentation Actor 的 WeaponMesh 与 TP Weapon Mesh）没有 AnimInstance 时
	 * 回退到 Single Node PlayAnimation（只允许用于这两个 Weapon Mesh）。
	 * @return false 当 Mesh 或 Montage 为空；否则 true。
	 */
	bool PlayMontageOnMesh(USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage,
		float PlayRate = 1.0f) const;

	void StopMontageOnMesh(USkeletalMeshComponent* MeshComponent, UAnimMontage* Montage,
		float BlendOutTime) const;
	bool PlayEquipPresentation(const UApecoxWeaponPresentationDefinition* Presentation);
	void OnEquipArmsMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void FinishEquipPresentation(bool bInterrupted, bool bStopArmsMontage, float BlendOutTime);
	void OnInspectArmsMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void FinishInspectPresentation(bool bInterrupted, bool bStopArmsMontage, float BlendOutTime);
	void OnReloadCharacterMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void FinishReloadPresentation(bool bInterrupted, bool bStopCharacterMontage, float BlendOutTime);
	void ApplyReloadPresentationState();
	void BeginReloadMagazineVisuals(bool bEmptyReload,
		const UApecoxWeaponPresentationDefinition* Presentation);
	void HideLoadedReloadMagazine();
	void RestoreReloadMagazineVisuals();
	void ClearReloadMagazineVisualTimers();

	/**
	 * 在指定附着组件/附着点上生成一次性 Niagara 与音效（空指针/空 Socket 安全）。
	 * AttachPointName 非 None 时验证 Socket 存在（TP 的 MuzzleSocketName）；
	 * 为 None 时使用组件自身 Transform（FP 的 MuzzlePoint）。
	 * TP 武器在 MuzzleSocketName 为空时不回退到武器原点。
	 * VFX/SFX 只是表现，不影响射击合法性、扣弹和伤害。
	 */
	void PlayMuzzlePresentation(USceneComponent* AttachComponent, FName AttachPointName,
		const UApecoxWeaponPresentationDefinition* Presentation) const;

	// ========================================================================
	// 成员变量
	// ========================================================================

	/** 缓存的 ASC 引用——装备 AbilitySet 授予和撤销使用 */
	UPROPERTY()
	TObjectPtr<UApecoxAbilitySystemComponent> AbilitySystemComponent;

	/** 武器 AbilitySet 授予句柄——卸下时成对撤销，避免误伤英雄技能 */
	UPROPERTY()
	FApecoxAbilitySetGrantedHandles WeaponAbilitySetHandles;

	/**
	 * Server + Owner 精确装备身份——OwnerOnly 复制。
	 * Owning Client 用它与武器 GA 的 SourceObject 比较以通过 IsSourceWeaponCurrentlyEquipped()。
	 * Simulated Proxy 不看此值——继续通过 EquippedWeaponState 摘要驱动远端表现。
	 */
	UPROPERTY(Replicated)
	TObjectPtr<UApecoxWeaponInstance> CurrentWeaponInstance;

	/** 公开复制摘要——对所有相关客户端可见，驱动 TP 表现 */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponState)
	FApecoxEquippedWeaponState EquippedWeaponState;

	/** 对所有相关客户端可见的换弹表现摘要：0=无，1=普通，2=空仓。 */
	UPROPERTY(ReplicatedUsing = OnRep_ReloadPresentationState)
	uint8 ReloadPresentationState = 0;

	/**
	 * 当前实际可见的镭射状态，公开复制给观察者。它不是射击真相；只驱动远端挂件、Beam 与 Dot。
	 * Owner 仍立即使用本地 bLaserRequestedOn 显示第一人称结果。
	 */
	UPROPERTY(ReplicatedUsing = OnRep_LaserPresentationEnabled)
	bool bLaserPresentationEnabled = false;

	/** 动态创建的第一人称完整武器表现 Actor——附着到 GetFirstPersonMesh() */
	UPROPERTY(Transient)
	TObjectPtr<AApecoxWeaponPresentationActor> FirstPersonWeaponPresentationActor;

	/** 动态创建的第三人称武器 Mesh——附着到 GetMesh() */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ThirdPersonWeaponMeshComponent;

	/** 只在非拥有者视角创建；复用纯表现 Actor 的镭射组件并挂到 TP Weapon Mesh。 */
	UPROPERTY(Transient)
	TObjectPtr<AApecoxWeaponPresentationActor> ThirdPersonLaserPresentationActor;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveFirstPersonArmsInspectMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveFirstPersonWeaponInspectMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> InspectAudioComponent;

	bool bInspectPresentationActive = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveFirstPersonArmsEquipMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveFirstPersonWeaponEquipMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> EquipAudioComponent;

	bool bEquipPresentationActive = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveReloadCharacterMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveReloadWeaponMontage;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ActiveReloadCharacterMesh;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ActiveReloadWeaponMesh;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ReloadAudioComponent;

	bool bReloadPresentationActive = false;

	/** 本地用户的持久开关意图；冲刺等只暂时隐藏，不清除此值。 */
	bool bLaserRequestedOn = false;

	/** 防止 Character Tick 每帧重复发送相同的镭射表现 RPC。 */
	bool bLastSubmittedLaserPresentationEnabled = false;

	FTimerHandle ReloadHideLoadedMagazineTimerHandle;
	FTimerHandle ReloadRestoreMagazineTimerHandle;
};
