// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ApecoxGameMode.generated.h"

class APlayerController;

/**
 * AApecoxGameMode
 * - 建立 Apecox 项目自有 Gameplay Framework 类型边界
 * - 在构造函数中指定 GameState、PlayerController、PlayerState 和默认 Pawn 类型
 * - 管理死亡后的重生调度：每名 Controller 拥有独立的延迟 Timer，
 *   通过 PendingRespawnControllers 集合仅对同一 Controller 防重入
 */
UCLASS()
class APECOX_API AApecoxGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AApecoxGameMode();

	/**
	 * 请求在 RespawnDelaySeconds 后为该 Controller 重生 Pawn
	 * - 仅服务器处理
	 * - PendingRespawnControllers 集合阻止同一 Controller 重复排队
	 * - 每个 Controller 创建独立 Timer，不共享全局到期时刻
	 */
	void RequestPlayerRespawn(APlayerController* Controller);

protected:
	/** 单个 Controller 的独立延迟重生回调 */
	void RestartPlayerAfterDelay(TWeakObjectPtr<AController> Controller);

	/** 死亡后到重生之间的延迟（秒），可在 GameMode Blueprint Defaults 中修改 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|Respawn")
	float RespawnDelaySeconds = 3.0f;

private:
	/** 等待重生的 Controller 集合，仅负责同一 Controller 防重入；
	 *  使用 TWeakObjectPtr 防止阻止被销毁 Controller 的垃圾回收 */
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<AController>> PendingRespawnControllers;
};
