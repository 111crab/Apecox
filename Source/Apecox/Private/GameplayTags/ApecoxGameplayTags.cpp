// Copyright Apecox. All Rights Reserved.

#include "GameplayTags/ApecoxGameplayTags.h"

namespace ApecoxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability_Tactical, "InputTag.Ability.Tactical",
		"英雄战术/小技能输入槽位——不是具体技能身份，仅表示玩家通过输入触发此槽位的能力");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Input_AbilityBlocked, "State.Input.AbilityBlocked",
		"阻止玩家通过输入激活 Ability——不阻止服务器事件或 OnAvatarSet 被动激活");
}
