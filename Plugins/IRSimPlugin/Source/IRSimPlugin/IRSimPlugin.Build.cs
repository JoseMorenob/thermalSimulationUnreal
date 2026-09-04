using UnrealBuildTool;
using System.IO;

public class IRSimPlugin : ModuleRules
{
    public IRSimPlugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        if (Target.Platform != UnrealTargetPlatform.Win64)
        {
            throw new BuildException("IRSimPlugin currently supports only Win64 because ir_core.lib is built for MSVC x64.");
        }

        string IrCoreRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "ThirdParty", "IRCore"));
        string IrCoreInclude = Path.Combine(IrCoreRoot, "Include");
        string IrCoreLibrary = Path.Combine(IrCoreRoot, "Lib", "Win64", "ir_core.lib");

        if (!File.Exists(IrCoreLibrary))
        {
            throw new BuildException(
                "Missing IR core static library: " + IrCoreLibrary +
                ". Run scripts\\build_ir_core.ps1 from the repository root first.");
        }

        PublicSystemIncludePaths.Add(IrCoreInclude);
        PublicAdditionalLibraries.Add(IrCoreLibrary);
        PublicDefinitions.Add("IR_CORE_STATIC=1");

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
