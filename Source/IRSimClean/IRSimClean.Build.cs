// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// El proyecto principal contiene la escena de demostracion la funcionalidad
// reutilizable y el modelo IR viven en el plugin IRSimPlugin

public class IRSimClean : ModuleRules
{
	public IRSimClean(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "IRSimPlugin" });
	}
}
