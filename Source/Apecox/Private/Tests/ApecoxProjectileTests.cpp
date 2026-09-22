// Copyright Apecox. All Rights Reserved.
#if WITH_DEV_AUTOMATION_TESTS
#include "Weapons/ApecoxWeaponProjectile.h"
#include "Weapons/ApecoxProjectileTracer.h"
#include "Weapons/ApecoxWeaponFireConfig.h"
#include "Weapons/ApecoxRangedWeaponInstance.h"
#include "Weapons/ApecoxWeaponDefinition.h"
#include "Weapons/ApecoxWeaponPresentationDefinition.h"
#include "Character/ApecoxPlayerCharacter.h"
#include "Equipment/ApecoxEquipmentComponent.h"
#include "AbilitySystem/Abilities/Weapons/ApecoxProjectileFireAbility.h"
#include "AbilitySystem/ApecoxAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/ApecoxVitalAttributeSet.h"
#include "Player/ApecoxPlayerState.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/CollisionProfile.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"
#include "Misc/AutomationTest.h"

namespace
{
UBoxComponent* AddBox(AActor* Actor, FVector Position, FVector Extent)
{
    auto* Box = NewObject<UBoxComponent>(Actor);
    Actor->AddInstanceComponent(Box);
    Actor->SetRootComponent(Box);
    Box->SetBoxExtent(Extent);
    Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Actor->SetActorLocation(Position);
    Box->RegisterComponent();
    return Box;
}
void Advance(FTestWorldWrapper& TestWorld, float Duration)
{
    for (int32 I = 0; I < FMath::CeilToInt(Duration * 120); ++I)
    {
        // Wrapper advances GFrameCounter as well as world time. Raw repeated World::Tick
        // inside one automation frame skips tick functions and timers after the first call.
        TestWorld.TickTestWorld(1.0f / 120.0f);
    }
}
int32 CountProjectiles(UWorld* World)
{
    int32 Count = 0;
    for (TActorIterator<AApecoxWeaponProjectile> It(World); It; ++It)
    {
        if (IsValid(*It)) { ++Count; }
    }
    return Count;
}
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FApecoxProjectileTest,
    "Apecox.Weapon.Projectile", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

void FApecoxProjectileTest::GetTests(TArray<FString>& Names, TArray<FString>& Commands) const
{
    for (const TCHAR* Name : { TEXT("FlightAndGravity"), TEXT("HighSpeedThinWall"), TEXT("ChannelIgnore"),
        TEXT("DelayedDamageOnce"), TEXT("MovedTargetMiss"), TEXT("Lifetime"), TEXT("OwnerIgnored"),
        TEXT("AbilityCommit"), TEXT("AbilityNearWall"), TEXT("AbilityDelayedDamage"), TEXT("InvalidConfig"),
		TEXT("RARVisualDefaults"), TEXT("TracerVisualDefaults"), TEXT("SavedRifleDamageConfig") })
    {
        Names.Add(Name); Commands.Add(Name);
    }
}

bool FApecoxProjectileTest::RunTest(const FString& Parameters)
{
	if (Parameters == TEXT("SavedRifleDamageConfig"))
	{
		const UApecoxWeaponDefinition* Definition = LoadObject<UApecoxWeaponDefinition>(nullptr,
			TEXT("/Game/Blueprints/Weapons/Rifle/DA_Weapon_Rifle.DA_Weapon_Rifle"));
		if (!TestNotNull(TEXT("Saved rifle definition loads"), Definition))
		{
			return false;
		}
		const FApecoxProjectileFireConfig* Config = Definition->GetFireConfig<FApecoxProjectileFireConfig>();
		if (!TestNotNull(TEXT("Saved rifle keeps projectile fire config"), Config))
		{
			return false;
		}
		TestEqual(TEXT("Saved rifle base damage is 13"), Config->BaseDamage, 13.0f);
		return true;
	}

	if (Parameters == TEXT("TracerVisualDefaults"))
	{
		const AApecoxProjectileTracer* TracerCDO = GetDefault<AApecoxProjectileTracer>();
		const UStaticMesh* Mesh = TracerCDO->Visual ? TracerCDO->Visual->GetStaticMesh() : nullptr;
		TestFalse(TEXT("Cosmetic tracer never replicates"), TracerCDO->GetIsReplicated());
		TestNotNull(TEXT("Cosmetic tracer uses a visible mesh"), Mesh);
		if (Mesh)
		{
			TestEqual(TEXT("Tracer reuses the RAR glowing bullet mesh"), Mesh->GetPathName(),
				FString(TEXT("/Game/InfimaGames/ArtCore/Weapons/Guns/Meshes/SM_IG_Projectile_Bullet.SM_IG_Projectile_Bullet")));
		}
		TestTrue(TEXT("Tracer is elongated along flight X"),
			TracerCDO->TracerVisualScale.X > TracerCDO->TracerVisualScale.Y * 10.0f);
		TestTrue(TEXT("Tracer raises emissive intensity above the source material default"),
			TracerCDO->EmissiveIntensity > 100.0f);
		TestTrue(TEXT("Tracer has a bounded cosmetic lifetime"),
			TracerCDO->MaximumLifeSeconds > 0.0f && TracerCDO->MaximumLifeSeconds <= 1.0f);
		const UFunction* TracerRPC = UApecoxEquipmentComponent::StaticClass()->FindFunctionByName(
			TEXT("MulticastPlayProjectileTracer"));
		TestNotNull(TEXT("Equipment exposes the tracer multicast RPC"), TracerRPC);
		if (TracerRPC)
		{
			TestTrue(TEXT("Tracer event is a network multicast"),
				TracerRPC->HasAnyFunctionFlags(FUNC_NetMulticast));
			TestFalse(TEXT("High-frequency tracer multicast is deliberately unreliable"),
				TracerRPC->HasAnyFunctionFlags(FUNC_NetReliable));
		}
		return true;
	}

	if (Parameters == TEXT("RARVisualDefaults"))
	{
		const AApecoxWeaponProjectile* ProjectileCDO = GetDefault<AApecoxWeaponProjectile>();
		const UStaticMesh* Mesh = ProjectileCDO->Visual ? ProjectileCDO->Visual->GetStaticMesh() : nullptr;
		TestNotNull(TEXT("RAR glowing bullet mesh is assigned"), Mesh);
		if (Mesh)
		{
			TestEqual(TEXT("Projectile uses the RAR bullet mesh"), Mesh->GetPathName(),
				FString(TEXT("/Game/InfimaGames/ArtCore/Weapons/Guns/Meshes/SM_IG_Projectile_Bullet.SM_IG_Projectile_Bullet")));
		}
		TestTrue(TEXT("Authoritative projectile mesh is hidden to prevent a replicated duplicate tracer"),
			ProjectileCDO->Visual && ProjectileCDO->Visual->bHiddenInGame);
		return true;
	}

    FTestWorldWrapper TestWorld;
    if (!TestWorld.CreateTestWorld(EWorldType::Game) || !TestWorld.BeginPlayInTestWorld())
    {
        TestWorld.ForwardErrorMessages(this); return false;
    }
    UWorld* World = TestWorld.GetTestWorld();
    FApecoxProjectileFireConfig Config;
    Config.GravityScale = 0;
    Config.MinSpeed = Config.MaxSpeed = 1000;
    Config.LifeSeconds = 2;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (Parameters.StartsWith(TEXT("Ability")) || Parameters == TEXT("InvalidConfig"))
    {
        auto* Character = World->SpawnActor<AApecoxPlayerCharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Spawn);
        auto* Controller = World->SpawnActor<APlayerController>();
        Controller->SetAsLocalPlayerController();
        Controller->Possess(Character);
        Character->SetActorTickEnabled(false);
        Controller->SetActorTickEnabled(false);
        Character->GetCharacterMovement()->SetComponentTickEnabled(false);
        auto* ASC = NewObject<UApecoxAbilitySystemComponent>(Character);
        Character->AddInstanceComponent(ASC); ASC->RegisterComponent();
        ASC->InitAbilityActorInfo(Character, Character);
        Character->GetEquipmentComponent()->InitializeWithAbilitySystem(ASC);
        UAbilitySystemComponent* VictimASC = nullptr;
        if (Parameters == TEXT("AbilityDelayedDamage"))
        {
            // Exercise the native SetByCaller GE using the weapon config's 13 damage default.
            auto* Victim = World->SpawnActor<AApecoxPlayerState>();
            AddBox(Victim, Character->GetPawnViewLocation() + FVector(500, 0, 0), FVector(1, 100, 100));
            Victim->SetActorEnableCollision(true);
            VictimASC = Victim->GetAbilitySystemComponent();
            VictimASC->InitAbilityActorInfo(Victim, Victim);
            VictimASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetMaxHealthAttribute(), 100);
            VictimASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetHealthAttribute(), 100);
        }
        auto* Definition = NewObject<UApecoxWeaponDefinition>();
        Definition->FireConfig.InitializeAs<FApecoxProjectileFireConfig>(Config);
        Definition->PresentationDefinition = NewObject<UApecoxWeaponPresentationDefinition>();
        if (Parameters == TEXT("InvalidConfig"))
        {
            Definition->FireConfig.GetMutable<FApecoxProjectileFireConfig>().MinSpeed = -1;
            AddExpectedError(TEXT("No valid fire configuration"), EAutomationExpectedErrorFlags::Contains, 1);
        }
        auto* Weapon = NewObject<UApecoxRangedWeaponInstance>(Controller);
        Weapon->Initialize(Definition);
        TestTrue(TEXT("Equip projectile weapon"), Character->GetEquipmentComponent()->EquipWeapon(EApecoxWeaponSlot::Primary, Weapon));
        const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UApecoxProjectileFireAbility::StaticClass(), 1, INDEX_NONE, Weapon));
        if (Parameters == TEXT("AbilityNearWall"))
        {
            // Default muzzle is +50cm; this wall sits halfway between camera and muzzle.
            AddBox(World->SpawnActor<AActor>(), Character->GetPawnViewLocation() + FVector(25, 0, 0), FVector(0.5, 100, 100));
        }
        ASC->TryActivateAbility(Handle);
        const bool bInvalid = Parameters == TEXT("InvalidConfig");
        TestEqual(TEXT("Commit exactly one round, invalid config consumes none"), Weapon->GetCurrentMagazineAmmo(), bInvalid ? 30 : 29);
        TestEqual(TEXT("Only clear valid launch has a live projectile"), CountProjectiles(World), (Parameters == TEXT("AbilityCommit") || VictimASC) ? 1 : 0);
        TestFalse(TEXT("Fire ability ends immediately, without waiting for impact"), ASC->FindAbilitySpecFromHandle(Handle)->IsActive());
        if (Parameters == TEXT("AbilityCommit"))
        {
            Character->GetEquipmentComponent()->UnequipCurrentWeapon();
            Advance(TestWorld, 0.1f);
            TestEqual(TEXT("Unequipping does not erase a launched projectile"), CountProjectiles(World), 1);
            TestEqual(TEXT("Flight cannot consume a second round"), Weapon->GetCurrentMagazineAmmo(), 29);
        }
        if (VictimASC)
        {
            TestEqual(TEXT("Fire acceptance does not apply damage"), VictimASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), 100.0f);
            Character->GetEquipmentComponent()->UnequipCurrentWeapon();
            Advance(TestWorld, 0.2f);
            TestEqual(TEXT("Target still healthy while bullet travels"), VictimASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), 100.0f);
            Advance(TestWorld, 0.4f);
            TestEqual(TEXT("Configured GE deals 13 damage after unequip and flight"), VictimASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), 87.0f);
            TestEqual(TEXT("Only one round spent"), Weapon->GetCurrentMagazineAmmo(), 29);
        }
        return true;
    }

    auto* Owner = World->SpawnActor<AActor>();
    Spawn.Owner = Owner;
    const FVector Origin(0, 0, 100);
    auto* Projectile = World->SpawnActor<AApecoxWeaponProjectile>(Origin, FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Projectile"), Projectile)) { return false; }
    FGameplayEffectSpecHandle Spec;
    UAbilitySystemComponent* TargetASC = nullptr;
    AActor* TargetActor = nullptr;
    if (Parameters == TEXT("FlightAndGravity")) { Config.GravityScale = 1; }
    if (Parameters == TEXT("Lifetime")) { Config.LifeSeconds = 0.1f; }
    if (Parameters == TEXT("OwnerIgnored")) { AddBox(Owner, Origin, FVector(80)); }
    if (Parameters == TEXT("HighSpeedThinWall") || Parameters == TEXT("ChannelIgnore"))
    {
        Config.MinSpeed = Config.MaxSpeed = 20000;
        auto* Wall = AddBox(World->SpawnActor<AActor>(), FVector(500, 0, 100), FVector(0.5, 100, 100));
        if (Parameters == TEXT("ChannelIgnore")) { Wall->SetCollisionResponseToChannel(Config.TraceChannel, ECR_Ignore); }
    }
    if (Parameters == TEXT("DelayedDamageOnce") || Parameters == TEXT("MovedTargetMiss"))
    {
        auto* Target = World->SpawnActor<AApecoxPlayerState>();
        TargetActor = Target;
        AddBox(Target, FVector(500, 0, 100), FVector(1, 25, 25));
        Target->SetActorEnableCollision(true);
        TargetASC = Target->GetAbilitySystemComponent();
        TargetASC->InitAbilityActorInfo(Target, Target);
        TargetASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetMaxHealthAttribute(), 100);
        TargetASC->SetNumericAttributeBase(UApecoxVitalAttributeSet::GetHealthAttribute(), 100);
        auto* Damage = NewObject<UGameplayEffect>();
        Damage->DurationPolicy = EGameplayEffectDurationType::Instant;
        FGameplayModifierInfo& Modifier = Damage->Modifiers.AddDefaulted_GetRef();
        Modifier.Attribute = UApecoxVitalAttributeSet::GetHealthAttribute();
        Modifier.ModifierOp = EGameplayModOp::Additive;
        Modifier.ModifierMagnitude = FScalableFloat(-20);
        Spec = FGameplayEffectSpecHandle(new FGameplayEffectSpec(Damage, FGameplayEffectContextHandle(new FGameplayEffectContext()), 1));
    }
    Projectile->Launch(Config, FVector(Config.MinSpeed, 0, 0), Spec, nullptr, nullptr, 1, FHitResult());
    if (TargetASC)
    {
        Advance(TestWorld, 0.2f);
        TestEqual(TEXT("Sight alignment causes no instant damage"), TargetASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), 100.0f);
        if (Parameters == TEXT("MovedTargetMiss")) { TargetActor->SetActorLocation(FVector(500, 200, 100)); }
        Advance(TestWorld, 0.5f);
        TestEqual(TEXT("Damage uses collision-time target position"), TargetASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), Parameters == TEXT("MovedTargetMiss") ? 100.0f : 80.0f);
        if (Parameters == TEXT("DelayedDamageOnce"))
        {
            TestFalse(TEXT("Projectile consumed on first hit"), IsValid(Projectile));
            Advance(TestWorld, 0.1f);
            TestEqual(TEXT("No duplicate hit next frame"), TargetASC->GetNumericAttribute(UApecoxVitalAttributeSet::GetHealthAttribute()), 80.0f);
        }
        return true;
    }
    Advance(TestWorld, Parameters == TEXT("FlightAndGravity") ? 0.5f : 0.15f);
    if (Parameters == TEXT("FlightAndGravity"))
    {
        TestEqual(TEXT("Finite speed travels 500cm in 0.5sec"), Projectile->GetActorLocation().X, 500.0, 1.0);
        const double ExpectedZ = Origin.Z + 0.5 * World->GetGravityZ() * 0.5 * 0.5;
        TestEqual(TEXT("Gravity produces ballistic drop"), Projectile->GetActorLocation().Z, ExpectedZ, 2.0);
    }
    else if (Parameters == TEXT("HighSpeedThinWall") || Parameters == TEXT("Lifetime"))
    {
        TestFalse(TEXT("Projectile reclaimed by collision or lifetime"), IsValid(Projectile));
    }
    else
    {
        TestTrue(TEXT("Ignored geometry does not stop flight"), IsValid(Projectile));
        if (IsValid(Projectile)) { TestTrue(TEXT("Projectile moved past ignored geometry"), Projectile->GetActorLocation().X > 100); }
    }
    return true;
}
#endif
