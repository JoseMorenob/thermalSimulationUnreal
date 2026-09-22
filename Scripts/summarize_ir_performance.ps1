[CmdletBinding()]
param([Parameter(Mandatory)][string]$InputDirectory)

$ErrorActionPreference = 'Stop'
$InputDirectory = (Resolve-Path $InputDirectory).Path
function Number([object]$value) {
    $number = 0.0
    $text = [string]$value
    if ([double]::TryParse($text, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::CurrentCulture, [ref]$number)) { return $number }
    if ([double]::TryParse($text, [Globalization.NumberStyles]::Float,
        [Globalization.CultureInfo]::InvariantCulture, [ref]$number)) { return $number }
    return $null
}
function Quantile([double[]]$values, [double]$q) {
    if ($values.Count -eq 0) { return $null }
    $sorted = @($values | Sort-Object)
    $index = [math]::Min([math]::Ceiling($q * $sorted.Count) - 1, $sorted.Count - 1)
    return $sorted[$index]
}
function ColumnValues($rows, [string[]]$names) {
    foreach ($name in $names) {
        if ($rows.Count -gt 0 -and $rows[0].PSObject.Properties.Name -contains $name) {
            return @($rows | ForEach-Object { Number $_.$name } | Where-Object { $null -ne $_ })
        }
    }
    return @()
}

$summary = [System.Collections.Generic.List[object]]::new()
Get-ChildItem -LiteralPath $InputDirectory -Filter 'raw-*.csv' | Sort-Object Name | ForEach-Object {
    $parts = $_.BaseName -split '-'
    $configuration, $repetition = $parts[1], [int]$parts[2]
    $lines = Get-Content -LiteralPath $_.FullName
    $headerIndex = [array]::FindIndex([string[]]$lines, [Predicate[string]]{ param($line) $line -match '(^|,)"?FrameTime"?(,|$)' })
    if ($headerIndex -lt 0) { throw "No se encontro la cabecera FrameTime en $($_.Name)." }
    $rows = @($lines[$headerIndex..($lines.Count - 1)] | ConvertFrom-Csv)
    $frame = [double[]](ColumnValues $rows @('FrameTime'))
    $game = [double[]](ColumnValues $rows @('GameThreadTime', 'GameThread'))
    $render = [double[]](ColumnValues $rows @('RenderThreadTime', 'RenderThread'))
    $gpu = [double[]](ColumnValues $rows @('GPUFrameTime', 'GPUTime'))
    $vram = [double[]](ColumnValues $rows @('GPUMem/LocalUsedMB'))
    if ($frame.Count -eq 0) { throw "FrameTime no contiene muestras numericas en $($_.Name)." }
    $summary.Add([pscustomobject]@{
        configuration = $configuration; repetition = $repetition; source_csv = $_.Name; frames = $frame.Count
        frame_ms_median = [math]::Round((Quantile $frame 0.50), 3)
        frame_ms_p95 = [math]::Round((Quantile $frame 0.95), 3)
        fps_median = [math]::Round(1000.0 / (Quantile $frame 0.50), 2)
        fps_1_percent_low = [math]::Round(1000.0 / (Quantile $frame 0.99), 2)
        game_ms_median = if ($game.Count) { [math]::Round((Quantile $game 0.50), 3) } else { $null }
        render_ms_median = if ($render.Count) { [math]::Round((Quantile $render 0.50), 3) } else { $null }
        gpu_ms_median = if ($gpu.Count) { [math]::Round((Quantile $gpu 0.50), 3) } else { $null }
        vram_local_used_mib_median = if ($vram.Count) { [math]::Round((Quantile $vram 0.50), 2) } else { $null }
    })
}
$summary | Export-Csv -NoTypeInformation -Encoding utf8 (Join-Path $InputDirectory 'frame-summary-per-run.csv')

$process = @()
$samplePath = Join-Path $InputDirectory 'process-samples.csv'
if (Test-Path $samplePath) { $process = @(Import-Csv $samplePath) }
$aggregate = foreach ($group in ($summary | Group-Object configuration)) {
    $runs = @($group.Group)
    $base = [pscustomobject]@{ configuration = $group.Name; repetitions = $runs.Count; frames_total = ($runs | Measure-Object frames -Sum).Sum
        frame_ms_median = [math]::Round((Quantile ([double[]]$runs.frame_ms_median) 0.5), 3)
        fps_median = [math]::Round((Quantile ([double[]]$runs.fps_median) 0.5), 2)
        fps_1_percent_low = [math]::Round((Quantile ([double[]]$runs.fps_1_percent_low) 0.5), 2)
        gpu_ms_median = if (@($runs | Where-Object { $null -ne $_.gpu_ms_median }).Count) { [math]::Round((Quantile ([double[]]$runs.gpu_ms_median) 0.5), 3) } else { $null }
        vram_local_used_mib_median = if (@($runs | Where-Object { $null -ne $_.vram_local_used_mib_median }).Count) { [math]::Round((Quantile ([double[]]$runs.vram_local_used_mib_median) 0.5), 2) } else { $null }
        working_set_mib_median = $null; gpu_memory_mib_median = $null }
    $processForConfiguration = @($process | Where-Object configuration -eq $group.Name)
    if ($processForConfiguration.Count) {
        $base.working_set_mib_median = [math]::Round((Quantile ([double[]]($processForConfiguration | ForEach-Object { Number $_.working_set_mib })) 0.5), 2)
        $gpuMem = [double[]]($processForConfiguration | ForEach-Object { Number $_.gpu_memory_mib } | Where-Object { $null -ne $_ })
        if ($gpuMem.Count) { $base.gpu_memory_mib_median = [math]::Round((Quantile $gpuMem 0.5), 2) }
    }
    $base
}
$baseRow = @($aggregate | Where-Object configuration -eq 'base' | Select-Object -First 1)
foreach ($row in $aggregate) {
    $row | Add-Member NoteProperty frame_ms_overhead_vs_base $(if ($baseRow) { [math]::Round($row.frame_ms_median - $baseRow.frame_ms_median, 3) } else { $null })
    $row | Add-Member NoteProperty gpu_ms_overhead_vs_base $(if ($baseRow -and $null -ne $row.gpu_ms_median -and $null -ne $baseRow.gpu_ms_median) { [math]::Round($row.gpu_ms_median - $baseRow.gpu_ms_median, 3) } else { $null })
}
$aggregate | Export-Csv -NoTypeInformation -Encoding utf8 (Join-Path $InputDirectory 'summary.csv')
Write-Host "Resumen creado: $(Join-Path $InputDirectory 'summary.csv')"
