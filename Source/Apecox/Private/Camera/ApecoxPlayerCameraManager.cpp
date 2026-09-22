// Copyright Apecox. All Rights Reserved.

#include "Camera/ApecoxPlayerCameraManager.h"

AApecoxPlayerCameraManager::AApecoxPlayerCameraManager()
{
	// 限制上下观察角度，避免完整 Manny 第一人称摄像机进入头部或胸腔内部造成穿模。
	// 这两个值与 UE 5.8 官方 BP_FirstPersonCameraManager 一致，
	// 直接使用父类 APlayerCameraManager 已有属性，不重复声明成员变量。
	ViewPitchMin = -70.0f;
	ViewPitchMax = 80.0f;
}

void AApecoxPlayerCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
	// 先保留引擎正常的 CameraComponent / CalcCamera 计算，
	// 再覆盖每视图的透视近裁剪距离。
	Super::UpdateViewTargetInternal(OutVT, DeltaTime);

	// UE 5.8 提供每视图近裁剪面（PerspectiveNearClipPlane）。
	// 这里只调整本地第一人称视图，避免修改全局 NearClipPlane 影响其他视图的深度精度。
	OutVT.POV.PerspectiveNearClipPlane = FirstPersonNearClipPlane;
}
