// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ApecoxPlayerCharacter.h"
#include "Animation/ApecoxFirstPersonAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/Weapons/ApecoxProjectileFireAbility.h"
#include "AbilitySystem/TargetData/ApecoxRangedShotTargetData.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "GameplayTags/ApecoxGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/CollisionProfile.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"

namespace
{
// 无商业资产的真实World/Character/Equipment/ASC射击夹具。
struct FMovementCombatFixture
{
	FTestWorldWrapper TestWorld;
	UWorld* World = nullptr;
	AApecoxPlayerCharacter* Character = nullptr;
	APlayerController* Controller = nullptr;
	UCharacterMovementComponent* Movement = nullptr;
	UApecoxAbilitySystemComponent* ASC = nullptr;
	UApecoxRangedWeaponInstance* Weapon = nullptr;
	UApecoxWeaponPresentationDefinition* Presentation = nullptr;
	FGameplayAbilitySpecHandle FireHandle;

	bool Initialize(FAutomationTestBase& Test)
	{
		if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
		{
			TestWorld.ForwardErrorMessages(&Test);
			return false;
		}
		World = TestWorld.GetTestWorld();
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Character = World->SpawnActor<AApecoxPlayerCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Spawn);
		Controller = World->SpawnActor<APlayerController>();
		if (!Test.TestNotNull(TEXT("Character"), Character) || !Test.TestNotNull(TEXT("Controller"), Controller))
		{
			return false;
		}
		// UE5.8无NetDriver的测试世界按LocalPlayer/显式标记识别本地Controller。
		Controller->SetAsLocalPlayerController();
		Controller->Possess(Character);
		if (!Test.TestTrue(TEXT("Fixture has a local owning player"), Character->IsLocallyControlled()))
		{
			return false;
		}
		Controller->SetControlRotation(FRotator::ZeroRotator);
		// 由测试显式驱动输入/状态，World Tick只推进射速时钟，不让无地板测试世界中的Pawn坠落。
		Character->SetActorTickEnabled(false);
		Controller->SetActorTickEnabled(false);
		Movement = Character->GetCharacterMovement();
		Movement->SetComponentTickEnabled(false);
		Movement->SetMovementMode(MOVE_Falling);
		Movement->SetMovementMode(MOVE_Walking);
		ASC = NewObject<UApecoxAbilitySystemComponent>(Character);
		Character->AddInstanceComponent(ASC);
		ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(Character, Character);
		Character->GetEquipmentComponent()->InitializeWithAbilitySystem(ASC);
		UApecoxWeaponDefinition* Definition = NewObject<UApecoxWeaponDefinition>();
		Definition->FireConfig.InitializeAs<FApecoxProjectileFireConfig>();
		Presentation = NewObject<UApecoxWeaponPresentationDefinition>();
		Definition->PresentationDefinition = Presentation;
		Weapon = NewObject<UApecoxRangedWeaponInstance>(Controller);
		Weapon->Initialize(Definition);
		if (!Test.TestTrue(TEXT("Equip real weapon instance"),
			Character->GetEquipmentComponent()->EquipWeapon(EApecoxWeaponSlot::Primary, Weapon)))
		{
			return false;
		}
		FGameplayAbilitySpec FireSpec(UApecoxProjectileFireAbility::StaticClass(), 1, INDEX_NONE, Weapon);
		FireSpec.GetDynamicSpecSourceTags().AddTag(ApecoxGameplayTags::InputTag_Weapon_Fire);
		FireHandle = ASC->GiveAbility(FireSpec);
		return true;
	}

