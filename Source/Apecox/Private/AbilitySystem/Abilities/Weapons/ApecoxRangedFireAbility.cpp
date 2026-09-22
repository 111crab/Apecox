// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/Abilities/Weapons/ApecoxRangedFireAbility.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/TargetData/ApecoxRangedShotTargetData.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxWeaponStateComponent.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Player/ApecoxPlayerController.h"
#include "Game/ApecoxGameState.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Logging/LogMacros.h"

// ========================================================================
// 服务端验证常量——保守、有名字、集中定义，不加入每武器配置
// ========================================================================
namespace ApecoxShotValidation
{
	/** 客户端 ViewOrigin 与权威视点的最大距离（厘米）——超过此值视为客户端数据异常 */
	static constexpr float MaxViewOriginDelta = 500.0f;

	/** 客户端 AimDirection 与权威方向的最大角度差（度） */
	static constexpr float MaxAimDirectionAngleDeg = 15.0f;

	/** 两阶段 Trace 第二阶段终点余量（厘米）——防止因浮点精度差在意图点前停止 */
	static constexpr float Stage2EndpointMargin = 1.0f;

	/** 发射源与意图点过近阈值（厘米）——小于此值时安全返回 Miss */
	static constexpr float MinFireOriginToIntentDistance = 1.0f;

	bool IsPostMatch(const UWorld* World)
	{
		const AApecoxGameState* GameState = World ? World->GetGameState<AApecoxGameState>() : nullptr;
		return GameState && GameState->GetApecoxMatchPhase() == EApecoxMatchPhase::PostMatch;
	}
}

// ========================================================================
// 构造
// ========================================================================

UApecoxRangedFireAbility::UApecoxRangedFireAbility()
{
	// 固定属性——一次激活 = 一发，由 ASC WhileInputActive + CanActivateAbility RPM 门控驱动
	ActivationPolicy = EApecoxAbilityActivationPolicy::WhileInputActive;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationGroup = EApecoxAbilityActivationGroup::Independent;
}

// ========================================================================
// CanActivateAbility — 本地 RPM/弹匣门控
// ========================================================================

bool UApecoxRangedFireAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 先执行父类检查（ActivationBlockedTags、并发组、来源武器装备校验）
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// GameState is replicated to every participant, so both player prediction and
	// server-controlled AI stop starting new shots as soon as the score limit is reached.
	if (ApecoxShotValidation::IsPostMatch(GetWorld()))
	{
		return false;
	}

	// 本地控制端：RPM/弹匣门控——在此处拒绝可防止 ASC 每帧预测激活后立即被拒绝
	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (Character && Character->IsWeaponFireBlockedByMovement())
		{
			return false;
		}

		const UApecoxRangedWeaponInstance* RangedWI =
			GetRangedWeaponInstanceFromSpec(Handle, ActorInfo);
		if (RangedWI)
		{
			UWorld* World = GetWorld();
			if (World)
			{
				if (!RangedWI->CanStartLocalShot(World->GetTimeSeconds()))
				{
					UE_LOG(LogTemp, Verbose,
						TEXT("[Apecox] RangedFireAbility::CanActivateAbility: Local RPM/ammo gate "
							"rejected. Ammo=%d, WorldTime=%.4f."),
						RangedWI->GetCurrentMagazineAmmo(),
						World->GetTimeSeconds());
					return false;
				}
			}
		}
	}

	// Remote Authority 不在此处门控——等待 TargetData 后用 CanCommitServerShot 验证
	return true;
}

// ========================================================================
// ActivateAbility
// ========================================================================

void UApecoxRangedFireAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	// 1. 验证 RangedWeaponInstance 和当前弹道配置
	const UApecoxRangedWeaponInstance* ConstRangedWI =
		GetRangedWeaponInstanceFromSpec(Handle, ActorInfo);
	if (!ConstRangedWI)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RangedFireAbility: SourceObject is not a RangedWeaponInstance — aborting."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsFireConfigValid(ConstRangedWI))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RangedFireAbility: No valid fire configuration for this model on weapon '%s' — aborting."),
			*GetNameSafe(ConstRangedWI->GetRangedWeaponDefinition()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 2. 再次验证来源武器仍是当前装备实例（Fix 1 修复后 Owning Client 可正确通过）
	if (!IsSourceWeaponCurrentlyEquipped(Handle, ActorInfo))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RangedFireAbility: Source weapon was unequipped during activation — aborting."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 对称注册 TargetData 委托
	const FPredictionKey PredictionKey = ActivationInfo.GetActivationPredictionKey();
	TargetDataDelegateHandle = ASC->AbilityTargetDataSetDelegate(Handle, PredictionKey)
		.AddUObject(this, &UApecoxRangedFireAbility::OnTargetDataReady);

	// 4. 非本地控制的 Authority（Dedicated Server）等待客户端 TargetData
	if (!ActorInfo->IsLocallyControlled() && ActorInfo->IsNetAuthority())
	{
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, PredictionKey);
		return;
	}

	// 5. Owning Local Player + Listen Host：走统一 OnTargetDataReady 路径
	if (ActorInfo->IsLocallyControlled())
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		const float WorldTimeSeconds = World->GetTimeSeconds();

		// 激活检查之后仍可能切换状态；在推进本地射速和ShotId之前再次确认。
		const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(ActorInfo->AvatarActor.Get());
		if (Character && Character->IsWeaponFireBlockedByMovement())
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		// 5a. 生成 Shot ID + 更新本地射速计时
		UApecoxRangedWeaponInstance* RangedWI = const_cast<UApecoxRangedWeaponInstance*>(ConstRangedWI);
		const uint32 ShotId = RangedWI->StartLocalShot(WorldTimeSeconds);
		CurrentShotId = ShotId;

		// 5b. 堆分配 TargetData——生命周期由 TargetDataHandle 的 TSharedPtr 接管
		FApecoxRangedShotTargetData* ShotData = BuildLocalShotTargetData(WorldTimeSeconds);
		ShotData->ShotId = ShotId;
		ShotData->ClientFireTimeSeconds = WorldTimeSeconds;

		// 本发TargetData已经冻结扣扳机时的瞄准方向；随后抬起真实视线，
		// 因此下一发自然围绕后坐后的中心方向计算散布。
		if (AApecoxPlayerCharacter* MutableCharacter =
			Cast<AApecoxPlayerCharacter>(ActorInfo->AvatarActor.Get()))
		{
			MutableCharacter->NotifyLocalWeaponShot(RangedWI, ShotData->BurstShotIndex);
		}

		// 5c. 将堆对象编入 TargetDataHandle——Handle 内部通过 TSharedPtr 管理释放
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		TargetDataHandle.Add(ShotData);

		// 5d. 统一走 OnTargetDataReady——不另写直发后永不回调的分支
		OnTargetDataReady(TargetDataHandle, FGameplayTag());
	}
}

// ========================================================================
// EndAbility
// ========================================================================

void UApecoxRangedFireAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 1. 先遵守 UE 5.8 原生有效性检查——无效时安全返回，由基类/ASC 决定是否终结
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	// 2. ScopeLock 中不能执行清理——将派生类自身加入 WaitingToExecute 然后返回
	//    必须绑定 UApecoxRangedFireAbility::EndAbility（非基类），否则延迟执行时跳过派生清理
	if (ScopeLockCount > 0)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] RangedFireAbility::EndAbility: ScopeLockCount=%d > 0 — "
				"deferring to WaitingToExecute."),
			ScopeLockCount);
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(
			this, &UApecoxRangedFireAbility::EndAbility,
			Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}

	// 3. 不再处于 ScopeLock 中——安全执行派生类清理
	// 对称移除 TargetData 委托
	if (TargetDataDelegateHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FPredictionKey PredictionKey = ActivationInfo.GetActivationPredictionKey();
		ActorInfo->AbilitySystemComponent->AbilityTargetDataSetDelegate(Handle, PredictionKey)
			.Remove(TargetDataDelegateHandle);
		TargetDataDelegateHandle.Reset();
	}

	// 消费 Client Replicated TargetData——防止残留
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FPredictionKey PredictionKey = ActivationInfo.GetActivationPredictionKey();
		ActorInfo->AbilitySystemComponent->ConsumeClientReplicatedTargetData(Handle, PredictionKey);
	}

	// 清理实例临时状态
	CurrentShotId = 0;

	// 4. 最后交由基类完成通用 GAS 清理
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ========================================================================
// OnTargetDataReady — 统一回调入口
// ========================================================================

