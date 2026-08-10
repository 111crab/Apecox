// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Apecox Native GameplayTags
 * - 使用 UE 5.8 Native Gameplay Tags 命名空间 + 宏声明
 * - 不创建单例：Tag 本身就是全局注册，不需要额外包壳
 * - 只表达跨系统的稳定语义：输入意图、输入阻塞状态
 */
namespace ApecoxGameplayTags
{
	// --- 输入标签 ---
	// InputTag.Ability.Tactical：英雄战术/小技能输入槽位；不是具体技能身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Ability_Tactical);

	// InputTag.Move：二维移动输入——不是 Ability 身份，是稳定玩家移动意图
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);

	// InputTag.Look.Mouse：鼠标二维观察输入——不是 Ability 身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);

	// InputTag.Jump：跳跃输入——不是 Ability 身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);

	// --- 状态标签 ---
	// State.Input.AbilityBlocked：阻止玩家通过输入激活 Ability；
	// 不影响服务器事件、OnAvatarSet 被动激活或非玩家 Actor
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Input_AbilityBlocked);

	// --- 死亡事件标签 ---
	// GameplayEvent.Death：OutOfHealth 后由 HealthComponent 在 Authority 发送，
	// 作为 DeathAbility 的 GameplayEvent Trigger
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);

	// --- 死亡状态标签 ---
	// State.Death：死亡状态父标签，加入 ActivationBlockedTags 统一阻止普通 GA
	// State.Death.Dying：生命归零、死亡流程未完成（与 Dead 互斥）
	// State.Death.Dead：死亡流程完成、旧 Pawn 等待销毁/重生（与 Dying 互斥）
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death_Dying);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death_Dead);
}
