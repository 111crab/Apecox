// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ApecoxPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
UBoxComponent* AddCameraObstacle(UWorld* World, const FVector& Location, const FVector& Extent)
{
	AActor* Obstacle = World->SpawnActor<AActor>();
	if (!Obstacle)
	{
		return nullptr;
	}
	UBoxComponent* Box = NewObject<UBoxComponent>(Obstacle);
	Obstacle->AddInstanceComponent(Box);
	Obstacle->SetRootComponent(Box);
	Box->SetBoxExtent(Extent);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Obstacle->SetActorLocation(Location);
	Box->RegisterComponent();
	return Box;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxStanceCameraTest,
	"Apecox.Camera.Stance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxStanceCameraTest::GetTests(TArray<FString>& OutNames, TArray<FString>& OutCommands) const
{
	for (const TCHAR* Scenario : { TEXT("Crouch"), TEXT("Prone"), TEXT("Reverse"), TEXT("BlockedStand"),
		TEXT("LowCeiling"), TEXT("LeanHeight"), TEXT("Movement"), TEXT("FrameRate"), TEXT("AirCrouch") })
	{
		OutNames.Add(Scenario);
		OutCommands.Add(Scenario);
	}
}

bool FApecoxStanceCameraTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AApecoxPlayerCharacter* Character = World->SpawnActor<AApecoxPlayerCharacter>(
		FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator, SpawnParameters);
	if (!TestNotNull(TEXT("Character spawned"), Character))
	{
		return false;
	}
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	Movement->SetCrouchedHalfHeight(65.0f);
	Movement->SetMovementMode(MOVE_Falling);
	Movement->SetMovementMode(MOVE_Walking);
	const double BottomZ = Character->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
	const double StandingViewZ = Character->GetPawnViewLocation().Z;
	const auto Advance = [Character](int32 Frames, float DeltaSeconds = 1.0f / 60.0f)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			Character->UpdateFirstPersonCamera(DeltaSeconds);
		}
	};
	const auto Crouch = [this, Character, Movement]()
	{
		Character->HandleCrouchStarted(FInputActionValue(true));
		Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		TestTrue(TEXT("Native crouch took effect"), Character->IsCrouched());
	};

	if (Parameters == TEXT("Crouch") || Parameters == TEXT("Prone"))
	{
		const bool bCrouch = Parameters == TEXT("Crouch");
		if (bCrouch)
		{
			Crouch();
		}
		else
		{
			TestTrue(TEXT("Prone entered"), Character->SetProne(true));
		}
		const double TargetZ = BottomZ + (bCrouch ? Character->CrouchedViewHeight : Character->ProneViewHeight);
		TestEqual(TEXT("Capsule transition does not instantly move the view"), Character->GetPawnViewLocation().Z, StandingViewZ, 0.01);
		Advance(1);
		TestTrue(TEXT("First camera frame is between start and target"),
			Character->GetPawnViewLocation().Z < StandingViewZ && Character->GetPawnViewLocation().Z > TargetZ);
		Advance(120);
		TestEqual(TEXT("Stance reaches its independent target height"), Character->GetPawnViewLocation().Z, TargetZ, 0.01);
		const double LowViewZ = Character->GetPawnViewLocation().Z;
		if (bCrouch)
		{
			Character->HandleCrouchCompleted(FInputActionValue(false));
			Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		}
		else
		{
			TestTrue(TEXT("Prone exited"), Character->SetProne(false));
		}
		TestEqual(TEXT("Standing capsule restores without a view jump"), Character->GetPawnViewLocation().Z, LowViewZ, 0.01);
		Advance(120);
		TestEqual(TEXT("Standing view returns to authored baseline"), Character->GetPawnViewLocation().Z, StandingViewZ, 0.01);
	}
	else if (Parameters == TEXT("Reverse"))
	{
		Crouch();
		Advance(4);
		const double PartialCrouchZ = Character->GetPawnViewLocation().Z;
		TestTrue(TEXT("Enter prone during crouch camera transition"), Character->SetProne(true));
		TestEqual(TEXT("Crouch to prone keeps current view"), Character->GetPawnViewLocation().Z, PartialCrouchZ, 0.01);
		Advance(3);
		const double PartialProneZ = Character->GetPawnViewLocation().Z;
		TestTrue(TEXT("Prone continues lowering"), PartialProneZ < PartialCrouchZ);
		TestTrue(TEXT("Reverse to standing before prone finishes"), Character->SetProne(false));
		TestEqual(TEXT("Reversing keeps current view"), Character->GetPawnViewLocation().Z, PartialProneZ, 0.01);
		Advance(1);
		TestTrue(TEXT("View now rises toward standing"), Character->GetPawnViewLocation().Z > PartialProneZ);
		Advance(120);
		TestEqual(TEXT("Interrupted transition leaves no residual offset"), Character->GetPawnViewLocation().Z, StandingViewZ, 0.01);
	}
	else if (Parameters == TEXT("BlockedStand") || Parameters == TEXT("LowCeiling"))
	{
		TestTrue(TEXT("Enter prone"), Character->SetProne(true));
		const bool bLowCeiling = Parameters == TEXT("LowCeiling");
		if (!bLowCeiling)
		{
			Advance(120);
		}
		// 胶囊已经缩小，镜头可能还在高处；低顶测试专门覆盖这段时间。
		UBoxComponent* Ceiling = AddCameraObstacle(World, FVector(0.0, 0.0, 100.0), FVector(200.0, 200.0, 10.0));
		if (!TestNotNull(TEXT("Ceiling created"), Ceiling))
		{
			return false;
		}
		Character->UpdateFirstPersonCamera(0.0f);
		TestTrue(TEXT("Camera sphere stays below ceiling underside"), Character->GetPawnViewLocation().Z <= 82.01);
		const double BlockedViewZ = Character->GetPawnViewLocation().Z;
		TestFalse(TEXT("Ceiling prevents standing"), Character->SetProne(false));
		TestEqual(TEXT("Blocked stand leaves view untouched"), Character->GetPawnViewLocation().Z, BlockedViewZ, 0.01);
		if (!bLowCeiling)
		{
			Advance(30);
			TestEqual(TEXT("Blocked stand does not schedule a false camera rise"), Character->GetPawnViewLocation().Z, BlockedViewZ, 0.01);
		}
		Ceiling->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TestTrue(TEXT("Can stand after clearing ceiling"), Character->SetProne(false));
		TestEqual(TEXT("Clearing ceiling resumes from actual constrained view"), Character->GetPawnViewLocation().Z, BlockedViewZ, 0.01);
		Advance(1);
		TestTrue(TEXT("Camera recovers smoothly after ceiling clears"),
			Character->GetPawnViewLocation().Z > BlockedViewZ && Character->GetPawnViewLocation().Z < StandingViewZ);
		Advance(120);
		TestEqual(TEXT("Recovered standing height"), Character->GetPawnViewLocation().Z, StandingViewZ, 0.01);
	}
	else if (Parameters == TEXT("LeanHeight"))
	{
		// 使用不同于旧固定 Z 的眼高；障碍只覆盖新视点高度。
		Character->CrouchedViewHeight = 100.0f;
		Character->MaxLeanDistance = 45.0f;
		Crouch();
		Advance(120);
		Character->HandleLeanRightStarted(FInputActionValue(true));
		Advance(120);
		TestEqual(TEXT("Full lean in open space"), Character->GetPawnViewLocation().Y, 45.0, 0.01);
		UBoxComponent* Wall = AddCameraObstacle(World, FVector(0.0, 42.0, 104.0), FVector(100.0, 2.0, 10.0));
		if (!TestNotNull(TEXT("Wall created"), Wall))
		{
			return false;
		}
		Advance(1);
		TestTrue(TEXT("Existing lean is immediately limited at the actual eye height"), Character->GetPawnViewLocation().Y <= 32.01);
		TestEqual(TEXT("Lean does not overwrite stance height"), Character->GetPawnViewLocation().Z, 104.0, 0.01);
		Character->HandleLeanRightCompleted(FInputActionValue(false));
		Advance(120);
		TestEqual(TEXT("Releasing lean returns to center"), Character->GetPawnViewLocation().Y, 0.0, 0.01);
	}
	else if (Parameters == TEXT("Movement"))
	{
		TestTrue(TEXT("Enter prone"), Character->SetProne(true));
		Advance(4);
		const FVector BeforeMove = Character->GetPawnViewLocation();
		const FVector MovementDelta(100.0, -50.0, 80.0);
		Character->AddActorWorldOffset(MovementDelta, false, nullptr, ETeleportType::TeleportPhysics);
		Character->UpdateFirstPersonCamera(0.0f);
		TestEqual(TEXT("Actual movement follows immediately during stance blending"),
			Character->GetPawnViewLocation(), BeforeMove + MovementDelta, 0.01f);
	}
	else if (Parameters == TEXT("AirCrouch"))
	{
		// CMC 允许空中蹲伏，并在空中原地缩放胶囊，不能沿用地面保持底部的假设。
		Movement->SetMovementMode(MOVE_Falling);
		Crouch();
		TestEqual(TEXT("In-air capsule shrink does not lift camera"), Character->GetPawnViewLocation().Z, StandingViewZ, 0.01);
		Advance(1);
		const double BeforeReleaseZ = Character->GetPawnViewLocation().Z;
		Character->HandleCrouchCompleted(FInputActionValue(false));
		Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		TestFalse(TEXT("Native air crouch released"), Character->IsCrouched());
		TestEqual(TEXT("In-air capsule expansion does not drop camera"), Character->GetPawnViewLocation().Z, BeforeReleaseZ, 0.01);
	}
	else if (Parameters == TEXT("FrameRate"))
	{
		AApecoxPlayerCharacter* Other = World->SpawnActor<AApecoxPlayerCharacter>(
			FVector(500.0, 0.0, 100.0), FRotator::ZeroRotator, SpawnParameters);
		if (!TestNotNull(TEXT("Second character spawned"), Other))
		{
			return false;
		}
		Other->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		TestTrue(TEXT("First character enters prone"), Character->SetProne(true));
		TestTrue(TEXT("Second character enters prone"), Other->SetProne(true));
		Advance(6, 1.0f / 30.0f);
		for (int32 Frame = 0; Frame < 24; ++Frame)
		{
			Other->UpdateFirstPersonCamera(1.0f / 120.0f);
		}
		TestEqual(TEXT("Same elapsed time at 30 and 120 fps gives same view height"),
			Character->GetPawnViewLocation().Z, Other->GetPawnViewLocation().Z, 0.002);
		TestTrue(TEXT("Frame rate check covers an active transition"),
			Character->GetPawnViewLocation().Z > BottomZ + Character->ProneViewHeight && Character->GetPawnViewLocation().Z < StandingViewZ);
	}
	TestEqual(TEXT("Pawn view uses the actual camera position"),
		Character->GetPawnViewLocation(), Character->GetFirstPersonCamera()->GetComponentLocation(), 0.001f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
