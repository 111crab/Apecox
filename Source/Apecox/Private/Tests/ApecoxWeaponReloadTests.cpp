// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxWeaponReloadTest,
	"Apecox.Weapon.Reload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxWeaponReloadTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("Defaults"), TEXT("Initialize"), TEXT("Tactical"),
		TEXT("Empty"), TEXT("PartialReserve"), TEXT("CancelBeforeCommit"), TEXT("Eligibility") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FApecoxWeaponReloadTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	APlayerController* Owner = World->SpawnActor<APlayerController>();
	UApecoxWeaponDefinition* Definition = NewObject<UApecoxWeaponDefinition>();
	Definition->FireConfig.InitializeAs<FApecoxProjectileFireConfig>();
	UApecoxRangedWeaponInstance* Weapon = NewObject<UApecoxRangedWeaponInstance>(Owner);
	Weapon->Initialize(Definition);

	if (Parameters == TEXT("Defaults"))
	{
		TestTrue(TEXT("RAR reload defaults are valid"), Definition->ReloadConfig.IsValid());
		TestEqual(TEXT("Default reserve matches RAR rifle ammo pool"), Definition->ReloadConfig.InitialReserveAmmo, 150);
		TestEqual(TEXT("Tactical ammo commit matches RAR notify"),
			Definition->ReloadConfig.TacticalReloadCommitTime, 1.101991f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Empty ammo commit matches RAR notify"),
			Definition->ReloadConfig.EmptyReloadCommitTime, 1.424366f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Empty click follows 450 RPM"),
			Definition->ReloadConfig.EmptyFireInterval, 0.133333f, KINDA_SMALL_NUMBER);
		return true;
	}
	if (Parameters == TEXT("Initialize"))
	{
		TestEqual(TEXT("Magazine initializes full"), Weapon->CurrentMagazineAmmo, 30);
		TestEqual(TEXT("Reserve initializes from definition"), Weapon->ReserveAmmo, 150);
		TestEqual(TEXT("Weapon initializes outside reload"), Weapon->ReloadState,
			EApecoxWeaponReloadState::None);
		return true;
	}
	if (Parameters == TEXT("Tactical"))
	{
		Weapon->CurrentMagazineAmmo = 12;
		Weapon->ReserveAmmo = 40;
		TestTrue(TEXT("Tactical reload starts"), Weapon->BeginReload());
		TestEqual(TEXT("Non-empty magazine selects tactical branch"), Weapon->ReloadState,
			EApecoxWeaponReloadState::Tactical);
		TestFalse(TEXT("Reload blocks local shots"), Weapon->CanStartLocalShot(10.0f));
		TestTrue(TEXT("Commit transfers missing rounds"), Weapon->CommitReload());
		TestEqual(TEXT("Magazine reaches capacity"), Weapon->CurrentMagazineAmmo, 30);
		TestEqual(TEXT("Reserve pays exact transfer"), Weapon->ReserveAmmo, 22);
		TestFalse(TEXT("Same reload cannot commit twice"), Weapon->CommitReload());
		Weapon->FinishReload();
		TestEqual(TEXT("Finish clears reload state"), Weapon->ReloadState,
			EApecoxWeaponReloadState::None);
		return true;
	}
	if (Parameters == TEXT("Empty"))
	{
		Weapon->CurrentMagazineAmmo = 0;
		TestTrue(TEXT("Empty reload starts"), Weapon->BeginReload());
		TestEqual(TEXT("Empty magazine selects empty branch"), Weapon->ReloadState,
			EApecoxWeaponReloadState::Empty);
		TestTrue(TEXT("Empty reload commits"), Weapon->CommitReload());
		TestEqual(TEXT("Empty reload fills magazine"), Weapon->CurrentMagazineAmmo, 30);
		TestEqual(TEXT("Reserve decrements"), Weapon->ReserveAmmo, 120);
		return true;
	}
	if (Parameters == TEXT("PartialReserve"))
	{
		Weapon->CurrentMagazineAmmo = 25;
		Weapon->ReserveAmmo = 2;
		TestTrue(TEXT("Partial reserve still permits reload"), Weapon->BeginReload());
		TestTrue(TEXT("Partial reserve commits"), Weapon->CommitReload());
		TestEqual(TEXT("Only available rounds are loaded"), Weapon->CurrentMagazineAmmo, 27);
		TestEqual(TEXT("Reserve clamps at zero"), Weapon->ReserveAmmo, 0);
		return true;
	}
	if (Parameters == TEXT("CancelBeforeCommit"))
	{
		Weapon->CurrentMagazineAmmo = 8;
		TestTrue(TEXT("Reload starts before cancel"), Weapon->BeginReload());
		Weapon->FinishReload();
		TestEqual(TEXT("Cancel before commit preserves magazine"), Weapon->CurrentMagazineAmmo, 8);
		TestEqual(TEXT("Cancel before commit preserves reserve"), Weapon->ReserveAmmo, 150);
		return true;
	}
	if (Parameters == TEXT("Eligibility"))
	{
		TestFalse(TEXT("Full magazine cannot reload"), Weapon->CanReload());
		Weapon->CurrentMagazineAmmo = 20;
		Weapon->ReserveAmmo = 0;
		TestFalse(TEXT("No reserve cannot reload"), Weapon->CanReload());
		Weapon->ReserveAmmo = 1;
		TestTrue(TEXT("Missing round and reserve permit reload"), Weapon->CanReload());
		return true;
	}

	AddError(FString::Printf(TEXT("Unknown reload test command: %s"), *Parameters));
	return false;
}

#endif
