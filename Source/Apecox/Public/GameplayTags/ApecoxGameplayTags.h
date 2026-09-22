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
	// InputTag.Move：二维移动输入——不是 Ability 身份，是稳定玩家移动意图
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);

	// InputTag.Look.Mouse：鼠标二维观察输入——不是 Ability 身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);

	// InputTag.Jump：跳跃输入——不是 Ability 身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);

	// InputTag.Crouch：稳定的蹲伏输入意图——不是 GameplayAbility 身份，也不是角色蹲伏状态
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);

	// 第一人称移动姿态输入：直接驱动 Character/CMC，不经过 GAS
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Sprint);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Prone);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Lean_Left);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Lean_Right);

	// InputTag.Interact：交互键——拾取步枪、开门、对话等，不是 Ability 身份
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);

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

	// --- 武器输入标签 ---
	// InputTag.Weapon.Fire：开火输入意图——从 IA 路由到武器 AbilitySpec
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Fire);

	// InputTag.Weapon.Aim：按住式 ADS 输入意图——由 Character 处理镜头与瞄准状态
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Aim);

	// InputTag.Weapon.Inspect：按下一次触发一次的第一人称武器检视意图
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Inspect);

	// InputTag.Weapon.Reload：按下一次请求一次服务器权威换弹
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Reload);

	// InputTag.Weapon.Laser：按下一次切换本地第一人称镭射意图
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Weapon_Laser);

	// --- 武器 GameplayCue 标签 ---
	// GameplayCue.Weapon.Fire：开火表现通道——枪口 VFX、声音等
	// GameplayCue.Weapon.Impact：命中表现通道——粒子、声音、贴花等
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Fire);
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Weapon_Impact);

	// --- SetByCaller 数据标签 ---
	// SetByCaller.Damage：武器配置提交给原生 Instant Damage GE 的有符号 Health 改变量。
	APECOX_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
}
