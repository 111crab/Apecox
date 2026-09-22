// Copyright Apecox. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/BlendSpace.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/ApecoxCharacterAnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include "Weapons/ApecoxWeaponPresentationActor.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonHorizontalAimContractTest,
	"Apecox.Animation.ThirdPersonHorizontalAimContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonHorizontalAimContractTest::RunTest(const FString& Parameters)
{
	const UClass* ThirdPersonAnimClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Blueprints/Characters/Animations/ThirdPerson/ABP_Apecox_Rifle_TP.ABP_Apecox_Rifle_TP_C"));
	if (!TestNotNull(TEXT("Third-person rifle AnimBP class cold-loads"), ThirdPersonAnimClass))
	{
		return false;
	}

	TestTrue(TEXT("Third-person rifle AnimBP still derives from the shared native AnimInstance"),
		ThirdPersonAnimClass->IsChildOf(UApecoxCharacterAnimInstance::StaticClass()));

	const FFloatProperty* AimYawProperty = FindFProperty<FFloatProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("AimYaw"));
	const FFloatProperty* RootYawOffsetProperty = FindFProperty<FFloatProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("RootYawOffset"));
	const FBoolProperty* ShouldTurnInPlaceProperty = FindFProperty<FBoolProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("bShouldTurnInPlace"));
	const FBoolProperty* TurnInPlaceLeftProperty = FindFProperty<FBoolProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("bTurnInPlaceLeft"));
	const FBoolProperty* InterruptTurnInPlaceProperty = FindFProperty<FBoolProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("bShouldInterruptTurnInPlace"));
	const FFloatProperty* TurnInPlacePlayRateProperty = FindFProperty<FFloatProperty>(
		UApecoxCharacterAnimInstance::StaticClass(), TEXT("TurnInPlacePlayRate"));
	if (!TestNotNull(TEXT("Shared AnimInstance exposes AimYaw"), AimYawProperty)
		|| !TestNotNull(TEXT("Shared AnimInstance exposes RootYawOffset"), RootYawOffsetProperty)
		|| !TestNotNull(TEXT("Shared AnimInstance exposes the turn request"), ShouldTurnInPlaceProperty)
		|| !TestNotNull(TEXT("Shared AnimInstance exposes the turn direction"), TurnInPlaceLeftProperty)
		|| !TestNotNull(TEXT("Shared AnimInstance exposes the turn interruption request"), InterruptTurnInPlaceProperty)
		|| !TestNotNull(TEXT("Shared AnimInstance exposes the adaptive turn play rate"), TurnInPlacePlayRateProperty))
	{
		return false;
	}

	TestTrue(TEXT("AimYaw can be read by an AnimBlueprint"),
		AimYawProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	TestTrue(TEXT("RootYawOffset can be read by an AnimBlueprint"),
		RootYawOffsetProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	TestTrue(TEXT("Turn request can be read by an AnimBlueprint"),
		ShouldTurnInPlaceProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	TestTrue(TEXT("Turn direction can be read by an AnimBlueprint"),
		TurnInPlaceLeftProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	TestTrue(TEXT("Turn interruption can be read by an AnimBlueprint"),
		InterruptTurnInPlaceProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));
	TestTrue(TEXT("Adaptive turn play rate can be read by an AnimBlueprint"),
		TurnInPlacePlayRateProperty->HasAnyPropertyFlags(CPF_BlueprintVisible));

	const UApecoxCharacterAnimInstance* DefaultAnimInstance =
		GetDefault<UApecoxCharacterAnimInstance>();
	TestTrue(TEXT("AimYaw starts neutral"),
		FMath::IsNearlyZero(AimYawProperty->GetPropertyValue_InContainer(DefaultAnimInstance)));
	TestTrue(TEXT("RootYawOffset starts neutral"),
		FMath::IsNearlyZero(RootYawOffsetProperty->GetPropertyValue_InContainer(DefaultAnimInstance)));
	TestFalse(TEXT("Turn request starts inactive"),
		ShouldTurnInPlaceProperty->GetPropertyValue_InContainer(DefaultAnimInstance));
	TestFalse(TEXT("Turn direction starts neutral"),
		TurnInPlaceLeftProperty->GetPropertyValue_InContainer(DefaultAnimInstance));
	TestFalse(TEXT("Turn interruption starts inactive"),
		InterruptTurnInPlaceProperty->GetPropertyValue_InContainer(DefaultAnimInstance));
	TestTrue(TEXT("Adaptive turn play rate starts at one"), FMath::IsNearlyEqual(
		TurnInPlacePlayRateProperty->GetPropertyValue_InContainer(DefaultAnimInstance), 1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonTurnInPlaceAssetsColdLoadTest,
	"Apecox.Animation.ThirdPersonTurnInPlaceAssetsColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonTurnInPlaceAssetsColdLoadTest::RunTest(const FString& Parameters)
{
	static const TCHAR* TurnSequencePaths[] =
	{
		TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_TurnLeft_90.MM_Rifle_TurnLeft_90"),
		TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_TurnRight_90.MM_Rifle_TurnRight_90"),
		TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnLeft_90.MM_Rifle_Crouch_TurnLeft_90"),
		TEXT("/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/MM_Rifle_Crouch_TurnRight_90.MM_Rifle_Crouch_TurnRight_90"),
	};

	const USkeleton* SharedSkeleton = nullptr;
	for (const TCHAR* Path : TurnSequencePaths)
	{
		const UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, Path);
		if (!TestNotNull(FString::Printf(TEXT("Turn sequence cold-loads: %s"), Path), Sequence))
		{
			continue;
		}

		TestTrue(FString::Printf(TEXT("Turn sequence keeps root-motion extraction metadata: %s"), Path),
			Sequence->HasRootMotion());
		TestNotNull(FString::Printf(TEXT("Turn sequence keeps RemainingTurnYaw: %s"), Path),
			Sequence->GetCurveData().GetCurveData(FName(TEXT("RemainingTurnYaw"))));
		TestNotNull(FString::Printf(TEXT("Turn sequence keeps TurnYawWeight: %s"), Path),
			Sequence->GetCurveData().GetCurveData(FName(TEXT("TurnYawWeight"))));
		TestEqual(FString::Printf(TEXT("Turn sequence contains no migrated Lyra notifies: %s"), Path),
			Sequence->Notifies.Num(), 0);

		if (!SharedSkeleton)
		{
			SharedSkeleton = Sequence->GetSkeleton();
		}
		else
		{
			TestTrue(FString::Printf(TEXT("Turn sequence uses the shared Manny skeleton: %s"), Path),
				Sequence->GetSkeleton() == SharedSkeleton);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonBlendSpaceColdLoadTest,
	"Apecox.Animation.ThirdPersonBlendSpaceColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonBlendSpaceColdLoadTest::RunTest(const FString& Parameters)
{
	const UBlendSpace* BlendSpace = LoadObject<UBlendSpace>(
		nullptr,
		TEXT("/Game/Blueprints/Characters/Animations/ThirdPerson/BS_Apecox_Rifle_TP_Locomotion.BS_Apecox_Rifle_TP_Locomotion"));

	if (!TestNotNull(TEXT("Third-person rifle locomotion Blend Space loads without opening an asset editor"), BlendSpace))
	{
		return false;
	}

	TestEqual(TEXT("Blend Space keeps all configured locomotion samples"), BlendSpace->GetNumberOfBlendSamples(), 20);

	int32 WalkSamplesAtRuntimeSpeed = 0;
	int32 ObsoleteWalkSamples = 0;
	for (const FBlendSample& Sample : BlendSpace->GetBlendSamples())
	{
		const FString AnimationName = Sample.Animation ? Sample.Animation->GetName() : FString();
		if (AnimationName.StartsWith(TEXT("MM_Rifle_Walk_")))
		{
			WalkSamplesAtRuntimeSpeed += FMath::IsNearlyEqual(Sample.SampleValue.Y, 400.0f) ? 1 : 0;
			ObsoleteWalkSamples += FMath::IsNearlyEqual(Sample.SampleValue.Y, 220.0f) ? 1 : 0;
		}
	}
	TestEqual(TEXT("All five rifle walk samples align with the 400 cm/s gameplay speed"),
		WalkSamplesAtRuntimeSpeed, 5);
	TestEqual(TEXT("Blend Space no longer keeps the obsolete 220 cm/s walk row"),
		ObsoleteWalkSamples, 0);

	const FVector ProbeInputs[] =
	{
		FVector(0.0f, 0.0f, 0.0f),
		FVector(0.0f, 400.0f, 0.0f),
		FVector(-90.0f, 600.0f, 0.0f),
		FVector(90.0f, 600.0f, 0.0f),
		FVector(0.0f, 650.0f, 0.0f),
	};

	for (const FVector& Input : ProbeInputs)
	{
		TArray<FBlendSampleData> SampleData;
		int32 CachedTriangulationIndex = INDEX_NONE;
		const bool bFoundSamples = BlendSpace->GetSamplesFromBlendInput(
			Input, SampleData, CachedTriangulationIndex, true);

		TestTrue(FString::Printf(TEXT("Cold-loaded Blend Space resolves input %s"), *Input.ToString()), bFoundSamples);
		TestTrue(FString::Printf(TEXT("Cold-loaded Blend Space returns weighted samples for %s"), *Input.ToString()), !SampleData.IsEmpty());

		float TotalWeight = 0.0f;
		for (const FBlendSampleData& Sample : SampleData)
		{
			TestNotNull(TEXT("Resolved Blend Space sample has an animation"), Sample.Animation.Get());
			TotalWeight += Sample.TotalWeight;
		}
		TestTrue(FString::Printf(TEXT("Blend weights are normalized for %s"), *Input.ToString()),
			FMath::IsNearlyEqual(TotalWeight, 1.0f, UE_KINDA_SMALL_NUMBER));
	}

	const UClass* PlayerClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Blueprints/Characters/BP_ApecoxPlayerCharacter.BP_ApecoxPlayerCharacter_C"));
	if (TestNotNull(TEXT("Player character Blueprint class cold-loads"), PlayerClass))
	{
		const UObject* PlayerDefaults = PlayerClass->GetDefaultObject();
		const FFloatProperty* WalkSpeedProperty = FindFProperty<FFloatProperty>(PlayerClass, TEXT("WalkSpeed"));
		const FFloatProperty* SprintSpeedProperty = FindFProperty<FFloatProperty>(PlayerClass, TEXT("SprintSpeed"));
		if (TestNotNull(TEXT("Player Blueprint exposes WalkSpeed"), WalkSpeedProperty)
			&& TestNotNull(TEXT("Player Blueprint exposes SprintSpeed"), SprintSpeedProperty))
		{
			TestEqual(TEXT("Player Blueprint normal speed matches the Blend Space walk row"),
				WalkSpeedProperty->GetPropertyValue_InContainer(PlayerDefaults), 400.0f);
			TestEqual(TEXT("Player Blueprint sprint speed matches the validated sprint probe"),
				SprintSpeedProperty->GetPropertyValue_InContainer(PlayerDefaults), 650.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonFirePresentationColdLoadTest,
	"Apecox.Animation.ThirdPersonFirePresentationColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonFirePresentationColdLoadTest::RunTest(const FString& Parameters)
{
	const UApecoxWeaponPresentationDefinition* Presentation =
		LoadObject<UApecoxWeaponPresentationDefinition>(
			nullptr,
			TEXT("/Game/Blueprints/Weapons/Rifle/DA_WeaponPresentation_Rifle.DA_WeaponPresentation_Rifle"));
	if (!TestNotNull(TEXT("Rifle presentation loads in a cold editor process"), Presentation))
	{
		return false;
	}

	const UAnimMontage* CharacterMontage = Presentation->ThirdPersonCharacterFireMontage;
	const UAnimMontage* WeaponMontage = Presentation->ThirdPersonWeaponFireMontage;
	const USkeletalMesh* WeaponMesh = Presentation->ThirdPersonWeaponMesh;
	if (!TestNotNull(TEXT("Third-person character fire montage is configured"), CharacterMontage)
		|| !TestNotNull(TEXT("Third-person weapon fire montage is configured"), WeaponMontage)
		|| !TestNotNull(TEXT("Third-person weapon mesh is configured"), WeaponMesh))
	{
		return false;
	}

	TestEqual(TEXT("Character fire montage has one slot track"), CharacterMontage->SlotAnimTracks.Num(), 1);
	if (CharacterMontage->SlotAnimTracks.Num() == 1)
	{
		TestEqual(TEXT("Character fire montage uses Lyra pre-aim additive slot"),
			CharacterMontage->SlotAnimTracks[0].SlotName,
			FName(TEXT("FullBodyAdditivePreAim")));
	}

	TestTrue(TEXT("Weapon fire montage skeleton matches the equipped third-person rifle"),
		WeaponMontage->GetSkeleton() == WeaponMesh->GetSkeleton());
	TestEqual(TEXT("Weapon fire montage has one slot track"), WeaponMontage->SlotAnimTracks.Num(), 1);
	if (WeaponMontage->SlotAnimTracks.Num() == 1)
	{
		TestEqual(TEXT("Weapon fire montage uses its native default slot"),
			WeaponMontage->SlotAnimTracks[0].SlotName, FName(TEXT("DefaultSlot")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonReloadPresentationColdLoadTest,
	"Apecox.Animation.ThirdPersonReloadPresentationColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonReloadPresentationColdLoadTest::RunTest(const FString& Parameters)
{
	const UApecoxWeaponPresentationDefinition* Presentation =
		LoadObject<UApecoxWeaponPresentationDefinition>(
			nullptr,
			TEXT("/Game/Blueprints/Weapons/Rifle/DA_WeaponPresentation_Rifle.DA_WeaponPresentation_Rifle"));
	if (!TestNotNull(TEXT("Rifle presentation loads in a cold editor process"), Presentation))
	{
		return false;
	}

	const UAnimMontage* CharacterMontage = Presentation->ThirdPersonCharacterReloadMontage;
	const UAnimMontage* WeaponMontage = Presentation->ThirdPersonWeaponReloadMontage;
	const USkeletalMesh* WeaponMesh = Presentation->ThirdPersonWeaponMesh;
	if (!TestNotNull(TEXT("Third-person character reload montage is configured"), CharacterMontage)
		|| !TestNotNull(TEXT("Third-person weapon reload montage is configured"), WeaponMontage)
		|| !TestNotNull(TEXT("Third-person weapon mesh is configured"), WeaponMesh))
	{
		return false;
	}

	TestEqual(TEXT("Character reload montage has one clean slot track"),
		CharacterMontage->SlotAnimTracks.Num(), 1);
	if (CharacterMontage->SlotAnimTracks.Num() == 1)
	{
		TestEqual(TEXT("Character reload montage uses the Lyra upper-body slot"),
			CharacterMontage->SlotAnimTracks[0].SlotName, FName(TEXT("UpperBody")));
	}
	TestEqual(TEXT("Apecox character reload montage contains no Lyra-specific notifies"),
		CharacterMontage->Notifies.Num(), 0);
	TestTrue(TEXT("Character reload montage keeps the native Lyra 2.2 second source length"),
		FMath::IsNearlyEqual(CharacterMontage->GetPlayLength(), 2.2f, KINDA_SMALL_NUMBER));

	TestTrue(TEXT("Weapon reload montage skeleton matches the equipped third-person rifle"),
		WeaponMontage->GetSkeleton() == WeaponMesh->GetSkeleton());
	TestEqual(TEXT("Weapon reload montage has one slot track"), WeaponMontage->SlotAnimTracks.Num(), 1);
	if (WeaponMontage->SlotAnimTracks.Num() == 1)
	{
		TestEqual(TEXT("Weapon reload montage uses its native default slot"),
			WeaponMontage->SlotAnimTracks[0].SlotName, FName(TEXT("DefaultSlot")));
	}
	TestTrue(TEXT("Weapon reload montage keeps the matching native 2.2 second length"),
		FMath::IsNearlyEqual(WeaponMontage->GetPlayLength(), 2.2f, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApecoxThirdPersonLaserPresentationColdLoadTest,
	"Apecox.Animation.ThirdPersonLaserPresentationColdLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FApecoxThirdPersonLaserPresentationColdLoadTest::RunTest(const FString& Parameters)
{
	const UApecoxWeaponPresentationDefinition* Presentation =
		LoadObject<UApecoxWeaponPresentationDefinition>(
			nullptr,
			TEXT("/Game/Blueprints/Weapons/Rifle/DA_WeaponPresentation_Rifle.DA_WeaponPresentation_Rifle"));
	if (!TestNotNull(TEXT("Rifle presentation loads for third-person laser validation"), Presentation))
	{
		return false;
	}

	TestTrue(TEXT("Rifle presentation supports laser"), Presentation->bSupportsLaser);
	if (!TestNotNull(TEXT("Third-person rifle mesh is configured"), Presentation->ThirdPersonWeaponMesh.Get())
		|| !TestNotNull(TEXT("Shared laser attachment mesh is configured"), Presentation->LaserAttachmentMesh.Get())
		|| !TestNotNull(TEXT("Shared laser beam mesh is configured"), Presentation->LaserBeamMesh.Get())
		|| !TestNotNull(TEXT("Shared laser dot material is configured"), Presentation->LaserDotMaterial.Get()))
	{
		return false;
	}

	TestFalse(TEXT("Third-person laser mount socket is explicit"),
		Presentation->ThirdPersonLaserAttachmentSocketName.IsNone());
	TestNotNull(TEXT("Third-person rifle provides the configured laser mount socket"),
		Presentation->ThirdPersonWeaponMesh->FindSocket(
			Presentation->ThirdPersonLaserAttachmentSocketName));
	TestFalse(TEXT("Third-person stable laser attachment is explicit"),
		Presentation->ThirdPersonLaserStableAttachmentName.IsNone());
	const bool bHasStableAttachment =
		Presentation->ThirdPersonWeaponMesh->FindSocket(
			Presentation->ThirdPersonLaserStableAttachmentName) != nullptr
		|| Presentation->ThirdPersonWeaponMesh->GetRefSkeleton().FindBoneIndex(
			Presentation->ThirdPersonLaserStableAttachmentName) != INDEX_NONE;
	TestTrue(TEXT("Third-person rifle provides the stable laser attachment bone"),
		bHasStableAttachment);
	TestNotNull(TEXT("RAR laser attachment provides its emitter socket"),
		Presentation->LaserAttachmentMesh->FindSocket(Presentation->LaserEmitterSocketName));
	TestFalse(TEXT("Third-person laser mount transform is finite"),
		Presentation->ThirdPersonLaserAttachmentTransform.ContainsNaN());
	TestFalse(TEXT("Native presentation actor is spawnable as a lightweight TP laser host"),
		AApecoxWeaponPresentationActor::StaticClass()->HasAnyClassFlags(CLASS_Abstract));

	const FBoolProperty* ReplicatedLaserProperty = FindFProperty<FBoolProperty>(
		UApecoxEquipmentComponent::StaticClass(), TEXT("bLaserPresentationEnabled"));
	if (!TestNotNull(TEXT("Equipment exposes the public laser presentation state"),
		ReplicatedLaserProperty))
	{
		return false;
	}
	TestTrue(TEXT("Laser presentation state is replicated"),
		ReplicatedLaserProperty->HasAnyPropertyFlags(CPF_Net));
	TestEqual(TEXT("Laser presentation state has the expected RepNotify"),
		ReplicatedLaserProperty->RepNotifyFunc,
		FName(TEXT("OnRep_LaserPresentationEnabled")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
