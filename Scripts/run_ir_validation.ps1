param(
    [string]$Test = 'IRSimClean.Radiance.',
    [string]$Label = 'suite',
    [switch]$AllIsolated
)
$ErrorActionPreference = 'Stop'
if ($AllIsolated) {
    $names = @('CpuGpu', 'Atmosphere', 'AuxiliaryBuffers', 'FresnelAngular', 'AtmosphereEmission')
    foreach ($name in $names) {
        & $PSCommandPath -Test "IRSimClean.Radiance.$name" -Label "isolated-$name"
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    & $PSCommandPath -Label 'suite-after-isolated'
    exit $LASTEXITCODE
}
try {
    $root = (Resolve-Path "$PSScriptRoot/../../..").Path
    $out = Join-Path $root 'output/buffer-validation-2026-09-06'
    New-Item -ItemType Directory -Force $out | Out-Null
    $editor = 'C:/Program Files/Epic Games/UE_5.6/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
    $project = Join-Path $root 'TFM/IRSimClean/IRSimClean.uproject'
    & $editor $project '/Game/Maps/IRBufferValidation' '-unattended' '-nop4' '-nosplash' '-nosound' '-d3d11' '-RenderOffscreen' "-ExecCmds=Automation RunTests $Test" '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$out/$Label-report" "-abslog=$out/$Label.log" *> "$out/$Label-console.log"
    $code = $LASTEXITCODE
    Write-Output "Unreal exit code: $code; log: $out/$Label.log"
    if ($code -ne 0) { exit $code }
    $report = Get-Content "$out/$Label-report/index.json" -Raw | ConvertFrom-Json
    if ($report.failed -gt 0 -or $report.succeeded -lt 1) { throw "Automation failures or no tests executed: $Label" }
    exit 0
}
catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
