// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "ApecoxPlayerCharacter.generated.h"

class UApecoxAbilitySystemComponent;

/**
 * AApecoxPlayerCharacter
 * - 当前 Pawn/Avatar；不创建第二个 ASC，只缓存 PlayerState 的 ASC 引用
 * - 在服务器占有和客户端 PlayerState 就绪时对称初始化 ActorInfo
 * - 解除占有或销毁时，只有当前 Avatar 才能解除自己的 Avatar 关系
 *
 * 设计原因：
 * - 玩家 ASC 的生命周期应长于单次 Pawn 生命周期（死亡/重生可复用的 Ability/属性），
 *   因此 ASC 由 PlayerState 拥有，Character 只作为 Avatar 绑定
 * - Character 实现 IAbilitySystemInterface 是为了方便依赖 Avatar 的系统
 *   （如动画蓝图、武器系统）通过 Pawn 直接访问 ASC，无需回溯 PlayerState
 */
UCLASS()
class APECOX_API AApecoxPlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AApecoxPlayerCharacter();

	// --- IAbilitySystemInterface（public：外部系统需要查询 ASC） ---
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 项目强类型 ASC 访问；转发到缓存的 PlayerState ASC
	UApecoxAbilitySystemComponent* GetApecoxAbilitySystemComponent() const;

protected:
	// --- Avatar 生命周期（成对 Init/Uninit；protected：ActorInfo 变更不应成为任意外部可访问 API） ---

	/**
	 * 从当前 PlayerState 取得 ASC，以 (PlayerState, this) 绑定 ActorInfo，并缓存 ASC 引用。
	 *
	 * 幂等保护：如果已经绑定同一个 ASC/Avatar 组合则跳过。
	 * 旧 Avatar 清退：如果目标 ASC 仍绑定另一个 Apecox Character（客户端复制乱序：
	 *   新 Pawn 先到达并尝试绑定，而旧 Pawn 尚未执行 UnPossessed/EndPlay），
	 *   先调用旧 Character 的 UninitializeAbilitySystem() 让其对称解绑，
	 *   再为本 Character 绑定。
	 * 若旧 Avatar 不是 Apecox Character（不应在 GameMode 限定下发生），
	 *   通过 ensureMsgf 暴露异常并做最小安全清理。
	 */
	void InitializeAbilitySystem();

	/**
	 * 对称解绑：只有当前 ASC 的 AvatarActor 确实是 this 时才能取消活动 Ability。
	 *
	 * 正常换 Pawn 路径保留有效 PlayerState OwnerActor，只将 AvatarActor 置空；
	 * 仅当 OwnerActor 已失效时才调用 ClearActorInfo()。
	 *
	 * 为什么必须检查 GetAvatarActor() == this？
	 * 客户端复制顺序可能让旧 Pawn 在解除占有后较晚才执行清理——
	 * 此时 ASC 的 AvatarActor 已经指向新 Pawn，旧 Pawn 不能影响新 Pawn 的绑定。
	 */
	void UninitializeAbilitySystem();

	// --- 服务器路径：Controller 分配完成，Pawn 已经收到 Possess ---
	virtual void PossessedBy(AController* NewController) override;

	// --- 客户端路径：PlayerState 复制到达，远端 Pawn 也需要正确的 Avatar 信息 ---
	virtual void OnRep_PlayerState() override;

	// --- 解除占有：在依赖链仍有效时对称解绑 ---
	virtual void UnPossessed() override;

	// --- 销毁：最终清理，确保不残留引用 ---
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// 只缓存当前 PlayerState 的 ASC 引用；标记 Transient 表示它不随 Character 序列化/保存
	// 命名用 "Cached" 前缀明确表示这是只读缓存，Character 不是 ASC 的 Owner
	UPROPERTY(Transient)
	TObjectPtr<UApecoxAbilitySystemComponent> CachedAbilitySystemComponent;
};