void UApecoxRangedFireAbility::OnTargetDataReady(
	const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	const FGameplayAbilitySpecHandle Handle = GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActivationInfo ActivationInfo = GetCurrentActivationInfo();

	// 无效 TargetData 必须结束 Ability，不得静默留下 Active Spec
	if (!InData.IsValid(0))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[Apecox] RangedFireAbility: OnTargetDataReady with invalid/empty data — ending ability."));
		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
		return;
	}

	// CurrentActorInfo 或 ASC 无效时尽最大可能安全结束
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RangedFireAbility: OnTargetDataReady with invalid CurrentActorInfo or ASC — "
				"ending ability."));
		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	const FPredictionKey PredictionKey = ActivationInfo.GetActivationPredictionKey();

	const FGameplayAbilityTargetData* RawData = InData.Get(0);
	if (!RawData || !RawData->GetScriptStruct() || !RawData->GetScriptStruct()->IsChildOf(FApecoxRangedShotTargetData::StaticStruct()))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] RangedFireAbility: OnTargetDataReady received unexpected TargetData type '%s' "
				"— expected FApecoxRangedShotTargetData. Aborting."),
			*GetNameSafe(RawData ? RawData->GetScriptStruct() : nullptr));
		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
		return;
	}

	const FApecoxRangedShotTargetData* ShotData =
		static_cast<const FApecoxRangedShotTargetData*>(RawData);

	// A request may have been predicted just before the winning projectile resolved.
	// Recheck at TargetData commit time so Authority never consumes ammo or creates a
	// new projectile after PostMatch.
	if (ApecoxShotValidation::IsPostMatch(GetWorld()))
	{
		if (CurrentActorInfo->IsNetAuthority())
		{
			SendShotConfirmation(ShotData->ShotId, EApecoxShotConfirmation::Rejected);
		}
		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
		return;
	}

	// 每一发都读取当前玩法状态。尤其是Authority等待TargetData期间，角色可能已开始移动。
	// 必须在CommitAbility、扣弹与Fire Cue之前拒绝，而不是仅隐藏动画。
	const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(CurrentActorInfo->AvatarActor.Get());
	if (Character && Character->IsWeaponFireBlockedByMovement())
	{
		if (CurrentActorInfo->IsNetAuthority())
		{
			SendShotConfirmation(ShotData->ShotId, EApecoxShotConfirmation::Rejected);
		}
		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
		return;
	}

	// ========================================================================
	// 路径 A：Owning Client（非 Authority）——本地预测
	// ========================================================================
	if (CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority())
	{
		{
			FScopedPredictionWindow ScopedPrediction(ASC, PredictionKey);

			if (!CommitAbility(Handle, CurrentActorInfo, ActivationInfo))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Apecox] RangedFireAbility: Client CommitAbility failed for ShotId=%u."),
					ShotData->ShotId);
				EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
				return;
			}

			// 记录未确认射击——候选命中状态仅用于预测表现
			UApecoxWeaponStateComponent* WeaponState = GetWeaponStateComponent(CurrentActorInfo);
			if (WeaponState)
			{
				WeaponState->RecordUnconfirmedShot(ShotData->ShotId,
					ShotData->HitResult.bBlockingHit);
			}

			// 预测 Fire Cue——本地立即播放动画/音效
			const UApecoxRangedWeaponInstance* RangedWI =
				GetRangedWeaponInstanceFromSpec(Handle, CurrentActorInfo);
			ExecuteWeaponFireCue(CurrentActorInfo, RangedWI);

			// 发送 TargetData 给服务器
			ASC->CallServerSetReplicatedTargetData(Handle, PredictionKey, InData,
				FGameplayTag(), ASC->ScopedPredictionKey);
		}

		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, false);
		return;
	}

	// ========================================================================
	// 路径 B：Authority（Dedicated Server + Listen Host）——验证 + 结算
	// ========================================================================
	if (CurrentActorInfo->IsNetAuthority())
	{
		const UApecoxRangedWeaponInstance* ConstRangedWI =
			GetRangedWeaponInstanceFromSpec(Handle, CurrentActorInfo);

		if (!ConstRangedWI)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Apecox] RangedFireAbility: RangedWeaponInstance no longer valid "
					"on authority — ShotId=%u. Ending ability."),
				ShotData->ShotId);
			SendShotConfirmation(ShotData->ShotId, EApecoxShotConfirmation::Rejected);
			EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
			return;
		}

		UApecoxRangedWeaponInstance* RangedWI =
			const_cast<UApecoxRangedWeaponInstance*>(ConstRangedWI);

		{
			FScopedPredictionWindow ScopedPrediction(ASC, PredictionKey);

			if (!CommitAbility(Handle, CurrentActorInfo, ActivationInfo))
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[Apecox] RangedFireAbility: Authority CommitAbility failed for ShotId=%u."),
					ShotData->ShotId);
				SendShotConfirmation(ShotData->ShotId, EApecoxShotConfirmation::Rejected);
				EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
				return;
			}

			// 执行权威射击事务——内部含完整的验证+Trace+扣弹+Damage+Cue+Confirm 流程
			ProcessAuthoritativeShot(*ShotData, RangedWI);
		}

		EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, false);
		return;
	}

	// 路径 C：非 Owner、非 Authority 的 Remote Client——不应到达此处，必须结束
	UE_LOG(LogTemp, Verbose,
		TEXT("[Apecox] RangedFireAbility: OnTargetDataReady on non-owner non-authority — ending ability."));
	EndAbility(Handle, CurrentActorInfo, ActivationInfo, true, true);
}

