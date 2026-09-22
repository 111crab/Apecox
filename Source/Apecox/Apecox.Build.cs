// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Apecox : ModuleRules
{
	public Apecox(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "ModularGameplay" });

		PrivateDependencyModuleNames.AddRange(new string[] { "NetCore", "Niagara", "AIModule", "NavigationSystem" });
	}
}
