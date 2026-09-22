// Copyright Apecox. All Rights Reserved.

#include "GameplayTags/ApecoxGameplayTags.h"

namespace ApecoxGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move",
		"二维移动输入意图——不是 Ability 身份，是稳定玩家移动方向");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse",
		"鼠标二维观察输入意图——不是 Ability 身份");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump",
		"跳跃输入意图——不是 Ability 身份");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch",
		"蹲伏输入意图——不是 GameplayAbility 身份，也不是角色蹲伏状态");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Sprint, "InputTag.Sprint",
		"按住冲刺输入意图——由 Character 根据姿态、落地状态和移动方向决定是否生效");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Prone, "InputTag.Prone",
		"切换趴下输入意图——由 Character 负责胶囊高度、起身空间和移动速度");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Lean_Left, "InputTag.Lean.Left",
		"按住向左探头输入意图——只驱动第一人称镜头和动画表现");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Lean_Right, "InputTag.Lean.Right",
		"按住向右探头输入意图——只驱动第一人称镜头和动画表现");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact",
		"交互键——拾取、开门、对话等，不是 Ability 身份");

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

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Fire, "InputTag.Weapon.Fire",
		"开火输入意图——从 IA 路由到武器 AbilitySpec，不负责射速或弹药");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Aim, "InputTag.Weapon.Aim",
		"按住式 ADS 输入意图——由 Character 驱动镜头、灵敏度、动画与射击瞄准分支");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Inspect, "InputTag.Weapon.Inspect",
		"按下一次触发一次的武器检视意图——由 Character 管理资格和打断关系");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Reload, "InputTag.Weapon.Reload",
		"按下一次请求一次服务器权威换弹——客户端不能指定弹量或提交时间");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Weapon_Laser, "InputTag.Weapon.Laser",
		"按下一次切换本地第一人称镭射——冲刺等状态只暂时隐藏，不清除用户开启意图");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Fire, "GameplayCue.Weapon.Fire",
		"开火表现通道——枪口 VFX、声音等，由本地预测并由服务器认可路径向远端表达");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Weapon_Impact, "GameplayCue.Weapon.Impact",
		"世界/角色命中点的粒子、声音、贴花等表现通道——只由 Authority 最终 HitResult 触发");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Damage",
		"武器配置传给原生 Instant Damage GameplayEffect 的有符号 Health 改变量");
}
