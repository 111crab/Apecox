// Copyright Apecox. All Rights Reserved.

#include "UI/ApecoxHUD.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Character/ApecoxBotCharacter.h"
#include "Character/ApecoxHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Game/ApecoxGameState.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Player/ApecoxPlayerController.h"
#include "Weapons/ApecoxWeaponStateComponent.h"
#include "Player/ApecoxPlayerState.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"

namespace
{
	void DrawPanel(UCanvas* Canvas, const FVector2D& Position, const FVector2D& Size,
		const FLinearColor& Color = FLinearColor(0.005f, 0.008f, 0.015f, 0.78f))
	{
		FCanvasTileItem Panel(Position, Size, Color);
		Panel.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Panel);
	}

	void DrawBar(UCanvas* Canvas, const FVector2D& Position, const FVector2D& Size,
		float Alpha, const FLinearColor& FillColor)
	{
		FCanvasTileItem Background(Position, Size, FLinearColor(0.04f, 0.045f, 0.06f, 0.95f));
		Background.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Background);
		const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
		if (ClampedAlpha > 0.0f)
		{
			FCanvasTileItem Fill(Position, FVector2D(Size.X * ClampedAlpha, Size.Y), FillColor);
			Fill.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Fill);
		}
	}

	FLinearColor GetShieldTierColor(EApecoxShieldTier Tier)
	{
		switch (Tier)
		{
		case EApecoxShieldTier::Blue: return FLinearColor(0.08f, 0.42f, 1.0f, 1.0f);
		case EApecoxShieldTier::Purple: return FLinearColor(0.62f, 0.12f, 1.0f, 1.0f);
		case EApecoxShieldTier::White:
		default: return FLinearColor(0.86f, 0.9f, 1.0f, 1.0f);
		}
	}

	const TCHAR* GetShieldTierName(EApecoxShieldTier Tier)
	{
		switch (Tier)
		{
		case EApecoxShieldTier::Blue: return TEXT("BLUE");
		case EApecoxShieldTier::Purple: return TEXT("PURPLE");
		case EApecoxShieldTier::White:
		default: return TEXT("WHITE");
		}
	}
}

void AApecoxHUD::BeginPlay()
{
	Super::BeginPlay();
	if (AApecoxPlayerController* ApecoxPC = Cast<AApecoxPlayerController>(PlayerOwner))
	{
		if (UApecoxWeaponStateComponent* WeaponState = ApecoxPC->GetWeaponStateComponent())
		{
			WeaponState->OnProjectileResolved.AddUniqueDynamic(this, &AApecoxHUD::HandleProjectileResolved);
			WeaponState->OnShotConfirmed.AddUniqueDynamic(this, &AApecoxHUD::HandleShotResult);
		}
	}
}

void AApecoxHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AApecoxPlayerController* ApecoxPC = Cast<AApecoxPlayerController>(PlayerOwner))
	{
		if (UApecoxWeaponStateComponent* WeaponState = ApecoxPC->GetWeaponStateComponent())
		{
			WeaponState->OnProjectileResolved.RemoveAll(this);
			WeaponState->OnShotConfirmed.RemoveAll(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AApecoxHUD::HandleShotResult(uint32 ShotId, EApecoxShotConfirmation Result)
{
	if (Result == EApecoxShotConfirmation::ConfirmedHit)
	{
		HitMarkerTimeRemaining = HitMarkerDuration;
	}
}

void AApecoxHUD::HandleProjectileResolved(
	uint32 ShotId, EApecoxShotConfirmation Result, AActor* DamagedActor)
{
	HandleShotResult(ShotId, Result);
	if (Result != EApecoxShotConfirmation::ConfirmedHit || !GetWorld())
	{
		return;
	}

	if (AApecoxBotCharacter* DamagedBot = Cast<AApecoxBotCharacter>(DamagedActor))
	{
		RevealedBotHealthUntil.FindOrAdd(DamagedBot) =
			GetWorld()->GetTimeSeconds() + BotHealthRevealDuration;
	}
}

void AApecoxHUD::DrawHUD()
{
	Super::DrawHUD();

	// 比赛结果属于 Controller/GameState，不依赖 Pawn。玩家死亡或 PostMatch 不再重生时也必须可见。
	if (!Canvas)
	{
		return;
	}

	if (!PlayerOwner)
	{
		return;
	}

	DrawMatchStatus();
	DrawPlayerStatus();

	APawn* Pawn = PlayerOwner->GetPawn();
	if (!Pawn)
	{
		return;
	}

	const AApecoxPlayerCharacter* Character = Cast<AApecoxPlayerCharacter>(Pawn);
	DrawWeaponStatus(Character);

	DrawBotHealthBars();

	const FVector2D ScreenCenter(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	HitMarkerTimeRemaining = FMath::Max(0.0f,
		HitMarkerTimeRemaining - (GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f));
	const bool bShowingHitMarker = HitMarkerTimeRemaining > 0.0f;
	if (bShowingHitMarker)
	{
		// Draw before the ADS early return: authoritative hit feedback is visible through the optic too.
		DrawHitMarker(ScreenCenter);
	}

	// 铁瞄已经提供屏幕中心参照；ADS 只隐藏腰射十字准星，弹药仍保留。
	if (Character && Character->GetAimAlpha() >= 0.5f)
	{
		return;
	}

	// 计算屏幕中心
	const float CenterX = Canvas->ClipX * 0.5f;
	const float CenterY = Canvas->ClipY * 0.5f;
	const FLinearColor Color = CrosshairColor;
	const float Thickness = CrosshairLineThickness;
	const float Length = CrosshairLineLength;
	float MovementAlpha = 0.0f;
	bool bIsAirborne = false;
	if (Character)
	{
		const float HorizontalSpeed = Character->GetVelocity().Size2D();
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const float CurrentMaxSpeed = Movement ? FMath::Max(Movement->GetMaxSpeed(), 1.0f) : 1.0f;
		MovementAlpha = FMath::Clamp(HorizontalSpeed / CurrentMaxSpeed, 0.0f, 1.0f);
		bIsAirborne = Movement && Movement->IsFalling();
	}

	const float TargetGap = CrosshairGap
		+ CrosshairMovementExpansion * MovementAlpha
		+ (bIsAirborne ? CrosshairAirborneExpansion : 0.0f);
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	SmoothedCrosshairGap = FMath::FInterpTo(
		SmoothedCrosshairGap,
		TargetGap,
		DeltaSeconds,
		CrosshairExpansionInterpSpeed);
	const float Gap = SmoothedCrosshairGap;

	if (CrosshairCenterDotRadius > 0.0f)
	{
		FCanvasTileItem Dot(
			FVector2D(CenterX - CrosshairCenterDotRadius, CenterY - CrosshairCenterDotRadius),
			FVector2D(CrosshairCenterDotRadius * 2.0f), Color);
		Dot.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Dot);
	}

	// 上段——从中心上方 Gap 向上延伸 Length
	{
		const FVector2D Start(CenterX, CenterY - Gap);
		const FVector2D End(CenterX, CenterY - Gap - Length);
		FCanvasLineItem LineItem(Start, End);
		LineItem.SetColor(Color.ToFColor(true));
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}

	// 下段——从中心下方 Gap 向下延伸 Length
	{
		const FVector2D Start(CenterX, CenterY + Gap);
		const FVector2D End(CenterX, CenterY + Gap + Length);
		FCanvasLineItem LineItem(Start, End);
		LineItem.SetColor(Color.ToFColor(true));
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}

	// 左段——从中心左侧 Gap 向左延伸 Length
	{
		const FVector2D Start(CenterX - Gap, CenterY);
		const FVector2D End(CenterX - Gap - Length, CenterY);
		FCanvasLineItem LineItem(Start, End);
		LineItem.SetColor(Color.ToFColor(true));
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}

	// 右段——从中心右侧 Gap 向右延伸 Length
	{
		const FVector2D Start(CenterX + Gap, CenterY);
		const FVector2D End(CenterX + Gap + Length, CenterY);
		FCanvasLineItem LineItem(Start, End);
		LineItem.SetColor(Color.ToFColor(true));
		LineItem.LineThickness = Thickness;
		Canvas->DrawItem(LineItem);
	}
}

void AApecoxHUD::DrawPlayerStatus()
{
	if (!Canvas || !PlayerOwner || !GEngine)
	{
		return;
	}

	const AApecoxPlayerState* PlayerState = PlayerOwner->GetPlayerState<AApecoxPlayerState>();
	const UApecoxVitalAttributeSet* Vital = PlayerState ? PlayerState->GetVitalAttributeSet() : nullptr;
	UFont* Font = GEngine->GetMediumFont();
	if (!PlayerState || !Vital || !Font)
	{
		return;
	}

	const float UIScale = FMath::Clamp(Canvas->ClipY / 1080.0f, 0.78f, 1.35f);
	const FVector2D PanelSize(400.0f * UIScale, 166.0f * UIScale);
	const FVector2D PanelOrigin(36.0f * UIScale, Canvas->ClipY - PanelSize.Y - 36.0f * UIScale);
	DrawPanel(Canvas, PanelOrigin, PanelSize);

	const float Health = Vital->GetHealth();
	const float MaxHealth = FMath::Max(Vital->GetMaxHealth(), 1.0f);
	const float Shield = Vital->GetShield();
	const float MaxShield = FMath::Max(Vital->GetMaxShield(), 1.0f);
	const EApecoxShieldTier ShieldTier = PlayerState->GetShieldTier();
	const FLinearColor ShieldColor = GetShieldTierColor(ShieldTier);
	const float TextScale = 1.05f * UIScale;

	FCanvasTextItem HealthText(
		PanelOrigin + FVector2D(18.0f, 12.0f) * UIScale,
		FText::FromString(FString::Printf(TEXT("HEALTH   %03d / %03d"),
			FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth))),
		Font, FLinearColor::White);
	HealthText.Scale = FVector2D(TextScale);
	HealthText.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(HealthText);
	DrawBar(Canvas, PanelOrigin + FVector2D(18.0f, 47.0f) * UIScale,
		FVector2D(364.0f, 13.0f) * UIScale, Health / MaxHealth,
		FLinearColor(0.04f, 0.85f, 0.25f, 1.0f));

	FCanvasTextItem ShieldText(
		PanelOrigin + FVector2D(18.0f, 68.0f) * UIScale,
		FText::FromString(FString::Printf(TEXT("SHIELD [%s]   %02d / %02d"),
			GetShieldTierName(ShieldTier), FMath::CeilToInt(Shield), FMath::CeilToInt(Vital->GetMaxShield()))),
		Font, ShieldColor);
	ShieldText.Scale = FVector2D(TextScale);
	ShieldText.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(ShieldText);
	DrawBar(Canvas, PanelOrigin + FVector2D(18.0f, 103.0f) * UIScale,
		FVector2D(364.0f, 13.0f) * UIScale, Shield / MaxShield, ShieldColor);

	const FString EvolutionString = ShieldTier == EApecoxShieldTier::Purple
		? TEXT("EVO   MAX LEVEL")
		: FString::Printf(TEXT("EVO   %03d TO %s"),
			FMath::CeilToInt(PlayerState->GetShieldEvolutionPointsToNextTier()),
			ShieldTier == EApecoxShieldTier::White ? TEXT("BLUE") : TEXT("PURPLE"));
	FCanvasTextItem EvolutionText(
		PanelOrigin + FVector2D(18.0f, 127.0f) * UIScale,
		FText::FromString(EvolutionString), Font, FLinearColor(0.82f, 0.86f, 0.92f, 1.0f));
	EvolutionText.Scale = FVector2D(0.9f * UIScale);
	EvolutionText.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(EvolutionText);
}

void AApecoxHUD::DrawWeaponStatus(const AApecoxPlayerCharacter* Character)
{
	if (!Canvas || !Character || !GEngine)
	{
		return;
	}
	const UApecoxEquipmentComponent* Equipment = Character->GetEquipmentComponent();
	const UApecoxRangedWeaponInstance* Weapon = Equipment
		? Cast<UApecoxRangedWeaponInstance>(Equipment->GetCurrentWeaponInstance()) : nullptr;
	UFont* MediumFont = GEngine->GetMediumFont();
	UFont* LargeFont = GEngine->GetLargeFont();
	if (!Weapon || !MediumFont || !LargeFont)
	{
		return;
	}

	const float UIScale = FMath::Clamp(Canvas->ClipY / 1080.0f, 0.78f, 1.35f);
	const FVector2D PanelSize(310.0f * UIScale, 112.0f * UIScale);
	const FVector2D PanelOrigin(
		Canvas->ClipX - PanelSize.X - 36.0f * UIScale,
		Canvas->ClipY - PanelSize.Y - 36.0f * UIScale);
	DrawPanel(Canvas, PanelOrigin, PanelSize);

	FString WeaponName(TEXT("ASSAULT RIFLE"));
	if (const UApecoxWeaponDefinition* Definition = Weapon->GetRangedWeaponDefinition())
	{
		if (!Definition->DisplayName.IsEmpty())
		{
			WeaponName = Definition->DisplayName.ToString().ToUpper();
		}
	}
	FCanvasTextItem NameItem(PanelOrigin + FVector2D(18.0f, 11.0f) * UIScale,
		FText::FromString(WeaponName), MediumFont, FLinearColor(0.72f, 0.78f, 0.86f, 1.0f));
	NameItem.Scale = FVector2D(0.9f * UIScale);
	NameItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(NameItem);

	const FString AmmoString = FString::Printf(TEXT("%02d  /  %03d"),
		Weapon->GetCurrentMagazineAmmo(), Weapon->GetReserveAmmo());
	FCanvasTextItem AmmoItem(PanelOrigin + FVector2D(18.0f, 42.0f) * UIScale,
		FText::FromString(AmmoString), LargeFont,
		Weapon->IsMagazineEmpty() ? EmptyAmmoTextColor : AmmoTextColor);
	AmmoItem.Scale = FVector2D(1.12f * UIScale);
	AmmoItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(AmmoItem);
}

void AApecoxHUD::DrawMatchStatus()
{
	if (!Canvas || !GetWorld() || !GEngine)
	{
		return;
	}

	const AApecoxGameState* ApecoxGameState = GetWorld()->GetGameState<AApecoxGameState>();
	if (!ApecoxGameState)
	{
		return;
	}

	UFont* ScoreFont = GEngine->GetMediumFont();
	if (!ScoreFont)
	{
		return;
	}

	const FString ScoreString = FString::Printf(
		TEXT("PLAYERS  %d / %d     AI  %d / %d"),
		ApecoxGameState->GetPlayerTeamScore(),
		ApecoxGameState->GetTargetScore(),
		ApecoxGameState->GetAITeamScore(),
		ApecoxGameState->GetTargetScore());
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(ScoreFont, ScoreString, TextWidth, TextHeight);
	const FVector2D ScorePanelSize(TextWidth * MatchScoreTextScale + 44.0f,
		TextHeight * MatchScoreTextScale + 22.0f);
	DrawPanel(Canvas, FVector2D(Canvas->ClipX * 0.5f - ScorePanelSize.X * 0.5f,
		MatchScoreTopMargin - 8.0f), ScorePanelSize);
	FCanvasTextItem ScoreItem(
		FVector2D(Canvas->ClipX * 0.5f - TextWidth * MatchScoreTextScale * 0.5f,
			MatchScoreTopMargin),
		FText::FromString(ScoreString),
		ScoreFont,
		MatchScoreTextColor);
	ScoreItem.Scale = FVector2D(MatchScoreTextScale);
	ScoreItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(ScoreItem);

	if (ApecoxGameState->GetApecoxMatchPhase() != EApecoxMatchPhase::PostMatch)
	{
		return;
	}

	const bool bPlayersWon = ApecoxGameState->GetMatchWinner() == EApecoxMatchWinner::Players;
	const FString ResultString = bPlayersWon ? TEXT("VICTORY") : TEXT("DEFEAT");
	UFont* ResultFont = GEngine->GetLargeFont();
	if (!ResultFont)
	{
		return;
	}

	float ResultWidth = 0.0f;
	float ResultHeight = 0.0f;
	Canvas->StrLen(ResultFont, ResultString, ResultWidth, ResultHeight);
	FCanvasTextItem ResultItem(
		FVector2D(Canvas->ClipX * 0.5f - ResultWidth * MatchResultTextScale * 0.5f,
			Canvas->ClipY * 0.34f),
		FText::FromString(ResultString),
		ResultFont,
		bPlayersWon ? VictoryTextColor : DefeatTextColor);
	ResultItem.Scale = FVector2D(MatchResultTextScale);
	ResultItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(ResultItem);

	const FString HintString(TEXT("SCORE LIMIT REACHED - RESTART PIE TO PLAY AGAIN"));
	float HintWidth = 0.0f;
	float HintHeight = 0.0f;
	Canvas->StrLen(ScoreFont, HintString, HintWidth, HintHeight);
	FCanvasTextItem HintItem(
		FVector2D(Canvas->ClipX * 0.5f - HintWidth * 0.5f,
			Canvas->ClipY * 0.34f + ResultHeight * MatchResultTextScale + 14.0f),
		FText::FromString(HintString),
		ScoreFont,
		FLinearColor::White);
	HintItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(HintItem);
}

void AApecoxHUD::DrawHitMarker(const FVector2D& ScreenCenter)
{
	if (!Canvas)
	{
		return;
	}

	const float Inner = HitMarkerInnerGap;
	const float Outer = Inner + HitMarkerLineLength;
	const FColor Color = HitMarkerColor.ToFColor(true);
	const FVector2D Directions[] =
	{
		FVector2D(-1.0f, -1.0f), FVector2D(1.0f, -1.0f),
		FVector2D(-1.0f, 1.0f), FVector2D(1.0f, 1.0f)
	};

	for (const FVector2D& Direction : Directions)
	{
		FCanvasLineItem Line(ScreenCenter + Direction * Inner, ScreenCenter + Direction * Outer);
		Line.SetColor(Color);
		Line.LineThickness = HitMarkerLineThickness;
		Canvas->DrawItem(Line);
	}
}

void AApecoxHUD::DrawBotHealthBars()
{
	if (!bShowBotHealthBars || !Canvas || !PlayerOwner || !GetWorld())
	{
		return;
	}

	const APawn* ViewerPawn = PlayerOwner->GetPawn();
	const FVector ViewerLocation = ViewerPawn
		? ViewerPawn->GetPawnViewLocation() : PlayerOwner->GetFocalLocation();
	UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
	const float WorldTime = GetWorld()->GetTimeSeconds();
	for (auto It = RevealedBotHealthUntil.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || It.Value() <= WorldTime)
		{
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<AApecoxBotCharacter> It(GetWorld()); It; ++It)
	{
		AApecoxBotCharacter* Bot = *It;
		const UApecoxHealthComponent* Health = Bot ? Bot->GetHealthComponent() : nullptr;
		const float* RevealUntil = RevealedBotHealthUntil.Find(Bot);
		if (!RevealUntil || *RevealUntil <= WorldTime || !IsValid(Bot) || Bot->IsHidden()
			|| !Health || Health->IsDeadOrDying()
			|| FVector::DistSquared(ViewerLocation, Bot->GetActorLocation())
				> FMath::Square(BotHealthBarMaxDistance))
		{
			continue;
		}

		const UCapsuleComponent* Capsule = Bot->GetCapsuleComponent();
		const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.0f;
		FVector2D ScreenPosition;
		if (!PlayerOwner->ProjectWorldLocationToScreen(
			Bot->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight + 28.0f), ScreenPosition))
		{
			continue;
		}

		const float HealthAlpha = Health->GetHealthNormalized();
		const FVector2D BarOrigin = ScreenPosition - FVector2D(BotHealthBarSize.X * 0.5f, 0.0f);
		FCanvasTileItem Border(BarOrigin - FVector2D(2.0f),
			BotHealthBarSize + FVector2D(4.0f), FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
		Border.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Border);

		FCanvasTileItem EmptyBar(BarOrigin, BotHealthBarSize,
			FLinearColor(0.16f, 0.02f, 0.02f, 0.9f));
		EmptyBar.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(EmptyBar);

		if (HealthAlpha > 0.0f)
		{
			const FLinearColor FillColor = FLinearColor::LerpUsingHSV(
				FLinearColor(0.9f, 0.05f, 0.02f), FLinearColor(0.05f, 0.9f, 0.1f), HealthAlpha);
			FCanvasTileItem FillBar(BarOrigin,
				FVector2D(BotHealthBarSize.X * HealthAlpha, BotHealthBarSize.Y), FillColor);
			FillBar.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(FillBar);
		}

		if (Font)
		{
			const FString HealthString = FString::Printf(TEXT("%d / %d"),
				FMath::CeilToInt(Health->GetHealth()), FMath::CeilToInt(Health->GetMaxHealth()));
			float TextWidth = 0.0f;
			float TextHeight = 0.0f;
			Canvas->StrLen(Font, HealthString, TextWidth, TextHeight);
			const FVector2D TextPosition(
				ScreenPosition.X - TextWidth * BotHealthTextScale * 0.5f,
				BarOrigin.Y - TextHeight * BotHealthTextScale - 3.0f);
			FCanvasTextItem TextItem(TextPosition, FText::FromString(HealthString), Font, FLinearColor::White);
			TextItem.Scale = FVector2D(BotHealthTextScale);
			TextItem.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(TextItem);
		}
	}
}
