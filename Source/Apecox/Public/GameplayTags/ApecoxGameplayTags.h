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

	// --- 状态标签 ---
	// State.Input.AbilityBlocked：阻止玩家通过输入激活 Ability；
	// 不影响服务器事件、OnAvatarSet 被动激活或非玩家 Actor
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Input_AbilityBlocked);
}