// ========================================================================
// BuildLocalShotTargetData — 堆分配本地候选 Trace
// ========================================================================

FApecoxRangedShotTargetData* UApecoxRangedFireAbility::BuildLocalShotTargetData(
	float WorldTimeSeconds) const
{
	FApecoxRangedShotTargetData* ShotData = new FApecoxRangedShotTargetData();

	if (!CurrentActorInfo)
	{
		return ShotData;
	}

	// 玩家使用真实 PlayerController 视点；服务器 AI 没有 PlayerController，
	// 必须回退到 Pawn 的 ViewLocation/BaseAimRotation。旧实现对 AI 直接返回
	// 零坐标 TargetData，随后必然被 Authority 视点距离校验拒绝。
	APlayerController* PC = CurrentActorInfo->PlayerController.Get();
	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else if (const APawn* Pawn = Cast<APawn>(CurrentActorInfo->AvatarActor.Get()))
	{
		ViewLocation = Pawn->GetPawnViewLocation();
		ViewRotation = Pawn->GetBaseAimRotation();
	}
	else if (const AActor* Avatar = CurrentActorInfo->AvatarActor.Get())
	{
		ViewLocation = Avatar->GetActorLocation();
		ViewRotation = Avatar->GetActorRotation();
	}
	else
	{
		return ShotData;
	}

	ShotData->ViewOrigin = ViewLocation;
	ShotData->AimDirection = ViewRotation.Vector();
	if (const AApecoxPlayerCharacter* Character =
		Cast<AApecoxPlayerCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		ShotData->bIsAiming = Character->IsAiming();
	}

	// 从 WeaponInstance 获取 MaxRange
	const UApecoxRangedWeaponInstance* RangedWI =
		GetRangedWeaponInstanceFromSpec(GetCurrentAbilitySpecHandle(), CurrentActorInfo);
	const FApecoxRangedFireConfig* FireConfig =
		RangedWI ? RangedWI->GetRangedFireConfig() : nullptr;
	if (RangedWI)
	{
		ShotData->BurstId = RangedWI->GetLocalBurstId();
		ShotData->BurstShotIndex = RangedWI->GetLastStartedLocalBurstShotIndex();
	}
	const float MaxRange = FireConfig ? FireConfig->MaxRange : 10000.0f;
	const ECollisionChannel TraceChannel = FireConfig ?
		static_cast<ECollisionChannel>(FireConfig->TraceChannel.GetValue()) : ECC_Visibility;

	// 本地候选 Trace——只作预测视觉和诊断
	UWorld* World = GetWorld();
	if (World)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RangedFireClient), false);
		QueryParams.bReturnPhysicalMaterial = false;

		if (AActor* Avatar = CurrentActorInfo->AvatarActor.Get())
		{
			QueryParams.AddIgnoredActor(Avatar);
		}

		const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * MaxRange;

		FHitResult LocalHit;
		if (World->LineTraceSingleByChannel(LocalHit, ViewLocation, TraceEnd,
			TraceChannel, QueryParams))
		{
			ShotData->HitResult = LocalHit;
		}
	}

	return ShotData;
}

