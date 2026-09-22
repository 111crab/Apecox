// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ApecoxInventoryItemInstance.h"
#include "ApecoxWeaponInstance.generated.h"

class UApecoxWeaponDefinition;

/**
 * UApecoxWeaponInstance
 * - 武器运行时实例——提供类型安全的 GetWeaponDefinition() 访问。
 * - 当前无可变武器字段；Phase 2B 的弹匣、散布、配件和射击序号归属于这里。
 * - 装备 GA 将此实例作为 SourceObject 授予，用于分辨伤害来源和武器身份。
 */
UCLASS()
class APECOX_API UApecoxWeaponInstance : public UApecoxInventoryItemInstance
{
	GENERATED_BODY()

public:
	/** 类型安全的武器定义访问——等价于 Cast<UApecoxWeaponDefinition>(GetItemDefinition()) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	const UApecoxWeaponDefinition* GetWeaponDefinition() const;
};
