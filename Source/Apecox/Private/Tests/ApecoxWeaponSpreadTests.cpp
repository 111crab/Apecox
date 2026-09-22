// Copyright Apecox. All Rights Reserved.
#if WITH_DEV_AUTOMATION_TESTS
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
UCurveFloat* MakeLinearSpreadCurve(UObject* Outer)
{
	UCurveFloat* Curve = NewObject<UCurveFloat>(Outer);
	Curve->FloatCurve.AddKey(0.0f, 0.0f);
	Curve->FloatCurve.AddKey(0.05f, 0.0f);
	Curve->FloatCurve.AddKey(1.0f, 1.0f);
	return Curve;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxWeaponSpreadTest,
	"Apecox.Weapon.Spread", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxWeaponSpreadTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("CurveAndFirstShot"), TEXT("MovementAndAiming"),
		TEXT("DeterministicEllipse"), TEXT("BurstState"), TEXT("ServerBurstValidation") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FApecoxWeaponSpreadTest::RunTest(const FString& Parameters)
{
	FApecoxProjectileFireConfig Config;
	Config.AutomaticSpreadCurve = MakeLinearSpreadCurve(GetTransientPackage());

	if (Parameters == TEXT("CurveAndFirstShot"))
	{
		TestEqual(TEXT("First shot samples t=0 and stays precise"),
			Config.EvaluateSpreadMultiplier(0, false, false), 0.0f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Early protected range remains precise"),
			Config.EvaluateSpreadMultiplier(1, false, false), 0.0f, KINDA_SMALL_NUMBER);
		TestTrue(TEXT("Long burst increases spread"),
			Config.EvaluateSpreadMultiplier(15, false, false) > Config.EvaluateSpreadMultiplier(2, false, false));
		TestEqual(TEXT("Full magazine reaches curve value one"),
			Config.EvaluateSpreadMultiplier(30, false, false), 1.0f, KINDA_SMALL_NUMBER);
		return true;
	}

	if (Parameters == TEXT("MovementAndAiming"))
	{
		TestEqual(TEXT("Moving first shot receives fixed RAR movement term"),
			Config.EvaluateSpreadMultiplier(0, false, true), 0.3f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Aiming scales continuous term and removes movement term"),
			Config.EvaluateSpreadMultiplier(30, true, true), 0.3f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("Hip full burst combines continuous and movement terms"),
			Config.EvaluateSpreadMultiplier(30, false, true), 1.3f, KINDA_SMALL_NUMBER);
		return true;
	}

	if (Parameters == TEXT("DeterministicEllipse"))
	{
		const FVector Base = FVector::ForwardVector;
		const FVector A = Config.ApplySpread(Base, 81, 30, false, false);
		const FVector B = Config.ApplySpread(Base, 81, 30, false, false);
		const FVector C = Config.ApplySpread(Base, 82, 30, false, false);
		TestTrue(TEXT("Same shot identity returns same direction"), A.Equals(B, KINDA_SMALL_NUMBER));
		TestFalse(TEXT("Different shot identity changes random sample"), A.Equals(C, KINDA_SMALL_NUMBER));
		const FRotator Delta = (A.Rotation() - Base.Rotation()).GetNormalized();
		TestTrue(TEXT("Yaw remains inside configured ellipse"), FMath::Abs(Delta.Yaw) <= 12.01f);
		TestTrue(TEXT("Pitch remains inside configured ellipse"), FMath::Abs(Delta.Pitch) <= 12.01f);
		TestTrue(TEXT("Direction remains normalized"), A.IsUnit(0.001f));
		return true;
	}

	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	AActor* Owner = TestWorld.GetTestWorld()->SpawnActor<AActor>();
	UApecoxWeaponDefinition* Definition = NewObject<UApecoxWeaponDefinition>();
	Definition->FireConfig.InitializeAs<FApecoxProjectileFireConfig>(Config);
	UApecoxRangedWeaponInstance* Weapon = NewObject<UApecoxRangedWeaponInstance>(Owner);
	Weapon->Initialize(Definition);

	if (Parameters == TEXT("BurstState"))
	{
		const uint32 FirstBurst = Weapon->GetLocalBurstId();
		Weapon->StartLocalShot(1.0f);
		TestEqual(TEXT("First local shot has index zero"), Weapon->GetLastStartedLocalBurstShotIndex(), 0);
		Weapon->StartLocalShot(1.1f);
		TestEqual(TEXT("Second local shot advances index"), Weapon->GetLastStartedLocalBurstShotIndex(), 1);
		Weapon->EndLocalFireBurst();
		TestEqual(TEXT("Release clears local shot count"), Weapon->GetLocalBurstShotCount(), 0);
		TestTrue(TEXT("Release advances burst identity"), Weapon->GetLocalBurstId() != FirstBurst);
		Weapon->StartLocalShot(1.2f);
		TestEqual(TEXT("Next press starts at index zero"), Weapon->GetLastStartedLocalBurstShotIndex(), 0);
		return true;
	}

	TestTrue(TEXT("Authority accepts first shot at burst index zero"),
		Weapon->CommitServerShot(1, 1.0f, 1, 0));
	TestTrue(TEXT("Authority accepts increasing index in same burst"),
		Weapon->CommitServerShot(2, 1.1f, 1, 1));
	TestFalse(TEXT("Authority rejects lower index in same burst"),
		Weapon->CanCommitServerShot(3, 1.2f, 1, 0));
	TestFalse(TEXT("New burst cannot begin at nonzero index"),
		Weapon->CanCommitServerShot(3, 1.2f, 2, 2));
	TestTrue(TEXT("New burst begins at index zero"),
		Weapon->CommitServerShot(3, 1.2f, 2, 0));
	TestEqual(TEXT("Server now expects second shot of new burst"), Weapon->GetServerBurstShotCount(), 1);
	return true;
}
#endif
