// Copyright Apecox. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ApecoxHUD.generated.h"

enum class EApecoxShotConfirmation : uint8;

/**
 * AApecoxHUD
 * - Apecox 项目级 HUD 入口——提供十字准星、当前步枪弹药、人机生命与比赛状态显示。
 * - 使用 AHUD + UCanvas 绘制，不启动 UMG、Widget Blueprint 或纹理资源。
 * - 仅本地表现：不复制、不加入 GAS、不创建 GameplayTag。
 *
 * 为什么准星归属 HUD 而非 Character 或 Weapon：
 * - HUD 是每个本地玩家的画布入口，天然适合屏幕中心 UI。
 * - Character 和 Weapon 负责 3D 世界表现；准星是 2D 屏幕覆盖层。
 * - 未来 UMG 根界面也可以由本类创建或承载。
 */
UCLASS()
class APECOX_API AApecoxHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

protected:
	/** 准星颜色——默认白色 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;

	/** 每条线段长度（像素） */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairLineLength = 8.0f;

	/** 线宽（像素） */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairLineThickness = 2.0f;

	/** 中心点与四条线段之间的间隔（像素） */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairGap = 4.0f;

	/** 达到当前移动速度上限时，在基础间隔之外增加的像素数。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairMovementExpansion = 10.0f;

	/** 角色腾空时额外增加的像素数；与水平移动扩散叠加。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairAirborneExpansion = 14.0f;

	/** 准星张开和回收的插值速度。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CrosshairExpansionInterpSpeed = 12.0f;

	/** 腰射准星中心常驻实心点；ADS 使用瞄具自身红点。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Crosshair", meta = (ClampMin = "0.0"))
	float CrosshairCenterDotRadius = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Hit Marker")
	FLinearColor HitMarkerColor = FLinearColor(1.0f, 0.04f, 0.02f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Hit Marker", meta = (ClampMin = "0.01"))
	float HitMarkerDuration = 0.14f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Hit Marker", meta = (ClampMin = "0.0"))
	float HitMarkerInnerGap = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Hit Marker", meta = (ClampMin = "0.0"))
	float HitMarkerLineLength = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Hit Marker", meta = (ClampMin = "0.1"))
	float HitMarkerLineThickness = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Ammo")
	FLinearColor AmmoTextColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Ammo")
	FLinearColor EmptyAmmoTextColor = FLinearColor(1.0f, 0.15f, 0.05f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Ammo", meta = (ClampMin = "0.1"))
	float AmmoTextScale = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Ammo", meta = (ClampMin = "0.0"))
	FVector2D AmmoScreenMargin = FVector2D(48.0f, 48.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Bot Health")
	bool bShowBotHealthBars = true;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Bot Health", meta = (ClampMin = "0.0"))
	float BotHealthBarMaxDistance = 6000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Bot Health", meta = (ClampMin = "1.0"))
	FVector2D BotHealthBarSize = FVector2D(110.0f, 10.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Bot Health", meta = (ClampMin = "0.1"))
	float BotHealthTextScale = 0.75f;

	/** 本地玩家实际伤到某个 AI 后，该目标血条继续显示的时间。再次命中会刷新。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Bot Health", meta = (ClampMin = "0.1", Units = "s"))
	float BotHealthRevealDuration = 4.0f;

	/** 比赛进行中常驻显示的队伍比分。 */
	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match")
	FLinearColor MatchScoreTextColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match")
	FLinearColor VictoryTextColor = FLinearColor(0.08f, 1.0f, 0.2f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match")
	FLinearColor DefeatTextColor = FLinearColor(1.0f, 0.08f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match", meta = (ClampMin = "0.1"))
	float MatchScoreTextScale = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match", meta = (ClampMin = "0.1"))
	float MatchResultTextScale = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apecox|HUD|Match", meta = (ClampMin = "0.0"))
	float MatchScoreTopMargin = 28.0f;

private:
	void DrawMatchStatus();
	void DrawPlayerStatus();
	void DrawWeaponStatus(const class AApecoxPlayerCharacter* Character);
	void DrawBotHealthBars();
	void DrawHitMarker(const FVector2D& ScreenCenter);

	UFUNCTION()
	void HandleShotResult(uint32 ShotId, EApecoxShotConfirmation Result);

	UFUNCTION()
	void HandleProjectileResolved(uint32 ShotId, EApecoxShotConfirmation Result,
		AActor* DamagedActor);

	float SmoothedCrosshairGap = 4.0f;
	float HitMarkerTimeRemaining = 0.0f;
	TMap<TWeakObjectPtr<class AApecoxBotCharacter>, float> RevealedBotHealthUntil;
};
