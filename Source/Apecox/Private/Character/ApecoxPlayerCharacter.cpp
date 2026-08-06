// Copyright Apecox. All Rights Reserved.

#include "Character/ApecoxPlayerCharacter.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

AApecoxPlayerCharacter::AApecoxPlayerCharacter()
{
	// Character 不创建 ASC；ASC 由 PlayerState 拥有，保证跨 Pawn 生命周期持久
	CachedAbilitySystemComponent = nullptr;
}

// --- IAbilitySystemInterface 转发：通过缓存引用访问 PlayerState 的 ASC ---

UAbilitySystemComponent* AApecoxPlayerCharacter::GetAbilitySystemComponent() const
{
	return CachedAbilitySystemComponent;
}

UApecoxAbilitySystemComponent* AApecoxPlayerCharacter::GetApecoxAbilitySystemComponent() const
{
	return CachedAbilitySystemComponent;
}

// --- Avatar 初始化：从 PlayerState 绑定 ActorInfo ---

void AApecoxPlayerCharacter::InitializeAbilitySystem()
{
	AApecoxPlayerState* ApecoxPS = GetPlayerState<AApecoxPlayerState>();
	if (!ApecoxPS)
	{
		return;
	}

	UApecoxAbilitySystemComponent* ASC = ApecoxPS->GetApecoxAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// 幂等检查：如果已经绑定同一个 ASC 且 AvatarActor 已经是 this，不做重复初始化
	if (CachedAbilitySystemComponent == ASC && ASC->GetAvatarActor() == this)
	{
		return;
	}

	// 如果本 Character 之前缓存了另一个 ASC（PlayerState 变更场景），复用统一解绑
	if (CachedAbilitySystemComponent && CachedAbilitySystemComponent != ASC)
	{
		UninitializeAbilitySystem();
	}

	// 修复 1：在绑定目标 ASC 之前，检查 ASC 上是否仍有旧 Avatar。
	// 客户端复制乱序时，新 Pawn 可能先到达并尝试绑定，
	// 而旧 Pawn 尚未执行 UnPossessed/EndPlay——此时必须主动清退旧 Avatar。
	AActor* ExistingAvatar = ASC->GetAvatarActor();
	if (ExistingAvatar && ExistingAvatar != this)
	{
		if (AApecoxPlayerCharacter* OldCharacter = Cast<AApecoxPlayerCharacter>(ExistingAvatar))
		{
			// 旧 Character 仍是 Apecox 类型：调用其统一解绑，
			// 让它取消旧 Avatar 上的活动 Ability 并清空缓存（同类 protected 函数可互相访问）
			OldCharacter->UninitializeAbilitySystem();
		}
		else
		{
			// 旧 Avatar 不是 Apecox PlayerCharacter——在当前 GameMode/Pawn 约束下不应出现
			ensureMsgf(false, TEXT("[Apecox] ASC %s has unexpected AvatarActor %s (type: %s) when %s is trying to bind. "
				"Performing minimal safe cleanup: clearing Avatar while preserving OwnerActor."),
				*GetNameSafe(ASC), *GetNameSafe(ExistingAvatar), *ExistingAvatar->GetClass()->GetName(),
				*GetNameSafe(this));

			// 最小安全清理：保留 OwnerActor，只解除旧 Avatar 绑定
			ASC->SetAvatarActor(nullptr);
		}
	}

	// 绑定新 ASC：OwnerActor = PlayerState，AvatarActor = this Character
	ASC->InitAbilityActorInfo(ApecoxPS, this);
	CachedAbilitySystemComponent = ASC;
}

// --- Avatar 解绑：对称清理，带旧 Pawn 防误清保护 ---

void AApecoxPlayerCharacter::UninitializeAbilitySystem()
{
	if (CachedAbilitySystemComponent)
	{
		// 关键保护：只有当前 Pawn 仍然是 ASC 的 Avatar 时才能操作 ActorInfo。
		// 客户端复制顺序可能导致旧 Pawn 的 UnPossessed/EndPlay 延迟执行——
		// 如果 ASC 已经绑定给新 Pawn，旧 Pawn 不能影响新 Pawn 的绑定。
		if (CachedAbilitySystemComponent->GetAvatarActor() == this)
		{
			// 取消此 Avatar 上的所有活动 Ability
			CachedAbilitySystemComponent->CancelAllAbilities();

			// 修复 2：正常换 Pawn 路径保留 PlayerState OwnerActor，只把 AvatarActor 置空。
			// PlayerState 是 ASC 的跨 Pawn 持久 Owner，ClearActorInfo() 同时清空 Owner 和 Avatar
			// 会破坏 ASC 在重生间隙的逻辑归属，与"PlayerState 持有 ASC"的架构前提冲突。
			// 仅当 OwnerActor 已失效时才回退到 ClearActorInfo()。
			// 不调用 RemoveAllGameplayCues()——持续 GE/Cue 跨 Avatar 策略尚未设计。
			if (CachedAbilitySystemComponent->GetOwnerActor() != nullptr)
			{
				CachedAbilitySystemComponent->SetAvatarActor(nullptr);
			}
			else
			{
				CachedAbilitySystemComponent->ClearActorInfo();
			}
		}

		// 无论是否仍是 Avatar，都清空本 Character 的缓存引用
		CachedAbilitySystemComponent = nullptr;
	}
}

// --- 服务器路径：Controller 完成占有后初始化 ---

void AApecoxPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 服务器上 Controller → PlayerState → Pawn 链已经就绪，可以安全初始化 ASC 绑定
	InitializeAbilitySystem();
}

// --- 客户端路径：PlayerState 复制到达后初始化 ---

void AApecoxPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 客户端在收到 PlayerState 后绑定 ASC ActorInfo。
	// 注意：不能只处理 IsLocallyControlled()——远端 Pawn（Simulated Proxy）
	// 后续也需要正确的 Avatar 信息以承载 GameplayCue 和表现层查询。
	if (GetPlayerState())
	{
		InitializeAbilitySystem();
	}
	else
	{
		// PlayerState 被清空（如玩家离开），安全解绑
		UninitializeAbilitySystem();
	}
}

// --- 解除占有：对称解绑，确保依赖链在解绑时仍然有效 ---

void AApecoxPlayerCharacter::UnPossessed()
{
	// 先解绑再调 Super，保证 ASC/Cached 在 Controller/PlayerState 引用失效前清理
	UninitializeAbilitySystem();

	Super::UnPossessed();
}

// --- 销毁：最终清理，确保不残留引用 ---

void AApecoxPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 在父类 EndPlay 之前解绑，此时组件/Controller 依赖仍完整
	UninitializeAbilitySystem();

	Super::EndPlay(EndPlayReason);
}
