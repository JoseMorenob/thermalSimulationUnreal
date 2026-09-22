// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

// El proyecto principal contiene la escena de demostración. La funcionalidad
// reutilizable y el modelo de radiancia viven en el plugin IRSimPlugin.

public class IRSimClean : ModuleRules
{
	public IRSimClean(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "IRSimPlugin"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Slate",
			"SlateCore",
			"RenderCore",
			"RHI"
		});

		// IRPipelineCore es la segunda etapa: convierte radiancia física en
		// respuesta de detector. Se distribuye como una biblioteca estática.
		string IRPipelineCorePath = Path.Combine(ModuleDirectory, "..", "IRPipelineCore");
		PublicIncludePaths.Add(Path.Combine(IRPipelineCorePath, "Include"));
		PublicAdditionalLibraries.Add(Path.Combine(IRPipelineCorePath, "Lib", "IRPipelineCore.lib"));
	}
}
