// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ApecoxPlayerController.generated.h"

/**
 * AApecoxPlayerController
 * - Apecox 项目级 PlayerController
 * - 每帧在 PostProcessInput 中统一调度 ASC 输入处理
 * - 不绑定具体 IA、不保存 AbilitySet、不实现技能逻辑
 *
 * 为什么选择 PostProcessInput：
 * Enhanced Input 子系统的所有 Action 回调已在本帧内完成——此时 ASC 可以看到完整输入快照。
 * 若在每个 IA 回调里立即激活，会让同帧组合键、按下/松开和多 Spec 的处理顺序分散。
 */
UCLASS()
class APECOX_API AApecoxPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
};
