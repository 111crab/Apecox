// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ApecoxAbilitySystemComponent.generated.h"

/**
 * UApecoxAbilitySystemComponent
 * - 建立 Apecox 项目级强类型 ASC 扩展点
 * - 本批不添加输入缓存、激活组、关系策略、RPC 或包装 API
 * - Character 生命周期逻辑由 Character 自身管理，不迁移到此
 */
UCLASS()
class APECOX_API UApecoxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
};
