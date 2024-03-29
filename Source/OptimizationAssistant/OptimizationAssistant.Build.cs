// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OptimizationAssistant : ModuleRules
{
	public OptimizationAssistant(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.bBuildEditor)
        {
            OptimizeCode = CodeOptimization.Never;
        }

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"RenderCore",
				"UnrealEd",
                "EditorStyle",
                "ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
                "AssetRegistry",
                "TargetPlatform",
                "DesktopPlatform",
                "MeshUtilities",
                "MeshDescription",
                "StaticMeshDescription",
                "SkeletalMeshUtilitiesCommon",

				// Project Module
                "HottaFramework",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
