// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ExampleProject : ModuleRules
{
	public ExampleProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ExampleProject",
			"ExampleProject/Variant_Platforming",
			"ExampleProject/Variant_Platforming/Animation",
			"ExampleProject/Variant_Combat",
			"ExampleProject/Variant_Combat/AI",
			"ExampleProject/Variant_Combat/Animation",
			"ExampleProject/Variant_Combat/Gameplay",
			"ExampleProject/Variant_Combat/Interfaces",
			"ExampleProject/Variant_Combat/UI",
			"ExampleProject/Variant_SideScrolling",
			"ExampleProject/Variant_SideScrolling/AI",
			"ExampleProject/Variant_SideScrolling/Gameplay",
			"ExampleProject/Variant_SideScrolling/Interfaces",
			"ExampleProject/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
