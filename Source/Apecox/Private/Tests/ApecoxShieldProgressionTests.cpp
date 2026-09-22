// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Pickups/ApecoxCombatPickup.h"
#include "Player/ApecoxPlayerState.h"
#include "Tests/AutomationCommon.h"

namespace
{
	void InitializeHealth(AApecoxPlayerState* PlayerState, float Health = 100.0f)
	{
		UApecoxAbilitySystemComponent* ASC = PlayerState->GetApecoxAbilitySystemComponent();
		ASC->InitAbilityActorInfo(PlayerState, PlayerState);
		ASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetMaxHealthAttribute(), 100.0f);
		ASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetHealthAttribute(), Health);
	}

	void ApplyDamage(AApecoxPlayerState* Source, AApecoxPlayerState* Target, float Damage)
	{
		UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>();
		DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = DamageEffect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UApecoxVitalAttributeSet::GetHealthAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(-Damage);
		FGameplayEffectContextHandle Context = Source->GetApecoxAbilitySystemComponent()->MakeEffectContext();
		Context.AddInstigator(Source, Source);
		const FGameplayEffectSpec Spec(DamageEffect, Context, 1.0f);
		Target->GetApecoxAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(Spec);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxShieldProgressionTest,
	"Apecox.Combat.ShieldProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxShieldProgressionTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	AApecoxPlayerState* Player = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	InitializeHealth(Player);
	Player->InitializeCombatAttributesForPawn(true);
	TestEqual(TEXT("Human starts with white shield"), Player->GetShieldTier(), EApecoxShieldTier::White);
	TestEqual(TEXT("White shield maximum is 25"), Player->GetMaxShield(), 25.0f);
	TestEqual(TEXT("White shield starts full"), Player->GetShield(), 25.0f);
	TestEqual(TEXT("White requires 500 evolution points"),
		Player->GetShieldEvolutionPointsToNextTier(), 500.0f);

	TestEqual(TEXT("Damage progress is accumulated exactly"), Player->AddShieldEvolutionPoints(499.0f), 499.0f);
	TestEqual(TEXT("499 points remains white"), Player->GetShieldTier(), EApecoxShieldTier::White);
	Player->AddShieldEvolutionPoints(1.0f);
	TestEqual(TEXT("500 points upgrades to blue"), Player->GetShieldTier(), EApecoxShieldTier::Blue);
	TestEqual(TEXT("Blue shield maximum is 50"), Player->GetMaxShield(), 50.0f);
	TestEqual(TEXT("Upgrade adds the new 25-point segment"), Player->GetShield(), 50.0f);

	Player->GetApecoxAbilitySystemComponent()->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetShieldAttribute(), 7.0f);
	TestTrue(TEXT("Battery changes shield progression or fill"), Player->TryApplyShieldBattery(300.0f));
	TestEqual(TEXT("A 300-point battery does not skip the 1000-point blue stage"),
		Player->GetShieldTier(), EApecoxShieldTier::Blue);
	TestEqual(TEXT("Battery refills the current blue shield"), Player->GetShield(), 50.0f);
	TestEqual(TEXT("Blue still needs 700 points after the battery"),
		Player->GetShieldEvolutionPointsToNextTier(), 700.0f);
	Player->AddShieldEvolutionPoints(699.0f);
	TestEqual(TEXT("1499 points remains blue"), Player->GetShieldTier(), EApecoxShieldTier::Blue);
	Player->AddShieldEvolutionPoints(1.0f);
	TestEqual(TEXT("1500 cumulative points reaches purple"), Player->GetShieldTier(), EApecoxShieldTier::Purple);
	TestEqual(TEXT("Purple shield maximum is 75"), Player->GetMaxShield(), 75.0f);
	TestEqual(TEXT("Purple upgrade adds the final 25-point segment"), Player->GetShield(), 75.0f);
	TestEqual(TEXT("Purple is capped at 1500 points"), Player->GetShieldEvolutionPoints(), 1500.0f);
	TestEqual(TEXT("Purple has no next-tier requirement"), Player->GetShieldEvolutionPointsToNextTier(), 0.0f);
	TestEqual(TEXT("Purple progression rejects overflow"), Player->AddShieldEvolutionPoints(50.0f), 0.0f);

	Player->GetApecoxAbilitySystemComponent()->SetNumericAttributeBase(
		UApecoxVitalAttributeSet::GetHealthAttribute(), 32.0f);
	TestTrue(TEXT("Health pickup heals a wounded player"), Player->TryApplyHealthPickup(100.0f));
	TestEqual(TEXT("Health pickup clamps at 100"),
		Player->GetVitalAttributeSet()->GetHealth(), 100.0f);
	TestFalse(TEXT("Full-health player does not consume another pack"), Player->TryApplyHealthPickup(100.0f));

	AApecoxPlayerState* Bot = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	InitializeHealth(Bot);
	Bot->SetCombatTeam(EApecoxCombatTeam::AI);
	Bot->InitializeCombatAttributesForPawn(false);
	TestEqual(TEXT("AI has no shield"), Bot->GetMaxShield(), 0.0f);
	TestEqual(TEXT("AI gains no evolution"), Bot->AddShieldEvolutionPoints(300.0f), 0.0f);
	TestFalse(TEXT("AI cannot consume shield battery"), Bot->TryApplyShieldBattery(300.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxShieldDamageRoutingTest,
	"Apecox.Combat.ShieldDamageRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxShieldDamageRoutingTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	AApecoxPlayerState* HumanTarget = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	AApecoxPlayerState* BotSource = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	InitializeHealth(HumanTarget);
	InitializeHealth(BotSource);
	HumanTarget->InitializeCombatAttributesForPawn(true);
	BotSource->SetCombatTeam(EApecoxCombatTeam::AI);
	BotSource->InitializeCombatAttributesForPawn(false);

	ApplyDamage(BotSource, HumanTarget, 13.0f);
	TestEqual(TEXT("First 13 damage is absorbed by shield"), HumanTarget->GetShield(), 12.0f);
	TestEqual(TEXT("Shield-only hit preserves health"), HumanTarget->GetVitalAttributeSet()->GetHealth(), 100.0f);
	ApplyDamage(BotSource, HumanTarget, 13.0f);
	TestEqual(TEXT("Second hit exhausts shield"), HumanTarget->GetShield(), 0.0f);
	TestEqual(TEXT("One overflow damage reaches health"), HumanTarget->GetVitalAttributeSet()->GetHealth(), 99.0f);

	AApecoxPlayerState* HumanSource = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	AApecoxPlayerState* BotTarget = TestWorld.GetTestWorld()->SpawnActor<AApecoxPlayerState>();
	InitializeHealth(HumanSource);
	InitializeHealth(BotTarget);
	HumanSource->InitializeCombatAttributesForPawn(true);
	BotTarget->SetCombatTeam(EApecoxCombatTeam::AI);
	BotTarget->InitializeCombatAttributesForPawn(false);
	ApplyDamage(HumanSource, BotTarget, 13.0f);
	TestEqual(TEXT("AI takes base rifle damage directly to health"),
		BotTarget->GetVitalAttributeSet()->GetHealth(), 87.0f);
	TestEqual(TEXT("Human receives one evolution point per actual enemy damage"),
		HumanSource->GetShieldEvolutionPoints(), 13.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxCombatPickupDefaultsTest,
	"Apecox.Combat.PickupDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxCombatPickupDefaultsTest::RunTest(const FString& Parameters)
{
	const AApecoxCombatPickup* Defaults = GetDefault<AApecoxCombatPickup>();
	TestTrue(TEXT("Combat pickup is replicated"), Defaults->GetIsReplicated());
	TestTrue(TEXT("Combat pickup starts available"), Defaults->IsPickupAvailable());
	return true;
}

#endif
