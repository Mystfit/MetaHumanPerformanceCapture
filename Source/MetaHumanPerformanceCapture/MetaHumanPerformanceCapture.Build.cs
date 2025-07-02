// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetaHumanPerformanceCapture : ModuleRules
{
	public MetaHumanPerformanceCapture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
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
				"CoreUObject",
				"Engine",
                "HairStrandsCore",
				"ChaosClothAssetEngine",
				"ChaosOutfitAssetEngine",
                "MetaHumanCharacter",
				"MetaHumanCharacterEditor",
				"MetaHumanCharacterPalette",
				"MetaHumanCharacterPaletteEditor",
				"MetaHumanDefaultPipeline",
				"MetaHumanSDKRuntime",
				// ... add other public dependencies that you statically link with here ...
			}
            );
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "MetaHumanCoreTechLib",
                "MetaHumanDefaultEditorPipeline",
                "UnrealEd",
				"Kismet",
				"SubobjectDataInterface",
				"DataflowEngine",
				"MetaHumanSDKEditor",
				"PerformanceCaptureCore",
				"IKRig",
				"FileUtilities",
				"RigLogicModule",
				"Projects",
				"RenderCore",
				"AssetTools",
				"MaterialEditor",
				"Json",
				"PluginUtils",
				"RigLogicModule",
				"ControlRigDeveloper",
				"EditorScriptingUtilities",
				"Blutility",
				"GeometryScriptingCore",
				"SkeletalMeshUtilitiesCommon",
				// ... add private dependencies that you statically link with here ...	
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