	void FireFrame()
	{
		World->Tick(LEVELTICK_All, 0.2f);
		ASC->ProcessAbilityInput(0.2f, false);
	}
};
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxMovementCombatTest,
	"Apecox.Movement.CombatRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxMovementCombatTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("ProneJump"), TEXT("ProneJumpLowCeiling"), TEXT("SprintFirePriority"),
		TEXT("ProneFirePriority"), TEXT("ProneCoastFirePriority"), TEXT("StandingMove"), TEXT("CrouchedMove"),
		TEXT("FireThenProne"), TEXT("FireInputFirst"), TEXT("UnarmedFire"), TEXT("DelayedTargetData"),
		TEXT("BurstRelease"), TEXT("AimState"), TEXT("AimFOV"),
		TEXT("InspectEligibility"), TEXT("InspectInterrupts"), TEXT("InspectStanceInterrupts"),
		TEXT("LaserActionGating"), TEXT("MovementAudioDistances") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FApecoxMovementCombatTest::RunTest(const FString& Parameters)
{
	FMovementCombatFixture F;
	if (!F.Initialize(*this)) { return false; }
	AApecoxPlayerCharacter* C = F.Character;
	if (Parameters == TEXT("LaserActionGating"))
	{
		F.Presentation->bSupportsLaser = true;
		TestTrue(TEXT("Armed living character can use laser"), C->CanUseWeaponLaser());

		C->bIsSprinting = true;
		TestFalse(TEXT("Sprint hides laser"), C->CanUseWeaponLaser());
		C->bIsSprinting = false;

		TestTrue(TEXT("Server shot creates room for reload"),
			F.Weapon->CommitServerShot(1, 1.0f, 1, 0));
		TestTrue(TEXT("Reload transaction starts"), F.Weapon->BeginReload());
		TestFalse(TEXT("Reload hides laser"), C->CanUseWeaponLaser());
		F.Weapon->FinishReload();

		C->bIsInspecting = true;
		TestFalse(TEXT("Inspect hides laser"), C->CanUseWeaponLaser());
		return true;
	}
	if (Parameters == TEXT("MovementAudioDistances"))
	{
		TestEqual(TEXT("Standing movement uses Apecox-calibrated walk stride"), C->GetCurrentFootstepDistance(), 200.0f);
		C->bIsAiming = true;
		TestEqual(TEXT("ADS uses Apecox-calibrated aim stride"), C->GetCurrentFootstepDistance(), 170.0f);
		C->bIsAiming = false;
		C->bIsSprinting = true;
		TestEqual(TEXT("Sprint preserves RAR fast-run cadence at Apecox speed"), C->GetCurrentFootstepDistance(), 250.0f);
		C->bIsSprinting = false;
		C->bIsProne = true;
		TestEqual(TEXT("Prone reuses low-stance stride"), C->GetCurrentFootstepDistance(), 100.0f);

		C->AccumulatedFootstepDistance = 0.0f;
		TestFalse(TEXT("Partial travel does not emit a footstep"), C->AccumulateFootstepDistance(199.0f, 200.0f));
		TestTrue(TEXT("Crossing the stride emits one footstep"), C->AccumulateFootstepDistance(2.0f, 200.0f));
		TestEqual(TEXT("Stride overflow is preserved"), C->AccumulatedFootstepDistance, 1.0f);
		return true;
	}
	if (Parameters == TEXT("AimState"))
	{
		C->HandleAimStarted(FInputActionValue(true));
		TestTrue(TEXT("Armed character enters ADS immediately"), C->IsAiming());
		C->HandleSprintStarted(FInputActionValue(true));
		TestFalse(TEXT("Sprint input exits ADS"), C->IsAiming());
		TestFalse(TEXT("Sprint input clears toggled aim intent"), C->bAimInputRequested);
		C->GetEquipmentComponent()->UnequipCurrentWeapon();
		C->HandleAimStarted(FInputActionValue(true));
		TestFalse(TEXT("Unarmed character cannot aim"), C->IsAiming());
		return true;
	}
	if (Parameters == TEXT("AimFOV"))
	{
		const float CameraFOV = C->BaseCameraFieldOfView;
		C->HandleAimStarted(FInputActionValue(true));
		C->UpdateAimState(0.2f);
		TestEqual(TEXT("ADS reaches full alpha in configured duration"), C->GetAimAlpha(), 1.0f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("ADS camera FOV follows RAR 0.75 multiplier"),
			C->GetFirstPersonCamera()->FieldOfView, CameraFOV * 0.75f, KINDA_SMALL_NUMBER);
		TestEqual(TEXT("ADS viewmodel FOV follows RAR setting"),
			C->GetFirstPersonCamera()->FirstPersonFieldOfView, 100.0f, KINDA_SMALL_NUMBER);
		C->HandleAimStarted(FInputActionValue(true));
		C->UpdateAimState(0.2f);
		TestEqual(TEXT("Second ADS click restores camera FOV"),
			C->GetFirstPersonCamera()->FieldOfView, CameraFOV, KINDA_SMALL_NUMBER);
		return true;
	}
	if (Parameters == TEXT("InspectEligibility"))
	{
		F.Presentation->FirstPersonArmsInspectMontage = NewObject<UAnimMontage>();
		F.Presentation->FirstPersonWeaponInspectMontage = NewObject<UAnimMontage>();
		TestTrue(TEXT("Idle armed local character may inspect"), C->CanInspect());
		C->bIsAiming = true;
		TestFalse(TEXT("ADS blocks starting inspect"), C->CanInspect());
		C->bIsAiming = false;
		C->bIsSprinting = true;
		TestFalse(TEXT("Sprint blocks starting inspect"), C->CanInspect());
		C->bIsSprinting = false;
		C->bWeaponFireInputHeld = true;
		TestFalse(TEXT("Held fire blocks starting inspect"), C->CanInspect());
		C->bWeaponFireInputHeld = false;
		C->HandleCrouchStarted(FInputActionValue(true));
		F.Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		TestFalse(TEXT("Crouch blocks starting inspect"), C->CanInspect());
		C->HandleCrouchCompleted(FInputActionValue(false));
		F.Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		TestTrue(TEXT("Standing again permits inspect"), C->CanInspect());
		TestTrue(TEXT("Enter prone for inspect eligibility"), C->SetProne(true));
		TestFalse(TEXT("Prone blocks starting inspect"), C->CanInspect());
		TestTrue(TEXT("Exit prone for inspect eligibility"), C->SetProne(false));
		F.Presentation->FirstPersonWeaponInspectMontage = nullptr;
		TestFalse(TEXT("Missing paired weapon montage blocks inspect"), C->CanInspect());
		return true;
	}
	if (Parameters == TEXT("InspectInterrupts"))
	{
		C->CachedAbilitySystemComponent = F.ASC;
		C->bIsInspecting = true;
		C->HandleAbilityInputTagPressed(
			FInputActionValue(true), ApecoxGameplayTags::InputTag_Weapon_Fire);
		TestFalse(TEXT("Fire input interrupts inspect before entering GAS"), C->IsInspecting());
		C->HandleAbilityInputTagReleased(
			FInputActionValue(false), ApecoxGameplayTags::InputTag_Weapon_Fire);

		C->bIsInspecting = true;
		C->HandleAimStarted(FInputActionValue(true));
		TestFalse(TEXT("Aim input interrupts inspect"), C->IsInspecting());
		TestTrue(TEXT("Aim enters ADS after interrupting inspect"), C->IsAiming());
		C->HandleAimStarted(FInputActionValue(true));

		C->bIsInspecting = true;
		C->HandleSprintStarted(FInputActionValue(true));
		TestFalse(TEXT("Sprint input interrupts inspect"), C->IsInspecting());
		return true;
	}
	if (Parameters == TEXT("InspectStanceInterrupts"))
	{
		C->bIsInspecting = true;
		C->HandleCrouchStarted(FInputActionValue(true));
		TestFalse(TEXT("Crouch input interrupts inspect before changing stance"), C->IsInspecting());
		C->HandleCrouchCompleted(FInputActionValue(false));

		C->bIsInspecting = true;
		C->HandleProneStarted(FInputActionValue(true));
		TestFalse(TEXT("Prone input interrupts inspect before changing stance"), C->IsInspecting());
		TestTrue(TEXT("Prone transition still executes after interrupt"), C->IsProne());
		return true;
	}
	if (Parameters.StartsWith(TEXT("ProneJump")))
	{
		TestTrue(TEXT("Enter prone"), C->SetProne(true));
		UBoxComponent* Ceiling = nullptr;
		if (Parameters == TEXT("ProneJumpLowCeiling"))
		{
			AActor* Obstacle = F.World->SpawnActor<AActor>();
			Ceiling = NewObject<UBoxComponent>(Obstacle);
			Obstacle->AddInstanceComponent(Ceiling);
			Obstacle->SetRootComponent(Ceiling);
			Ceiling->SetBoxExtent(FVector(200, 200, 10));
			Ceiling->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Obstacle->SetActorLocation(FVector(0, 0, 158));
			Ceiling->RegisterComponent();
		}
		const FVector ProneLocation = C->GetActorLocation();
		C->HandleJumpStarted(FInputActionValue(true));
		C->CheckJumpInput(1.0f / 60.0f);
		TestFalse(TEXT("Stand attempt does not request a jump"), C->bPressedJump != 0);
		TestEqual(TEXT("No upward impulse"), F.Movement->Velocity.Z, 0.0, 0.001);
		TestTrue(TEXT("Still grounded"), F.Movement->IsMovingOnGround());
		if (Ceiling)
		{
			TestTrue(TEXT("Blocked stand stays prone"), C->IsProne());
			TestEqual(TEXT("Blocked stand does not move capsule"), C->GetActorLocation(), ProneLocation, 0.001f);
			Ceiling->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->HandleJumpCompleted(FInputActionValue(false));
			C->HandleJumpStarted(FInputActionValue(true));
			TestFalse(TEXT("Next clear stand still consumes jump"), C->bPressedJump != 0);
		}
		TestFalse(TEXT("Now standing"), C->IsProne());
		C->HandleJumpCompleted(FInputActionValue(false));
		C->HandleJumpStarted(FInputActionValue(true));
		C->CheckJumpInput(1.0f / 60.0f);
		TestTrue(TEXT("A separate new press really jumps"), F.Movement->Velocity.Z > 0.0);
		return true;
	}

	if (Parameters == TEXT("DelayedTargetData"))
	{
		// 模拟Authority激活后等待数据，数据到达前已进入爬行，必须在扣弹前拒绝。
		// 用没有本地标记的另一个Controller模拟Authority上的远端拥有者。
		F.Controller->UnPossess();
		APlayerController* RemoteController = F.World->SpawnActor<APlayerController>();
		RemoteController->Possess(C);
		F.ASC->RefreshAbilityActorInfo();
		if (!TestFalse(TEXT("Remote fixture is not locally controlled"), C->IsLocallyControlled())) { return false; }
		FPredictionKey ClientKey;
		ClientKey.Current = 1;
		TestTrue(TEXT("Authority activation waits for data"), F.ASC->InternalTryActivateAbility(F.FireHandle, ClientKey));
		FGameplayAbilitySpec* Spec = F.ASC->FindAbilitySpecFromHandle(F.FireHandle);
		if (!TestTrue(TEXT("Ability pending"), Spec && Spec->IsActive())) { return false; }
		const FPredictionKey Key = Spec->GetPrimaryInstance()->GetCurrentActivationInfo().GetActivationPredictionKey();
		TestTrue(TEXT("Enter prone while data pending"), C->SetProne(true));
		F.Movement->Velocity = FVector(30, 0, 0);
		FApecoxRangedShotTargetData* Shot = new FApecoxRangedShotTargetData();
		Shot->ShotId = 1;
		Shot->ViewOrigin = C->GetPawnViewLocation();
		Shot->AimDirection = FVector::ForwardVector;
		FGameplayAbilityTargetDataHandle Data;
		Data.Add(Shot);
		F.ASC->AbilityTargetDataSetDelegate(F.FireHandle, Key).Broadcast(Data, FGameplayTag());
		TestEqual(TEXT("Late blocked shot consumes no ammo"), F.Weapon->GetCurrentMagazineAmmo(), 30);
		TestFalse(TEXT("Rejected ability ended"), Spec->IsActive());
		TestFalse(TEXT("TargetData delegate cleaned"), F.ASC->AbilityTargetDataSetDelegate(F.FireHandle, Key).IsBound());
		return true;
	}

	C->CachedAbilitySystemComponent = F.ASC;
	const auto PressFire = [&]() { C->HandleAbilityInputTagPressed(FInputActionValue(true), ApecoxGameplayTags::InputTag_Weapon_Fire); };
	const auto ReleaseFire = [&]() { C->HandleAbilityInputTagReleased(FInputActionValue(false), ApecoxGameplayTags::InputTag_Weapon_Fire); };
	const auto MoveForward = [&]() { C->HandleMoveInput(FInputActionValue(FVector2D(0, 1))); };
	const bool bProne = Parameters.StartsWith(TEXT("Prone"));
	const bool bSprint = Parameters.StartsWith(TEXT("Sprint")) || Parameters == TEXT("FireInputFirst");
	if (Parameters == TEXT("BurstRelease"))
	{
		const uint32 FirstBurst = F.Weapon->GetLocalBurstId();
		PressFire();
		F.FireFrame();
		TestEqual(TEXT("Fire input starts local burst"), F.Weapon->GetLocalBurstShotCount(), 1);
		ReleaseFire();
		TestEqual(TEXT("Real character release clears local burst"), F.Weapon->GetLocalBurstShotCount(), 0);
		TestTrue(TEXT("Real character release changes burst identity"), F.Weapon->GetLocalBurstId() != FirstBurst);
		return true;
	}
	if (Parameters == TEXT("UnarmedFire"))
	{
		C->GetEquipmentComponent()->UnequipCurrentWeapon();
		PressFire();
		TestFalse(TEXT("Unarmed fire does not lock locomotion"), C->bWeaponFireInputHeld);
		return true;
	}
	if (bProne) { TestTrue(TEXT("Enter prone"), C->SetProne(true)); }
	if (Parameters == TEXT("CrouchedMove"))
	{
		TestTrue(TEXT("Crouched character may walk off ledges"),
			F.Movement->bCanWalkOffLedgesWhenCrouching);
		C->HandleCrouchStarted(FInputActionValue(true));
		F.Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
	}
	if (Parameters == TEXT("FireInputFirst")) { PressFire(); }
	MoveForward();
	F.Movement->Velocity = FVector(bSprint ? 900 : 75, 0, 0);
	if (bSprint)
	{
		C->HandleSprintStarted(FInputActionValue(true));
		C->UpdateLocomotionState(0.0f);
		TestEqual(TEXT("Sprint eligibility before fire"), C->IsSprinting(), Parameters != TEXT("FireInputFirst"));
	}
	if (Parameters == TEXT("ProneCoastFirePriority")) { C->ConsumeMovementInputVector(); }
	if (Parameters != TEXT("FireInputFirst")) { PressFire(); }
	if (Parameters == TEXT("FireThenProne"))
	{
		TestTrue(TEXT("Enter prone while already holding fire"), C->SetProne(true));
	}
	if (C->IsProne())
	{
		TestTrue(TEXT("Fire stops existing crawl velocity"), F.Movement->Velocity.IsNearlyZero());
		TestTrue(TEXT("Fire consumes queued crawl input"), C->GetPendingMovementInputVector().IsNearlyZero());
	}
	if (bSprint)
	{
		TestFalse(TEXT("Fire cancels sprint"), C->IsSprinting());
		TestEqual(TEXT("Fire restores walking speed"), F.Movement->MaxWalkSpeed, C->WalkSpeed);
		TestFalse(TEXT("Standing forward input is preserved"), C->GetPendingMovementInputVector().IsNearlyZero());
	}
	TestFalse(TEXT("Fire is immediately legal after resolving movement"), C->IsWeaponFireBlockedByMovement());
	F.FireFrame();
	TestEqual(TEXT("First fire-priority shot consumes one round"), F.Weapon->GetCurrentMagazineAmmo(), 29);
	C->ConsumeMovementInputVector();
	MoveForward();
	if (C->IsProne())
	{
		TestTrue(TEXT("Continued fire suppresses new crawl input"), C->GetPendingMovementInputVector().IsNearlyZero());
	}
	else
	{
		TestFalse(TEXT("Ordinary movement still receives input"), C->GetPendingMovementInputVector().IsNearlyZero());
	}
	C->UpdateLocomotionState(0.0f);
	TestFalse(TEXT("Held fire prevents sprint reentry"), C->IsSprinting());
	F.FireFrame();
	TestEqual(TEXT("Held fire continues while conflicting movement is stopped"), F.Weapon->GetCurrentMagazineAmmo(), 28);
	ReleaseFire();
	C->ConsumeMovementInputVector();
	MoveForward();
	C->UpdateLocomotionState(0.0f);
	TestFalse(TEXT("Released fire permits movement input"), C->GetPendingMovementInputVector().IsNearlyZero());
	if (bSprint) { TestTrue(TEXT("Still-held sprint resumes after fire release"), C->IsSprinting()); }
	F.FireFrame();
	TestEqual(TEXT("Release never fires an extra round"), F.Weapon->GetCurrentMagazineAmmo(), 28);
	PressFire();
	C->UninitializeAbilitySystem();
	TestFalse(TEXT("Uninitialization clears held fire intent"), C->bWeaponFireInputHeld);
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxLookSwayTest,
	"Apecox.Camera.LookSway", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxLookSwayTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("TurnReverseReturn"), TEXT("WrapAndReset"), TEXT("FrameRate"),
		TEXT("ADSLock") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FApecoxLookSwayTest::RunTest(const FString& Parameters)
{
	FMovementCombatFixture F;
	if (!F.Initialize(*this)) { return false; }
	AApecoxPlayerCharacter* C = F.Character;
	const FTransform Base(FRotator(0, 15, 0), FVector(0, 0.66, -162.5));
	C->BaseFirstPersonMeshRelativeTransform = Base;
	C->GetFirstPersonMesh()->SetRelativeTransform(Base);
	const FTransform CameraBefore = C->GetFirstPersonCamera()->GetRelativeTransform();
	C->UpdateFirstPersonLookSway(1.0f / 60.0f);
	const auto Turn = [&](float Rate, int32 Frames, float Dt)
	{
		for (int32 I = 0; I < Frames; ++I)
		{
			FRotator Rotation = F.Controller->GetControlRotation();
			Rotation.Yaw += Rate * Dt;
			F.Controller->SetControlRotation(Rotation);
			C->UpdateFirstPersonLookSway(Dt);
			TestTrue(TEXT("Bounded yaw"), FMath::Abs(C->LookSwayAngles.Y) <= FMath::Abs(C->LookSwayMaxRotation.Yaw) + 0.001);
			TestTrue(TEXT("Sway does not change aim"), F.Controller->GetControlRotation().Equals(Rotation, 0.001));
		}
	};
	if (Parameters == TEXT("ADSLock"))
	{
		C->AimAlpha = 1.0f;
		Turn(180, 30, 1.0f / 60.0f);
		TestTrue(TEXT("ADS still exercises internal sway spring"), C->LookSwayAngles.Y < -1.0);
		TestTrue(TEXT("Full ADS masks viewmodel sway exactly"),
			C->GetFirstPersonMesh()->GetRelativeTransform().Equals(Base, 0.001));
		TestTrue(TEXT("ADS sway lock never changes camera transform"),
			C->GetFirstPersonCamera()->GetRelativeTransform().Equals(CameraBefore, 0.001));
	}
	else if (Parameters == TEXT("FrameRate"))
	{
		Turn(120, 15, 1.0f / 30.0f);
		const FVector At30 = C->LookSwayAngles;
		const FVector LocationAt30 = C->LookSwayLocation;
		TestTrue(TEXT("Frame-rate comparison exercises active sway"), At30.Y < -0.5);
		Turn(0, 6, 1.0f / 30.0f);
		const FVector Return30 = C->LookSwayAngles;
		const FVector LocationReturn30 = C->LookSwayLocation;
		C->bLookSwayActive = false;
		C->UpdateFirstPersonLookSway(1.0f / 120.0f);
		Turn(120, 60, 1.0f / 120.0f);
		TestEqual(TEXT("Same turn at 30 and 120 fps"), C->LookSwayAngles, At30, 0.01f);
		TestEqual(TEXT("Same translation at 30 and 120 fps"), C->LookSwayLocation, LocationAt30, 0.01f);
		Turn(0, 24, 1.0f / 120.0f);
		TestEqual(TEXT("Same recenter at 30 and 120 fps"), C->LookSwayAngles, Return30, 0.01f);
		TestEqual(TEXT("Same translation recenter at 30 and 120 fps"), C->LookSwayLocation, LocationReturn30, 0.01f);
	}
	else if (Parameters == TEXT("WrapAndReset"))
	{
		F.Controller->SetControlRotation(FRotator(0, 179, 0));
		C->bLookSwayActive = false;
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		F.Controller->SetControlRotation(FRotator(0, -179, 0));
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		TestTrue(TEXT("Yaw wrap stays a small right turn"), C->LookSwayAngles.Y < 0.0 && C->LookSwayAngles.Y > -1.0);
		C->UpdateFirstPersonLookSway(0.5f);
		TestTrue(TEXT("Hitch resets sway"), C->LookSwayAngles.IsNearlyZero());
		Turn(180, 12, 1.0f / 60.0f);
		C->bEnableFirstPersonLookSway = false;
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		TestTrue(TEXT("Disabled restores author transform"), C->GetFirstPersonMesh()->GetRelativeTransform().Equals(Base, 0.001));
		C->bEnableFirstPersonLookSway = true;
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		Turn(180, 12, 1.0f / 60.0f);
		F.Controller->UnPossess();
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		TestTrue(TEXT("Loss of control restores author transform"), C->GetFirstPersonMesh()->GetRelativeTransform().Equals(Base, 0.001));
		F.Controller->Possess(C);
		F.Controller->SetControlRotation(FRotator(0, 140, 0));
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		TestTrue(TEXT("Repossess rebases rotation without a kick"), C->LookSwayAngles.IsNearlyZero());
		Turn(180, 12, 1.0f / 60.0f);
		C->GetEquipmentComponent()->UnequipCurrentWeapon();
		C->UpdateFirstPersonLookSway(1.0f / 60.0f);
		TestTrue(TEXT("Unarmed restores author transform"), C->GetFirstPersonMesh()->GetRelativeTransform().Equals(Base, 0.001));
	}
	else
	{
		Turn(180, 30, 1.0f / 60.0f);
		TestTrue(TEXT("Right turn lags left"), C->LookSwayAngles.Y < -1.0);
		const FQuat SwayQuat = FRotator(C->LookSwayAngles.X, C->LookSwayAngles.Y, C->LookSwayAngles.Z).Quaternion();
		TestEqual(TEXT("Pivot is camera origin, not skeleton feet"), C->GetFirstPersonMesh()->GetRelativeLocation(),
			SwayQuat.RotateVector(Base.GetLocation()) + C->LookSwayLocation, 0.001f);
		TestTrue(TEXT("Turning adds visible lateral translation"), C->LookSwayLocation.Y < -0.5);
		Turn(-180, 30, 1.0f / 60.0f);
		TestTrue(TEXT("Reversal smoothly changes lag direction"), C->LookSwayAngles.Y > 1.0);
		Turn(0, 120, 1.0f / 60.0f);
		TestTrue(TEXT("Stop returns without accumulated drift"), C->GetFirstPersonMesh()->GetRelativeTransform().Equals(Base, 0.001));
	}
	TestTrue(TEXT("Sway never changes camera transform"), C->GetFirstPersonCamera()->GetRelativeTransform().Equals(CameraBefore, 0.001));
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxTurningTest,
	"Apecox.Animation.Turning", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxTurningTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
	for (const TCHAR* Name : { TEXT("StationaryAndStop"), TEXT("MovementAndAir"), TEXT("CrouchAndProne") })
	{
		Names.Add(Name);
		Commands.Add(Name);
	}
}

bool FApecoxTurningTest::RunTest(const FString& Parameters)
{
	FMovementCombatFixture F;
	if (!F.Initialize(*this)) { return false; }
	UApecoxFirstPersonAnimInstance* Anim = NewObject<UApecoxFirstPersonAnimInstance>(F.Character->GetFirstPersonMesh());
	Anim->OwningCharacter = F.Character;
	Anim->UpdateTurningState(1.0f / 60.0f);
	const auto Turn = [&](int32 Frames, float Rate = 90.0f)
	{
		for (int32 I = 0; I < Frames; ++I)
		{
			FRotator R = F.Controller->GetControlRotation();
			R.Yaw += Rate / 60.0f;
			F.Controller->SetControlRotation(R);
			Anim->UpdateTurningState(1.0f / 60.0f);
		}
	};
	Turn(60);
	TestTrue(TEXT("Stationary yaw engages Turning animation"), Anim->TurningAlpha > 0.5f && Anim->TurningAlpha <= 0.6f);
	if (Parameters == TEXT("MovementAndAir"))
	{
		F.Movement->Velocity = FVector(150, 0, 0);
		Turn(1);
		TestEqual(TEXT("Movement immediately removes extra walk overlay"), Anim->TurningAlpha, 0.0f);
		F.Movement->StopMovementImmediately();
		Turn(60);
		F.Movement->SetMovementMode(MOVE_Falling);
		Turn(1);
		TestEqual(TEXT("Airborne cannot carry turning walk loop"), Anim->TurningAlpha, 0.0f);
	}
	else if (Parameters == TEXT("CrouchAndProne"))
	{
		F.Character->Crouch();
		F.Movement->UpdateCharacterStateBeforeMovement(1.0f / 60.0f);
		Turn(90);
		TestEqual(TEXT("Crouch uses smaller turning weight"), Anim->TurningAlpha, 0.42f, 0.001f);
		F.Character->SetProne(true);
		TestTrue(TEXT("Fixture enters prone"), F.Character->IsProne());
		Turn(1);
		TestEqual(TEXT("Prone removes standing turning animation"), Anim->TurningAlpha, 0.0f);
	}
	else
	{
		Turn(120, 0.0f);
		TestTrue(TEXT("Stopping fades the animation out"), Anim->TurningAlpha < 0.001f);
		F.Character->GetEquipmentComponent()->UnequipCurrentWeapon();
		Turn(60);
		TestEqual(TEXT("Unarmed never adds rifle turning"), Anim->TurningAlpha, 0.0f);
	}
	return true;
}

#endif
