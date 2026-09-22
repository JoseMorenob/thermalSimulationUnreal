<#
.SYNOPSIS
    Ejecuta un benchmark reproducible base/IR/IR visible y preserva la evidencia bruta.

.DESCRIPTION
    Requiere una compilacion Development (no PIE). Cada ejecucion arranca el mismo mapa,
    descarta el calentamiento y el actor ARadianceCaptureActor controla csvprofile start/stop.
    No modifica el .umap: IRBenchmarkMode se aplica solo en tiempo de ejecucion.
#>
[CmdletBinding()]
param(
    [string]$Map = '/Game/Maps/IRRadianceDemo',
    [ValidateRange(1, 20)][int]$Repetitions = 3,
    [ValidateRange(1, 600)][int]$WarmupSeconds = 10,
    [ValidateRange(5, 3600)][int]$CaptureSeconds = 30,
    [ValidateRange(320, 7680)][int]$Width = 1920,
    [ValidateRange(240, 4320)][int]$Height = 1080,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$project = Join-Path $projectRoot 'IRSimClean.uproject'
if (-not (Test-Path -LiteralPath $project)) { throw "No existe el proyecto: $project" }
if (-not (Test-Path -LiteralPath $Editor)) { throw "No existe UnrealEditor.exe: $Editor" }
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $projectRoot ("Saved\IRPerformance\{0:yyyyMMdd-HHmmss}" -f (Get-Date))
}
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$csvDirectory = Join-Path $projectRoot 'Saved\Profiling\CSV'
New-Item -ItemType Directory -Force -Path $csvDirectory | Out-Null

function Get-GpuSample {
    $tool = Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue
    if ($null -eq $tool) { return $null }
    $line = & $tool.Source '--query-gpu=utilization.gpu,memory.used' '--format=csv,noheader,nounits' 2>$null | Select-Object -First 1
    if ($line -match '^\s*([0-9.]+),\s*([0-9.]+)') {
        return [pscustomobject]@{ gpu_util_percent = [double]$Matches[1]; gpu_memory_mib = [double]$Matches[2] }
    }
    return $null
}

$metadata = [ordered]@{
    created_utc = (Get-Date).ToUniversalTime().ToString('o')
    project = $project
    map = $Map
    editor = $Editor
    repetitions = $Repetitions
    warmup_seconds = $WarmupSeconds
    capture_seconds = $CaptureSeconds
    resolution = "$Width x $Height"
    os = (Get-CimInstance Win32_OperatingSystem | ForEach-Object Caption)
    cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name)
    gpu = @((Get-CimInstance Win32_VideoController | ForEach-Object Name))
    memory_bytes = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory
    command_line_note = 'NoVSync is passed so FrameTime is not masked by the display refresh limit.'
}
$metadata | ConvertTo-Json -Depth 4 | Set-Content -Encoding utf8 (Join-Path $OutputDirectory 'environment.json')

$samples = [System.Collections.Generic.List[object]]::new()
foreach ($mode in @('base', 'radiance', 'visible')) {
    for ($run = 1; $run -le $Repetitions; $run++) {
        $before = Get-ChildItem -LiteralPath $csvDirectory -Filter '*.csv' -ErrorAction SilentlyContinue |
            Select-Object -ExpandProperty FullName
        $arguments = @(
            "`"$project`"", $Map, '-game', '-NoVSync', '-nosound', '-unattended', '-nop4',
            "-ResX=$Width", "-ResY=$Height", '-WINDOWED',
            "-IRBenchmarkMode=$mode", "-IRBenchmarkWarmup=$WarmupSeconds", "-IRBenchmarkSeconds=$CaptureSeconds"
        )
        Write-Host "[$mode $run/$Repetitions] iniciando..."
        $process = Start-Process -FilePath $Editor -ArgumentList $arguments -PassThru
        while (-not $process.HasExited) {
            Start-Sleep -Seconds 1
            $process.Refresh()
            if (-not $process.HasExited) {
                $gpu = Get-GpuSample
                $samples.Add([pscustomobject]@{
                    timestamp_utc = (Get-Date).ToUniversalTime().ToString('o')
                    configuration = $mode
                    repetition = $run
                    process_id = $process.Id
                    working_set_mib = [math]::Round($process.WorkingSet64 / 1MB, 2)
                    private_memory_mib = [math]::Round($process.PrivateMemorySize64 / 1MB, 2)
                    gpu_util_percent = if ($gpu) { $gpu.gpu_util_percent } else { $null }
                    gpu_memory_mib = if ($gpu) { $gpu.gpu_memory_mib } else { $null }
                })
            }
        }
        if ($process.ExitCode -ne 0) { throw "Unreal finalizo con codigo $($process.ExitCode) en $mode/$run." }
        $newCsv = Get-ChildItem -LiteralPath $csvDirectory -Filter '*.csv' |
            Where-Object { $_.FullName -notin $before } | Sort-Object LastWriteTime | Select-Object -Last 1
        if ($null -eq $newCsv) { throw "No se genero CSV para $mode/$run. Comprueba el log de Unreal y que csvprofile este habilitado." }
        $destination = Join-Path $OutputDirectory ("raw-{0}-{1:D2}.csv" -f $mode, $run)
        Copy-Item -LiteralPath $newCsv.FullName -Destination $destination
        Write-Host "[$mode $run/$Repetitions] evidencia: $destination"
    }
}
$samples | Export-Csv -NoTypeInformation -Encoding utf8 (Join-Path $OutputDirectory 'process-samples.csv')
& (Join-Path $PSScriptRoot 'summarize_ir_performance.ps1') -InputDirectory $OutputDirectory
Write-Host "Benchmark terminado: $OutputDirectory"
