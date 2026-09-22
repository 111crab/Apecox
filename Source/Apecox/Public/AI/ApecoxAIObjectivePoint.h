// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "ApecoxAIObjectivePoint.generated.h"

/**
 * 地图作者放置的轻量争夺点。无战斗目标时，AI 在这些点之间移动并停留搜索。
 * Actor 本身不计分、不占领，也不复制；它只为服务器 AI 提供导航意图。
 */
UCLASS(Blueprintable, Placeable)
class APECOX_API AApecoxAIObjectivePoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	/** 被随机选择的相对权重。2 表示约为普通点的两倍概率。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|AI|Objective",
		meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	/** AI 到达此距离后停止移动并开始环视。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|AI|Objective",
		meta = (ClampMin = "25.0", Units = "cm"))
	float AcceptanceRadius = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|AI|Objective",
		meta = (ClampMin = "0.0", Units = "s"))
	float SearchDurationMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|AI|Objective",
		meta = (ClampMin = "0.0", Units = "s"))
	float SearchDurationMax = 2.0f;

	/** 关闭后保留在地图中，但不参与 AI 选点。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apecox|AI|Objective")
	bool bEnabled = true;
};
