// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ApecoxAIObjectivePoint.h"
#include "AI/ApecoxWanderAIController.h"
#include "Animation/ApecoxCharacterAnimInstance.h"
#include "Character/ApecoxBotCharacter.h"
#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Game/ApecoxGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/AutomationTest.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"
#include "Weapons/ApecoxWeaponDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxScoreAttackStateTest,
	"Apecox.ScoreAttack.MatchState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxScoreAttackStateTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}

	AApecoxGameState* State = TestWorld.GetTestWorld()->SpawnActor<AApecoxGameState>();
	if (!TestNotNull(TEXT("Score Attack GameState spawns"), State))
	{
		return false;
	}

	State->StartScoreAttack(3);
	TestEqual(TEXT("Match enters InProgress"), State->GetApecoxMatchPhase(), EApecoxMatchPhase::InProgress);
	TestEqual(TEXT("Target score is authoritative configuration"), State->GetTargetScore(), 3);
	TestEqual(TEXT("Player score starts at zero"), State->GetPlayerTeamScore(), 0);
	TestEqual(TEXT("AI score starts at zero"), State->GetAITeamScore(), 0);

	TestEqual(TEXT("Player score increments"), State->AddPlayerTeamScore(), 1);
	TestEqual(TEXT("AI score increments"), State->AddAITeamScore(), 1);
	TestEqual(TEXT("Scores never become negative"), State->AddAITeamScore(-10), 0);

	State->FinishMatch(EApecoxMatchWinner::Players);
	TestEqual(TEXT("Match enters PostMatch"), State->GetApecoxMatchPhase(), EApecoxMatchPhase::PostMatch);
	TestEqual(TEXT("Winner is preserved"), State->GetMatchWinner(), EApecoxMatchWinner::Players);
	TestEqual(TEXT("PostMatch rejects further score changes"), State->AddPlayerTeamScore(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxScoreAttackAssetsColdLoadTest,
	"Apecox.ScoreAttack.AssetsColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxScoreAttackAssetsColdLoadTest::RunTest(const FString& Parameters)
{
	const UClass* BotClass = LoadObject<UClass>(
		nullptr, TEXT("/Game/Blueprints/Characters/BP_ApecoxBotCharacter.BP_ApecoxBotCharacter_C"));
	if (!TestNotNull(TEXT("Bot Blueprint cold-loads"), BotClass))
	{
		return false;
	}
	TestTrue(TEXT("Bot Blueprint derives from combat bot"), BotClass->IsChildOf(AApecoxBotCharacter::StaticClass()));

	const AApecoxBotCharacter* BotDefaults = Cast<AApecoxBotCharacter>(BotClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Bot defaults resolve"), BotDefaults))
	{
		return false;
	}

	const FObjectProperty* WeaponProperty = FindFProperty<FObjectProperty>(BotClass, TEXT("StartingWeaponDefinition"));
	const UApecoxWeaponDefinition* WeaponDefinition = WeaponProperty
		? Cast<UApecoxWeaponDefinition>(WeaponProperty->GetObjectPropertyValue_InContainer(BotDefaults)) : nullptr;
	TestNotNull(TEXT("Bot cold-loads with a starting rifle definition"), WeaponDefinition);
	TestNotNull(TEXT("Bot keeps an AI controller class"), BotDefaults->AIControllerClass.Get());
	if (BotDefaults->AIControllerClass)
	{
		TestTrue(TEXT("Bot keeps the project combat AI controller"),
			BotDefaults->AIControllerClass == AApecoxWanderAIController::StaticClass()
			|| BotDefaults->AIControllerClass->IsChildOf(AApecoxWanderAIController::StaticClass()));
	}

	USkeletalMeshComponent* Mesh = BotDefaults->GetMesh();
	TestNotNull(TEXT("Bot has a third-person skeletal mesh component"), Mesh);
	if (Mesh)
	{
		TestTrue(TEXT("Bot mesh keeps the weapon_r attachment socket"),
			Mesh->DoesSocketExist(TEXT("weapon_r")));
		const UClass* AnimClass = Mesh->GetAnimClass();
		TestNotNull(TEXT("Bot cold-loads with a rifle AnimBP"), AnimClass);
		if (AnimClass)
		{
			TestTrue(TEXT("Bot rifle AnimBP derives from shared Apecox AnimInstance"),
				AnimClass->IsChildOf(UApecoxCharacterAnimInstance::StaticClass()));
		}
	}

	UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Blueprints/Maps/L_Apecox_DevGym.L_Apecox_DevGym"));
	if (!TestNotNull(TEXT("Demo map cold-loads"), Map) || !Map->PersistentLevel)
	{
		return false;
	}

	int32 BotCount = 0;
	int32 PlayerStartCount = 0;
	int32 NavBoundsCount = 0;
	for (const AActor* Actor : Map->PersistentLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		BotCount += Actor->IsA(BotClass) ? 1 : 0;
		PlayerStartCount += Actor->IsA<APlayerStart>() ? 1 : 0;
		NavBoundsCount += Actor->IsA<ANavMeshBoundsVolume>() ? 1 : 0;
	}
	TestTrue(TEXT("Demo map contains at least one combat bot"), BotCount > 0);
	TestTrue(TEXT("Demo map contains player starts"), PlayerStartCount > 0);
	TestTrue(TEXT("Demo map contains navigation bounds"), NavBoundsCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxScoreAttackObjectivePatrolDefaultsTest,
	"Apecox.ScoreAttack.ObjectivePatrolDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxScoreAttackObjectivePatrolDefaultsTest::RunTest(const FString& Parameters)
{
	const AApecoxWanderAIController* ControllerDefaults =
		GetDefault<AApecoxWanderAIController>();
	if (!TestNotNull(TEXT("Combat AI controller defaults resolve"), ControllerDefaults))
	{
		return false;
	}

	TestTrue(TEXT("AI acquisition is local rather than map-wide"),
		ControllerDefaults->GetAcquireRange() <= 2400.0f);
	TestTrue(TEXT("AI only fires inside its acquisition range"),
		ControllerDefaults->GetFireRange() <= ControllerDefaults->GetAcquireRange());

	const AApecoxAIObjectivePoint* ObjectiveDefaults = GetDefault<AApecoxAIObjectivePoint>();
	if (!TestNotNull(TEXT("Objective point defaults resolve"), ObjectiveDefaults))
	{
		return false;
	}
	TestTrue(TEXT("Objective point is enabled by default"), ObjectiveDefaults->bEnabled);
	TestTrue(TEXT("Objective point has a positive selection weight"),
		ObjectiveDefaults->SelectionWeight > 0.0f);
	TestTrue(TEXT("Objective search duration is ordered"),
		ObjectiveDefaults->SearchDurationMax >= ObjectiveDefaults->SearchDurationMin);
	TestTrue(TEXT("Objective arrival radius is usable"),
		ObjectiveDefaults->AcceptanceRadius >= 25.0f);
	return true;
}

#endif
