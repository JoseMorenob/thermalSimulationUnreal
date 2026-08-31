using UnrealBuildTool;

public class IRSimPlugin : ModuleRules
{
    public IRSimPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "InputCore",
                "RenderCore",
                "RHI"
            });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(
                new[]
                {
                    "UnrealEd"
                });
        }
    }
}
