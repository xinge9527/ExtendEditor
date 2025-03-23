// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SuperManager : ModuleRules
{
	public SuperManager(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] { });

		PrivateIncludePaths.AddRange(
			new string[]
			{
				System.IO.Path.GetFullPath(Target.RelativeEnginePath) + "Source/Editor/Blutility/Private"
			});

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "Blutility", "Niagara", "UMG", "UnrealEd", "EditorScriptingUtilities", "AssetTools"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject", "Engine", "Slate", "SlateCore"
			});

		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}
}