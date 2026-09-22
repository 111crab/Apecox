// Copyright Apecox. All Rights Reserved.

#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponInstance.h"

UApecoxWeaponDefinition::UApecoxWeaponDefinition()
{
	// 武器定义默认创建的运行时实例类型必须是武器实例，
	// 确保调用 CreateInstance 得到的 Instance 可以通过 Cast 安全访问 GetWeaponDefinition
	InstanceClass = UApecoxWeaponInstance::StaticClass();
}
