// Copyright Apecox. All Rights Reserved.

#include "AbilitySystem/TargetData/ApecoxRangedShotTargetData.h"
#include "Net/Core/PushModel/PushModel.h"

bool FApecoxRangedShotTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// 先序列化父类（FGameplayAbilityTargetData_SingleTargetHit 的 HitResult 等）
	// 父类 FGameplayAbilityTargetData_SingleTargetHit 实现了自己的 NetSerialize，
	// 通过 Super 调用确保继承字段正确序列化
	bool bParentSuccess = true;
	Super::NetSerialize(Ar, Map, bParentSuccess);

	// 序列化新增字段
	Ar << ShotId;
	Ar << BurstId;
	Ar << BurstShotIndex;
	Ar << ClientFireTimeSeconds;

	// FVector_NetQuantize10 和 FVector_NetQuantizeNormal 已内置 NetSerialize，
	// 通过 FArchive operator<< 自动使用量化序列化
	Ar << ViewOrigin;
	Ar << AimDirection;
	uint8 AimingBit = bIsAiming ? 1 : 0;
	Ar.SerializeBits(&AimingBit, 1);
	if (Ar.IsLoading())
	{
		bIsAiming = AimingBit != 0;
	}

	// 合并父类成功标志——任一层失败则整体失败
	bOutSuccess = bParentSuccess && !Ar.IsError();

	return true;
}