// ========================================================================
// ExecuteWeaponFireCue — 空指针安全的 Fire Cue 辅助
// ========================================================================

void UApecoxRangedFireAbility::ExecuteWeaponFireCue(
	const FGameplayAbilityActorInfo* ActorInfo,
	const UApecoxRangedWeaponInstance* RangedWeaponInstance) const
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !RangedWeaponInstance)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddInstigator(ActorInfo->AvatarActor.Get(),
		ActorInfo->AvatarActor.Get());

	FGameplayCueParameters CueParams;
	CueParams.EffectContext = EffectContext;
	CueParams.Instigator = ActorInfo->AvatarActor.Get();
	// SourceObject 指向 RangedWeaponInstance，让 Cue 系统知道此 Fire Cue 来自哪个武器
	CueParams.SourceObject = const_cast<UApecoxRangedWeaponInstance*>(RangedWeaponInstance);

	ASC->ExecuteGameplayCue(ApecoxGameplayTags::GameplayCue_Weapon_Fire, CueParams);
}

// ========================================================================
// ProcessAuthoritativeShot — 服务器完整验证 + Trace + 结算
// ========================================================================

void UApecoxRangedFireAbility::ProcessAuthoritativeShot(
	const FApecoxRangedShotTargetData& ShotData,
	UApecoxRangedWeaponInstance* RangedWeaponInstance)
{
	if (!CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo->AbilitySystemComponent.Get();
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float ServerTimeSeconds = World->GetTimeSeconds();
	const uint32 ShotId = ShotData.ShotId;

	// --- Step 1: 验证 Ability SourceObject 仍是当前装备实例 ---
	if (!IsSourceWeaponCurrentlyEquipped(GetCurrentAbilitySpecHandle(), CurrentActorInfo))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ProcessAuthoritativeShot: Source weapon not equipped — ShotId=%u rejected."),
			ShotId);
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}

	// --- Step 2: 验证 ShotId、RPM、弹匣和配置 ---
	if (!RangedWeaponInstance->CanCommitServerShot(
		ShotId, ServerTimeSeconds, ShotData.BurstId, ShotData.BurstShotIndex))
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Apecox] ProcessAuthoritativeShot: CanCommitServerShot failed — ShotId=%u rejected."),
			ShotId);
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}

	// --- Step 3: 验证客户端瞄准意图 VS 权威视点/方向 ---
	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}

	// 使用 Authority Pawn 的 GetPawnViewLocation() 和 GetBaseAimRotation()
	FVector AuthorityViewLocation;
	FRotator AuthorityAimRotation;

	if (APawn* Pawn = Cast<APawn>(Avatar))
	{
		AuthorityViewLocation = Pawn->GetPawnViewLocation();
		AuthorityAimRotation = Pawn->GetBaseAimRotation();
	}
	else
	{
		AuthorityViewLocation = Avatar->GetActorLocation();
		AuthorityAimRotation = Avatar->GetActorRotation();
	}

	// Reject non-finite input before distances/acos: NaN comparisons otherwise bypass limits.
    if (ShotData.ViewOrigin.ContainsNaN() || ShotData.AimDirection.ContainsNaN()
        || ShotData.AimDirection.IsNearlyZero() || !FMath::IsFinite(ShotData.ClientFireTimeSeconds))
    {
        SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
        return;
    }
    // 客户端 ViewOrigin 必须位于权威视点的有限容差内
	const float ViewOriginDelta = FVector::Dist(ShotData.ViewOrigin, AuthorityViewLocation);
	if (ViewOriginDelta > ApecoxShotValidation::MaxViewOriginDelta)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ProcessAuthoritativeShot: ViewOrigin delta %.1f exceeds max %.1f — "
				"ShotId=%u rejected."),
			ViewOriginDelta, ApecoxShotValidation::MaxViewOriginDelta, ShotId);
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}

	// AimDirection 必须与权威方向处于合理角度内
	const FVector AuthorityAimDir = AuthorityAimRotation.Vector();
	const FVector ClientAimDir = ShotData.AimDirection.GetSafeNormal();

	// Acos 前将 DotProduct 钳制到 [-1, 1]——防止量化/浮点误差产生 NaN
	const float DotProduct = FMath::Clamp(
		FVector::DotProduct(ClientAimDir, AuthorityAimDir), -1.0f, 1.0f);
	const float AimAngleDeg = FMath::RadiansToDegrees(FMath::Acos(DotProduct));

	if (AimAngleDeg > ApecoxShotValidation::MaxAimDirectionAngleDeg)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ProcessAuthoritativeShot: AimDirection angle %.1f deg exceeds max %.1f deg — "
				"ShotId=%u rejected."),
			AimAngleDeg, ApecoxShotValidation::MaxAimDirectionAngleDeg, ShotId);
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}

	if (!IsFireConfigValid(RangedWeaponInstance))
	{
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}
	const FApecoxRangedFireConfig* Config = RangedWeaponInstance->GetRangedFireConfig();
	const UCharacterMovementComponent* Movement = Avatar->FindComponentByClass<UCharacterMovementComponent>();
	const FVector HorizontalAcceleration = Movement
		? FVector(Movement->GetCurrentAcceleration().X, Movement->GetCurrentAcceleration().Y, 0.0f)
		: FVector::ZeroVector;
	const APawn* AvatarPawn = Cast<APawn>(Avatar);
	const FVector PendingMovement = AvatarPawn ? AvatarPawn->GetPendingMovementInputVector() : FVector::ZeroVector;
	const bool bHasMovementInput = !HorizontalAcceleration.IsNearlyZero()
		|| !FVector(PendingMovement.X, PendingMovement.Y, 0.0f).IsNearlyZero();
	const AApecoxPlayerCharacter* ApecoxCharacter = Cast<AApecoxPlayerCharacter>(Avatar);
	if (ShotData.bIsAiming && (!ApecoxCharacter || !ApecoxCharacter->CanAim()))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Apecox] ProcessAuthoritativeShot: invalid ADS intent — ShotId=%u rejected."), ShotId);
		SendShotConfirmation(ShotId, EApecoxShotConfirmation::Rejected);
		return;
	}
	const bool bIsAiming = ShotData.bIsAiming;
	FApecoxRangedShotTargetData SpreadShotData = ShotData;
	SpreadShotData.AimDirection = Config->ApplySpread(ShotData.AimDirection,
		ShotId, ShotData.BurstShotIndex, bIsAiming, bHasMovementInput);
	UE_LOG(LogTemp, Log,
		TEXT("[Apecox] ShotId=%u Burst=%u Index=%d SpreadMultiplier=%.3f Moving=%s Aiming=%s"),
		ShotId, ShotData.BurstId, ShotData.BurstShotIndex,
		Config->EvaluateSpreadMultiplier(ShotData.BurstShotIndex, bIsAiming, bHasMovementInput),
		bHasMovementInput ? TEXT("yes") : TEXT("no"), bIsAiming ? TEXT("yes") : TEXT("no"));
	ExecuteValidatedShot(SpreadShotData, RangedWeaponInstance);
}

void UApecoxRangedFireAbility::SendShotConfirmation(uint32 ShotId,
	EApecoxShotConfirmation Result) const
{
	UApecoxWeaponStateComponent* WeaponState = GetWeaponStateComponent(CurrentActorInfo);
	if (WeaponState)
	{
		WeaponState->SendShotConfirmation(ShotId, Result);
	}
}

// ========================================================================
// GetWeaponStateComponent
// ========================================================================

UApecoxWeaponStateComponent* UApecoxRangedFireAbility::GetWeaponStateComponent(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return nullptr;
	}

	AApecoxPlayerController* PC = Cast<AApecoxPlayerController>(ActorInfo->PlayerController.Get());
	if (!PC)
	{
		return nullptr;
	}

	return PC->FindComponentByClass<UApecoxWeaponStateComponent>();
}
