// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ApecoxRangedShotTargetData.generated.h"

/**
 * FApecoxRangedShotTargetData
 * - 一次实体子弹射击的客户端→服务器意图。
 * - 继承 FGameplayAbilityTargetData_SingleTargetHit 以兼容 GAS TargetData 框架，
 *   但继承的 HitResult 只作为客户端候选/诊断输入——不直接用于 Authority 伤害。
 *
 * 为什么继承 SingleTargetHit 而非完全自定义：
 * - GAS TargetData 基础设施（TargetDataSetDelegate、ReplicatedTargetData、PredictionKey）
 *   不需要完全自定义脚本结构即可工作；
 * - 新增字段（ShotId、ViewOrigin、AimDirection）是服务端验证的额外输入；
 * - 父类 HitResult 保留客户端候选命中，供诊断和未来 SSR 参考。
 *
 * 与 Lyra 的区别：
 * - Lyra 的 TargetData 将 bIsTargetDataValid 恒设为 true，不验证客户端 HitResult；
 * - Apecox仅使用合法瞄准意图，由服务端射线或实体子弹碰撞产生真实结果。
 */
USTRUCT()
struct APECOX_API FApecoxRangedShotTargetData : public FGameplayAbilityTargetData_SingleTargetHit
{
	GENERATED_BODY()

	FApecoxRangedShotTargetData() = default;

	// --- 继承自 FGameplayAbilityTargetData_SingleTargetHit ---
	// FHitResult HitResult — 客户端候选命中（不直接应用伤害）
	// bool bHitResultValid — 父类有效性标志

	// --- 新增网络字段 ---

	/** 一次射击的幂等标识——服务器防重复/倒序结算 */
	UPROPERTY()
	uint32 ShotId = 0;

	/** 每次松开开火后递增；让Authority在下一发识别新的连发串。 */
	UPROPERTY()
	uint32 BurstId = 1;

	/** 本发在当前连发串中的零基索引；第一发为0。 */
	UPROPERTY()
	int32 BurstShotIndex = 0;

	/** 客户端开枪时的世界时间——为诊断和未来 SSR 保留；V1 不据此回滚世界 */
	UPROPERTY()
	float ClientFireTimeSeconds = 0.0f;

	/** 客户端开枪时的视点——服务端用于合理性检查（与权威视点比较） */
	UPROPERTY()
	FVector_NetQuantize10 ViewOrigin = FVector::ZeroVector;

	/** 客户端准星方向——服务端与权威 BaseAimRotation 比较后使用 */
	UPROPERTY()
	FVector_NetQuantizeNormal AimDirection = FVector::ForwardVector;

	/** 扣扳机瞬间是否处于合法 ADS；Authority据此选择瞄准散布分支。 */
	UPROPERTY()
	bool bIsAiming = false;

	// ========================================================================
	//  FGameplayAbilityTargetData 接口
	// ========================================================================

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return StaticStruct();
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};

/** NetSerialize trait——使 GAS 网络层能正确序列化自定义字段 */
template<>
struct TStructOpsTypeTraits<FApecoxRangedShotTargetData> : public TStructOpsTypeTraitsBase2<FApecoxRangedShotTargetData>
{
	enum
	{
		WithNetSerializer = true,
	};
};
