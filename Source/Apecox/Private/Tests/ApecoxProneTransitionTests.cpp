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

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxProneTransitionTest,
	"Apecox.Movement.ProneTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxProneTransitionTest::GetTests(TArray<FString>& OutNames, TArray<FString>& OutCommands) const
{
	for (const TCHAR* Scenario : { TEXT("Standing"), TEXT("Crouched"), TEXT("CrouchedLowCeiling"), TEXT("PendingCrouch") })
	{
		OutNames.Add(Scenario);
		OutCommands.Add(Scenario);
	}
}

bool FApecoxProneTransitionTest::RunTest(const FString& Parameters)
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
	const double InitialBottomZ = Character->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
	const float StandingHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float ProneHalfHeight = Character->ProneCapsuleHalfHeight;
	const bool bStartCrouched = Parameters.StartsWith(TEXT("Crouched"));
	const bool bLowCeiling = Parameters == TEXT("CrouchedLowCeiling");

	if (bStartCrouched || Parameters == TEXT("PendingCrouch"))
	{
		Character->HandleCrouchStarted(FInputActionValue(true));
		if (bStartCrouched)
		{
			Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
			TestTrue(TEXT("Crouch request took effect"), Character->IsCrouched());
			TestEqual(TEXT("Crouch uses configured half-height"), Capsule->GetUnscaledCapsuleHalfHeight(), 65.0f);
		}
	}

	AActor* Ceiling = nullptr;
	UBoxComponent* CeilingCollision = nullptr;
	if (bLowCeiling)
	{
		// 留出蹲姿空间，但不允许站立：入趴不能依赖先成功 UnCrouch。
		Ceiling = World->SpawnActor<AActor>();
		if (!TestNotNull(TEXT("Ceiling spawned"), Ceiling))
		{
			return false;
		}
		CeilingCollision = NewObject<UBoxComponent>(Ceiling);
		Ceiling->AddInstanceComponent(CeilingCollision);
		Ceiling->SetRootComponent(CeilingCollision);
		CeilingCollision->SetBoxExtent(FVector(200.0, 200.0, 10.0));
		CeilingCollision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Ceiling->SetActorLocation(FVector(0.0, 0.0, 158.0));
		CeilingCollision->RegisterComponent();
	}

	TestTrue(TEXT("Entering prone succeeds from the current stance"), Character->SetProne(true));
	TestTrue(TEXT("Prone state is active"), Character->IsProne());
	TestEqual(TEXT("Immediate prone capsule half-height"), Capsule->GetUnscaledCapsuleHalfHeight(), ProneHalfHeight);

	// 重现用户顺序：Ctrl 蹲下 -> Z 入趴 -> 松开 Ctrl -> 后续 CMC 更新。
	Character->HandleCrouchCompleted(FInputActionValue(false));
	for (int32 Frame = 0; Frame < 3; ++Frame)
	{
		Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		Movement->UpdateCharacterStateAfterMovement(1.0f / 60.0f);
	}
	TestFalse(TEXT("Native crouch ended after entering prone"), Character->IsCrouched());
	TestFalse(TEXT("No pending native crouch request"), Movement->bWantsToCrouch != 0);
	TestEqual(TEXT("Prone height survives release and later CMC updates"), Capsule->GetUnscaledCapsuleHalfHeight(), ProneHalfHeight);
	TestEqual(TEXT("Prone preserves capsule bottom"),
		Character->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight(), InitialBottomZ, 0.01);
	// 镜头已独立控制眼高；胶囊回弹仍由上面的真实尺寸/底部断言捕获。
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		Character->UpdateFirstPersonCamera(1.0f / 60.0f);
	}
	TestEqual(TEXT("Prone view converges to configured independent height"),
		Character->GetPawnViewLocation().Z, InitialBottomZ + Character->ProneViewHeight, 0.01);

	if (bLowCeiling)
	{
		const FVector ProneLocation = Character->GetActorLocation();
		TestFalse(TEXT("Ceiling blocks standing up"), Character->SetProne(false));
		TestTrue(TEXT("Blocked stand keeps prone state"), Character->IsProne());
		TestEqual(TEXT("Blocked stand keeps capsule height"), Capsule->GetUnscaledCapsuleHalfHeight(), ProneHalfHeight);
		TestEqual(TEXT("Blocked stand keeps actor position"), Character->GetActorLocation(), ProneLocation, 0.01f);
		CeilingCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ceiling->Destroy();
	}

	TestTrue(TEXT("Standing succeeds with enough space"), Character->SetProne(false));
	TestFalse(TEXT("Prone state ends after standing"), Character->IsProne());
	TestEqual(TEXT("Standing capsule restored"), Capsule->GetUnscaledCapsuleHalfHeight(), StandingHalfHeight);
	TestEqual(TEXT("Standing preserves capsule bottom"),
		Character->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight(), InitialBottomZ, 0.01);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
