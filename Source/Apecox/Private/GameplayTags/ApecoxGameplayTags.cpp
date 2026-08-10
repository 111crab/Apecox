// Copyright Apecox. All Rights Reserved.

#include "GameplayTags/ApecoxGameplayTags.h"

namespace ApecoxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Ability_Tactical, "InputTag.Ability.Tactical",
		"英雄战术/小技能输入槽位——不是具体技能身份，仅表示玩家通过输入触发此槽位的能力");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move",
		"二维移动输入意图——不是 Ability 身份，是稳定玩家移动方向");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse",
		"鼠标二维观察输入意图——不是 Ability 身份");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump",
		"跳跃输入意图——不是 Ability 身份");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Input_AbilityBlocked, "State.Input.AbilityBlocked",
		"阻止玩家通过输入激活 Ability——不阻止服务器事件或 OnAvatarSet 被动激活");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Death, "GameplayEvent.Death",
		"OutOfHealth 后由 HealthComponent 在 Authority 发送，触发 DeathAbility 的稳定 GAS 事件");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death, "State.Death",
		"死亡状态父标签——加入 ActivationBlockedTags 统一阻止普通 GA 在死亡期间激活");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death_Dying, "State.Death.Dying",
		"生命归零、死亡流程尚未完成——与 State.Death.Dead 互斥");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death_Dead, "State.Death.Dead",
		"死亡流程完成、旧 Pawn 等待销毁或重生——与 State.Death.Dying 互斥");
}
