// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ApecoxInventoryItemDefinition.generated.h"

class UApecoxInventoryItemInstance;

/**
 * UApecoxInventoryItemDefinition
 * - 不可变物品身份，继承 UPrimaryDataAsset 以获得编辑器异步加载支持。
 * - 为当前武器提供共享的身份入口与运行时实例创建路径。
 * - 不加入图标、重量、稀有度、价格等尚未实现的字段。
 */
UCLASS(BlueprintType, Const)
class APECOX_API UApecoxInventoryItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 编辑器和未来 UI 使用的显示名 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	/** 创建运行时实例的类型——武器 InstanceClass 必须是 UApecoxWeaponInstance 子类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UApecoxInventoryItemInstance> InstanceClass;
};
