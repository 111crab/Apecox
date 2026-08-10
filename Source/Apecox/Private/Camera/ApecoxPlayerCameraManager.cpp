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
