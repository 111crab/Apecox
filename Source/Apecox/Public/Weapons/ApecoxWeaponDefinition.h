// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ApecoxInventoryItemDefinition.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "StructUtils/InstancedStruct.h"
#include "ApecoxWeaponDefinition.generated.h"

class UApecoxWeaponPresentationDefinition;
class UApecoxAbilitySet;
class UApecoxWeaponInstance;

/** 单把武器类型的换弹规则；默认时间来自 RAR 步枪的实际 Montage/Notify。 */
USTRUCT(BlueprintType)
struct APECOX_API FApecoxWeaponReloadConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0"))
	int32 InitialReserveAmmo = 150;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.01", Units = "s"))
	float TacticalReloadDuration = 2.366667f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.0", Units = "s"))
	float TacticalReloadCommitTime = 1.101991f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.01", Units = "s"))
	float EmptyReloadDuration = 2.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.0", Units = "s"))
	float EmptyReloadCommitTime = 1.424366f;

	/** RAR Fire Rate Empty=450 RPM，因此两次空击反馈至少间隔 60/450 秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reload", meta = (ClampMin = "0.01", Units = "s"))
	float EmptyFireInterval = 0.133333f;

	bool IsValid() const
	{
		return InitialReserveAmmo >= 0
			&& FMath::IsFinite(TacticalReloadDuration) && TacticalReloadDuration > 0.0f
			&& FMath::IsFinite(TacticalReloadCommitTime) && TacticalReloadCommitTime >= 0.0f
			&& TacticalReloadCommitTime <= TacticalReloadDuration
			&& FMath::IsFinite(EmptyReloadDuration) && EmptyReloadDuration > 0.0f
			&& FMath::IsFinite(EmptyReloadCommitTime) && EmptyReloadCommitTime >= 0.0f
			&& EmptyReloadCommitTime <= EmptyReloadDuration
			&& FMath::IsFinite(EmptyFireInterval) && EmptyFireInterval > 0.0f;
	}
};

/**
 * UApecoxWeaponDefinition
 * - 通用武器根定义，继承 InventoryItemDefinition。
 * - 首个资产是步枪，但类本身不使用 Rifle 命名——通用武器槽承载任意武器。
 * - 携带表现路径、装备期间授予的 AbilitySet 和类型化射击配置。
 *
 * 为什么使用 TInstancedStruct<FApecoxWeaponFireConfig>：
 * - 射击模型配置通过受约束结构保存，避免开放任意 Fragment/Step 数组。
 * - TInstancedStruct 提供带基类约束的类型化变体——只允许 FApecoxWeaponFireConfig 派生结构，
 *   不开放任意 Struct 类型。错误类型的安全模板访问返回 nullptr 而非崩溃。
 * - 这与 Lyra 把许多射击字段直接放在 RangedWeaponInstance 的做法不同：
 *   Apecox 将不可变配置放在 Definition，可变运行时状态（弹匣、射速账本）放在 Instance。
 */
UCLASS(BlueprintType, Const)
class APECOX_API UApecoxWeaponDefinition : public UApecoxInventoryItemDefinition
{
	GENERATED_BODY()

public:
	UApecoxWeaponDefinition();

	/** FP/TP/世界拾取表现入口——独立 DataAsset 支持资产替换 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<const UApecoxWeaponPresentationDefinition> PresentationDefinition;

	/** 装备期间授予的 AbilitySet（撤销由 EquipmentComponent 通过句柄管理） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TArray<TObjectPtr<const UApecoxAbilitySet>> EquippedAbilitySets;

	/**
	 * 类型化射击配置——只允许 FApecoxWeaponFireConfig 派生结构。
	 * 当前正式武器使用 FApecoxProjectileFireConfig。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Fire")
	TInstancedStruct<FApecoxWeaponFireConfig> FireConfig;

	/** 备用弹药、换弹提交点和总时长；与具体弹道模型无关。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Reload")
	FApecoxWeaponReloadConfig ReloadConfig;

	/** 只读访问公共射击配置——空配置或类型不匹配时返回 nullptr */
	const FApecoxWeaponFireConfig* GetFireConfig() const { return FireConfig.GetPtr(); }

	/** 类型安全模板访问——错误类型返回 nullptr，不做 reinterpret/static 强转 */
	template<typename T>
	const T* GetFireConfig() const
	{
		static_assert(TIsDerivedFrom<T, FApecoxWeaponFireConfig>::Value,
			"T must be derived from FApecoxWeaponFireConfig");
		return FireConfig.GetPtr<T>();
	}
};
