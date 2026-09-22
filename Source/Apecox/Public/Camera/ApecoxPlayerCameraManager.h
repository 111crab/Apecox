// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ApecoxPlayerCameraManager.generated.h"

/**
 * AApecoxPlayerCameraManager
 * - Apecox 项目级 PlayerCameraManager，管理本地玩家镜头的公共规则。
 * - 当前只负责第一人称上下观察角度约束，避免完整 Manny 身体模型中
 *   摄像机低头进入头部或胸腔内部造成穿模。
 * - 后续 ADS FOV、后坐力镜头反馈、死亡/观战镜头可在独立阶段扩展，
 *   本轮不预留空 API。
 * - PlayerCameraManager 不承载 Character 移动、GAS、武器或动画逻辑，
 *   也不需要网络复制。
 */
UCLASS()
class APECOX_API AApecoxPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AApecoxPlayerCameraManager();

protected:
	/**
	 * 覆写相机视图更新：在父类完成 CameraComponent / CalcCamera 计算后，
	 * 应用项目级第一人称近裁剪距离。
	 */
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

private:
	/** 本地第一人称视图使用的透视近裁剪距离，单位为厘米。不要降低为 RAR 原工程的 0.0001。 */
	UPROPERTY(EditDefaultsOnly, Category = "Camera|First Person", meta = (ClampMin = "1.0", UIMin = "1.0", Units = "cm"))
	float FirstPersonNearClipPlane = 1.0f;
};
