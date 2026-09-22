// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ApecoxPlayerCharacter.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Curves/CurveVector.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxWeaponRecoilTest,
	"Apecox.Weapon.Recoil", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxWeaponRecoilTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	Names.Add(TEXT("RARCurveSampling"));
	Commands.Add(TEXT("RARCurveSampling"));
	Names.Add(TEXT("SilentRecoveryPreservesAimDirection"));
	Commands.Add(TEXT("SilentRecoveryPreservesAimDirection"));
	Names.Add(TEXT("LastShotKickPersistsAsAimDirection"));
	Commands.Add(TEXT("LastShotKickPersistsAsAimDirection"));
	Names.Add(TEXT("CameraRollDoesNotTiltHorizon"));
	Commands.Add(TEXT("CameraRollDoesNotTiltHorizon"));
	Names.Add(TEXT("QuickRefireStartsFromNewBurstOrigin"));
	Commands.Add(TEXT("QuickRefireStartsFromNewBurstOrigin"));
	Names.Add(TEXT("ResetDoesNotMovePlayerView"));
	Commands.Add(TEXT("ResetDoesNotMovePlayerView"));
}

bool FApecoxWeaponRecoilTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("RARCurveSampling"))
	{
		UCurveVector* Curve = NewObject<UCurveVector>(GetTransientPackage());
		Curve->FloatCurves[0].AddKey(1.0f, 2.0f);
		Curve->FloatCurves[0].AddKey(2.0f, 5.0f);
		Curve->FloatCurves[1].AddKey(1.0f, 3.0f);
		Curve->FloatCurves[1].AddKey(2.0f, 7.0f);
		Curve->FloatCurves[2].AddKey(1.0f, 4.0f);
		Curve->FloatCurves[2].AddKey(2.0f, 9.0f);

		FApecoxProjectileFireConfig Config;
		Config.StandingWeaponRecoilLocationCurve = Curve;
		Config.StandingWeaponRecoilRotationCurve = Curve;
		Config.CameraRecoilRotationCurve = Curve;
		FVector Location;
		FVector Rotation;
		FVector Camera;
		Config.EvaluateRecoilTargets(0, false, Location, Rotation, Camera);
		TestEqual(TEXT("Burst index zero samples RAR shot count one"),
			Location, FVector(2.0, 3.0, 4.0) * 1.2, 0.001f);
		TestEqual(TEXT("Standing weapon rotation uses RAR multiplier"),
			Rotation, FVector(2.0, 3.0, 4.0) * 0.6, 0.001f);
		TestEqual(TEXT("Standing camera uses RAR multiplier"),
			Camera, FVector(2.0, 3.0, 4.0) * 1.3, 0.001f);
		Config.EvaluateRecoilTargets(1, false, Location, Rotation, Camera);
		TestEqual(TEXT("Second burst shot samples curve key two"),
			Location, FVector(5.0, 7.0, 9.0) * 1.2, 0.001f);
		return true;
	}

	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AApecoxPlayerCharacter* Character = World->SpawnActor<AApecoxPlayerCharacter>(SpawnParameters);
	APlayerController* Controller = World->SpawnActor<APlayerController>(SpawnParameters);
	if (!TestNotNull(TEXT("Character spawned"), Character)
		|| !TestNotNull(TEXT("Controller spawned"), Controller))
	{
		return false;
	}
	Controller->SetAsLocalPlayerController();
	Controller->Possess(Character);

	Character->RecoilController = Controller;
	Character->CameraRecoilRotationSpringParameters = FVector(0.6, 0.5, 0.002);

	if (Parameters == TEXT("SilentRecoveryPreservesAimDirection"))
	{
		Character->AppliedCameraRecoilRotation = FVector(1.0, 2.0, 0.5);
		Character->CurrentCameraRecoilRotation = FVector(1.0, 2.0, 0.5);
		Character->TargetCameraRecoilRotation = FVector::ZeroVector;
		Character->bCameraRecoilKickActive = false;
		Controller->SetControlRotation(FRotator(-1.0, 4.0, 0.0));
		for (int32 Frame = 0; Frame < 360; ++Frame)
		{
			Character->UpdateFirstPersonRecoil(1.0f / 120.0f);
		}

		TestEqual(TEXT("Internal camera recoil fully recovers"),
			Character->AppliedCameraRecoilRotation.Y, 0.0, 0.001);
		TestEqual(TEXT("Silent recovery preserves compensated pitch"),
			Controller->GetControlRotation().Pitch, -1.0, 0.01);
		TestEqual(TEXT("Silent recovery preserves compensated yaw"),
			Controller->GetControlRotation().Yaw, 4.0, 0.01);
		return true;
	}

	if (Parameters == TEXT("LastShotKickPersistsAsAimDirection")
		|| Parameters == TEXT("CameraRollDoesNotTiltHorizon"))
	{
		const bool bRollOnly = Parameters == TEXT("CameraRollDoesNotTiltHorizon");
		Character->TargetCameraRecoilRotation = bRollOnly
			? FVector(2.0, 0.0, 0.0) : FVector(1.0, 2.0, 0.5);
		Character->bCameraRecoilKickActive = true;
		Controller->SetControlRotation(FRotator::ZeroRotator);
		double MaxObservedPitch = 0.0;
		for (int32 Frame = 0; Frame < 720; ++Frame)
		{
			Character->UpdateFirstPersonRecoil(1.0f / 120.0f);
			MaxObservedPitch = FMath::Max(MaxObservedPitch,
				static_cast<double>(Controller->GetControlRotation().Pitch));
		}

		TestTrue(TEXT("Spawned test pawn is locally controlled"), Character->IsLocallyControlled());
		TestEqual(TEXT("Camera recoil never leaves persistent roll"),
			Controller->GetControlRotation().Roll, 0.0, 0.01);
		if (!bRollOnly)
		{
			TestTrue(TEXT("Last shot visibly reaches its pitch target"), MaxObservedPitch >= 1.95);
			TestEqual(TEXT("Last-shot pitch becomes the new aim direction"),
				Controller->GetControlRotation().Pitch, 2.0, 0.02);
			TestEqual(TEXT("Last-shot yaw becomes the new aim direction"),
				Controller->GetControlRotation().Yaw, 0.5, 0.02);
		}
		TestEqual(TEXT("Last-shot internal camera recoil fully recovers"),
			Character->AppliedCameraRecoilRotation.Y, 0.0, 0.001);
		return true;
	}

	if (Parameters == TEXT("QuickRefireStartsFromNewBurstOrigin"))
	{
		// 上一轮连发已经把真实瞄准推到5度，但内部弹簧只静默恢复到2度。
		// 新一轮首发目标小于旧残量；若不重置每轮坐标，首帧会产生负增量并向下跳。
		Character->CurrentCameraRecoilRotation = FVector(0.0, 2.0, 0.0);
		Character->AppliedCameraRecoilRotation = FVector(0.0, 2.0, 0.0);
		Character->TargetCameraRecoilRotation = FVector::ZeroVector;
		Character->CameraRecoilRotationSpringState.Velocity = FVector(0.0, -1.0, 0.0);
		Controller->SetControlRotation(FRotator(5.0, 0.0, 0.0));
		Character->StartCameraRecoilKick(FVector(0.0, 0.5, 0.0), true);

		double MinimumObservedPitch = Controller->GetControlRotation().Pitch;
		for (int32 Frame = 0; Frame < 720; ++Frame)
		{
			Character->UpdateFirstPersonRecoil(1.0f / 120.0f);
			MinimumObservedPitch = FMath::Min(MinimumObservedPitch,
				static_cast<double>(Controller->GetControlRotation().Pitch));
		}

		TestTrue(TEXT("Quick refire never applies the previous burst recovery as a downward kick"),
			MinimumObservedPitch >= 4.999);
		TestEqual(TEXT("New burst adds its own kick to the preserved aim direction"),
			Controller->GetControlRotation().Pitch, 5.5, 0.02);
		TestEqual(TEXT("New burst origin discards previous internal recovery velocity"),
			Character->AppliedCameraRecoilRotation.Y, 0.0, 0.001);
		return true;
	}

	Character->AppliedCameraRecoilRotation = FVector(0.0, 2.0, 0.0);
	Character->CurrentCameraRecoilRotation = FVector(0.0, 2.0, 0.0);
	Character->bCameraRecoilKickActive = true;
	Controller->SetControlRotation(FRotator(1.0, 0.0, 0.0));
	Character->ResetFirstPersonRecoil();
	TestEqual(TEXT("Lifecycle reset leaves the player view unchanged"),
		Controller->GetControlRotation().Pitch, 1.0, 0.01);
	TestTrue(TEXT("Lifecycle reset clears camera recoil state"),
		Character->AppliedCameraRecoilRotation.IsZero() && !Character->bCameraRecoilKickActive);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
