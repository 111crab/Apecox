// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ApecoxGameplayAbility.h"
#include "ApecoxWeaponGameplayAbility.generated.h"

class UApecoxWeaponInstance;
class UApecoxRangedWeaponInstance;
class UApecoxEquipmentComponent;

/**
 * UApecoxWeaponGameplayAbility
 * - 所有武器 Ability 的公共基类——负责来源武器验证和装备关系检查。
 * - 每个武器 GA 从 AbilitySpec 的 SourceObject 获取 WeaponInstance，
 *   从而区分自己来自哪个武器实例。
 *
 * 为什么必须通过传入 Handle 查找 Spec 而非依赖 GetCurrentAbilitySpec()：
 * - CanActivateAbility 可能在 CDO 或尚未建立 Current Spec 上下文时被调用；
 * - 传入的 Handle 是调用方持有的确定引用，从 ASC 反查 Spec 是安全路径。
 *
 * 为什么不缓存跨装备生命周期的裸 WeaponInstance 指针：
 * - 武器可能被卸下/切换——缓存的指针会变成悬空引用；
 * - 每次激活前重新验证 SourceObject 是当前装备实例，
 *   确保 GA 始终操作正确的武器。
 */
UCLASS(Abstract)
class APECOX_API UApecoxWeaponGameplayAbility : public UApecoxGameplayAbility
{
	GENERATED_BODY()

public:
	UApecoxWeaponGameplayAbility();

	// ========================================================================
	// 来源武器验证
	// ========================================================================

	/**
	 * 通过传入 SpecHandle 从 AbilitySpec 的 SourceObject 取得 WeaponInstance。
	 * 禁止激活前依赖 GetCurrentAbilitySpec()。
	 */
	const UApecoxWeaponInstance* GetWeaponInstanceFromSpec(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 类型安全取得 RangedWeaponInstance——不是远程武器类型时返回 nullptr */
	const UApecoxRangedWeaponInstance* GetRangedWeaponInstanceFromSpec(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;

	/**
	 * 验证 SourceObject 正是当前 UApecoxEquipmentComponent 装备的实例。
	 * 武器已卸下、Spec/SourceObject 无效或类型不符时安全拒绝。
	 */
	bool IsSourceWeaponCurrentlyEquipped(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo) const;

protected:
	// ========================================================================
	// 覆写
	// ========================================================================

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;
};
