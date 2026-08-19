[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Capture", "Compare")]
    [string]$Action,

    [Parameter(Mandatory = $true)]
    [string]$Scenario,

    [Parameter(Mandatory = $true)]
    [string]$RouteRoot,

    [string]$EvidenceRoot =
        (Join-Path $PSScriptRoot "..\.route-regression-evidence"),

    [string]$LogDirectory = $EvidenceRoot,
    [ValidateSet(
        "NoMaterialChange", "UndoRestoresData", "AllowChanges",
        "PathEdit", "TrackDbEdit", "RoadDbEdit"
    )]
    [string]$Expectation = "AllowChanges",

    [string]$RunId
)

$ErrorActionPreference = "Stop"

function Get-SafeName {
    param([string]$Value)
    $safe = $Value -replace '[^A-Za-z0-9._-]', '-'
    $safe = $safe.Trim('-')
    if ([string]::IsNullOrWhiteSpace($safe)) {
        throw "Scenario must contain at least one letter or number."
    }
    return $safe
}

function Get-RelativePath {
    param([string]$Root, [string]$Path)
    return $Path.Substring($Root.Length).TrimStart("\")
}

function Get-SHA256Hash {
    param([string]$Path)

    $stream = [IO.File]::OpenRead($Path)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($stream))).Replace("-", "")
    } finally {
        $algorithm.Dispose()
        $stream.Dispose()
    }
}

function Get-RouteManifest {
    param([string]$Root)
    foreach ($file in Get-ChildItem -LiteralPath $Root -Recurse -File |
            Sort-Object FullName) {
        [pscustomobject]@{
            RelativePath = Get-RelativePath -Root $Root -Path $file.FullName
            Length = $file.Length
            LastWriteTimeUtc = $file.LastWriteTimeUtc.ToString("o")
            SHA256 = Get-SHA256Hash -Path $file.FullName
        }
    }
}

function Copy-MutableRouteData {
    param([string]$Root, [string]$Destination)

    $mutableDirectories = @("world", "tiles", "lo_tiles", "paths")
    foreach ($directory in $mutableDirectories) {
        $sourceDirectory = Join-Path $Root $directory
        if (Test-Path -LiteralPath $sourceDirectory -PathType Container) {
            Copy-Item -LiteralPath $sourceDirectory `
                -Destination $Destination -Recurse
        }
    }

    $rootPatterns = @(
        "*.tdb", "*.rdb", "*.tit", "*.rit", "*.trk",
        "tsection.dat", "carspawn.dat", "forests.dat", "sigcfg.dat",
        "sigscr.dat", "speedpost.dat", "ssource.dat", "ttype.dat"
    )
    $rootFiles = foreach ($pattern in $rootPatterns) {
        Get-ChildItem -LiteralPath $Root -File -Filter $pattern `
            -ErrorAction SilentlyContinue
    }
    foreach ($file in $rootFiles | Sort-Object FullName -Unique) {
        Copy-Item -LiteralPath $file.FullName -Destination $Destination
    }
}

function Get-Comparison {
    param([array]$BeforeRows, [array]$AfterRows)

    $beforeByPath = @{}
    foreach ($row in $BeforeRows) {
        $beforeByPath[$row.RelativePath.ToLowerInvariant()] = $row
    }
    $afterByPath = @{}
    foreach ($row in $AfterRows) {
        $afterByPath[$row.RelativePath.ToLowerInvariant()] = $row
    }

    $keys = @($beforeByPath.Keys + $afterByPath.Keys | Sort-Object -Unique)
    foreach ($key in $keys) {
        $before = $beforeByPath[$key]
        $after = $afterByPath[$key]
        $status = "Unchanged"
        if ($null -eq $before) {
            $status = "Added"
        } elseif ($null -eq $after) {
            $status = "Removed"
        } elseif ($before.SHA256 -ne $after.SHA256) {
            $status = "ContentChanged"
        } elseif ($before.LastWriteTimeUtc -ne $after.LastWriteTimeUtc) {
            $status = "TimestampOnly"
        }

        [pscustomobject]@{
            Status = $status
            RelativePath = if ($null -ne $after) {
                $after.RelativePath
            } else {
                $before.RelativePath
            }
            BeforeLength = if ($null -ne $before) { $before.Length } else { "" }
            AfterLength = if ($null -ne $after) { $after.Length } else { "" }
            BeforeSHA256 = if ($null -ne $before) { $before.SHA256 } else { "" }
            AfterSHA256 = if ($null -ne $after) { $after.SHA256 } else { "" }
        }
    }
}

function Get-BinaryDifference {
    param([string]$BeforePath, [string]$AfterPath)

    $before = [IO.File]::ReadAllBytes($BeforePath)
    $after = [IO.File]::ReadAllBytes($AfterPath)
    $sharedLength = [Math]::Min($before.Length, $after.Length)
    $changed = 0
    $firstOffset = -1
    $lastOffset = -1
    for ($index = 0; $index -lt $sharedLength; ++$index) {
        if ($before[$index] -ne $after[$index]) {
            ++$changed
            if ($firstOffset -lt 0) {
                $firstOffset = $index
            }
            $lastOffset = $index
        }
    }
    $changed += [Math]::Abs($before.Length - $after.Length)

    return [pscustomobject]@{
        ChangedBytes = $changed
        FirstChangedOffset = $firstOffset
        LastChangedOffset = $lastOffset
    }
}

function Get-RawHeightDifference {
    param([string]$BeforePath, [string]$AfterPath)

    $before = [IO.File]::ReadAllBytes($BeforePath)
    $after = [IO.File]::ReadAllBytes($AfterPath)
    if (($before.Length % 2) -ne 0 -or ($after.Length % 2) -ne 0) {
        throw "Terrain RAW files must contain 16-bit samples."
    }

    $sampleCount = [Math]::Min($before.Length, $after.Length) / 2
    $changed = 0
    $maxDelta = 0
    $sumDelta = [long]0
    for ($sample = 0; $sample -lt $sampleCount; ++$sample) {
        $offset = $sample * 2
        $beforeValue = [BitConverter]::ToUInt16($before, $offset)
        $afterValue = [BitConverter]::ToUInt16($after, $offset)
        $delta = [Math]::Abs([int]$afterValue - [int]$beforeValue)
        if ($delta -gt 0) {
            ++$changed
            $sumDelta += $delta
            if ($delta -gt $maxDelta) {
                $maxDelta = $delta
            }
        }
    }

    return [pscustomobject]@{
        SamplesCompared = $sampleCount
        ChangedSamples = $changed
        MaxRawDelta = $maxDelta
        SumAbsoluteRawDelta = $sumDelta
        LengthChanged = $before.Length -ne $after.Length
    }
}

function Get-NormalizedTextHash {
    param([string]$Text)
    $normalized = ($Text -replace '\s+', ' ').Trim()
    $bytes = [Text.Encoding]::UTF8.GetBytes($normalized)
    $algorithm = [Security.Cryptography.SHA256]::Create()
    try {
        return ([BitConverter]::ToString(
            $algorithm.ComputeHash($bytes))).Replace("-", "")
    } finally {
        $algorithm.Dispose()
    }
}

function Get-DeclaredCount {
    param([string]$Text, [string]$Token)
    $match = [regex]::Match(
        $Text, "(?is)\b$([regex]::Escape($Token))\s*\(\s*(\d+)")
    if (-not $match.Success) {
        return 0
    }
    return [int]$match.Groups[1].Value
}

function Get-SimisaSummary {
    param([string]$Path)

    $text = Get-Content -LiteralPath $Path -Raw
    $extension = [IO.Path]::GetExtension($Path).ToLowerInvariant()
    $reversePathNodes = 0
    $waitPathNodes = 0
    foreach ($match in [regex]::Matches(
            $text, '(?im)^\s*TrPathNode\s*\(\s*([0-9A-Fa-f]{8})')) {
        $flags = [Convert]::ToUInt32($match.Groups[1].Value, 16)
        if (($flags -band 1) -ne 0) {
            ++$reversePathNodes
        }
        if (($flags -band 2) -ne 0) {
            ++$waitPathNodes
        }
    }
    return [pscustomobject]@{
        NormalizedSHA256 = Get-NormalizedTextHash -Text $text
        TrackNodesDeclared = Get-DeclaredCount -Text $text -Token "TrackNodes"
        TrackNodeRecords = [regex]::Matches(
            $text, '(?im)^\s*TrackNode\s*\(').Count
        VectorNodeRecords = [regex]::Matches(
            $text, '(?im)^\s*TrVectorNode\s*\(').Count
        JunctionNodeRecords = [regex]::Matches(
            $text, '(?im)^\s*TrJunctionNode\s*\(').Count
        TrItemsDeclared = Get-DeclaredCount -Text $text -Token "TrItemTable"
        TrItemRecords = [regex]::Matches(
            $text, '(?im)^\s*[A-Za-z0-9_]+Item\s*\(').Count
        PathPdpRecords = [regex]::Matches(
            $text, '(?im)^\s*TrackPDP\s*\(').Count
        PathNodesDeclared = Get-DeclaredCount -Text $text -Token "TrPathNodes"
        PathNodeRecords = [regex]::Matches(
            $text, '(?im)^\s*TrPathNode\s*\(').Count
        ReversePathNodes = $reversePathNodes
        WaitPathNodes = $waitPathNodes
        PathStart = ([regex]::Match(
            $text, '(?is)\bTrPathStart\s*\(\s*"([^"]*)"')).Groups[1].Value
        PathEnd = ([regex]::Match(
            $text, '(?is)\bTrPathEnd\s*\(\s*"([^"]*)"')).Groups[1].Value
        Extension = $extension
    }
}

function Get-LogSummary {
    param([string]$Directory, [datetime]$SinceUtc)

    $lines = @()
    if (Test-Path -LiteralPath $Directory -PathType Container) {
        $logs = Get-ChildItem -LiteralPath $Directory -Filter "tsre-log-*.txt" `
            -File | Where-Object { $_.LastWriteTimeUtc -ge $SinceUtc }
        foreach ($log in $logs) {
            $lines += Get-Content -LiteralPath $log.FullName
        }
    }

    return [pscustomobject]@{
        Files = @($logs).Count
        UndoBegin = @($lines | Where-Object { $_ -match '\] undo begin$' }).Count
        Undo = @($lines | Where-Object { $_ -match '\] undo$' }).Count
        Saves = @($lines | Where-Object { $_ -match ': save$' }).Count
        Warnings = @($lines | Where-Object { $_ -match '^\[W\]' }).Count
        Critical = @($lines | Where-Object {
            $_ -match '^\[(C|F)\]' -or $_ -match '(crash|assert|fatal)'
        }).Count
    }
}

$routePath = (Resolve-Path -LiteralPath $RouteRoot).Path.TrimEnd("\")
$safeScenario = Get-SafeName -Value $Scenario
$scenarioRoot = Join-Path $EvidenceRoot $safeScenario
New-Item -ItemType Directory -Path $scenarioRoot -Force | Out-Null

if ($Action -eq "Capture") {
    if ([string]::IsNullOrWhiteSpace($RunId)) {
        $RunId = Get-Date -Format "yyyyMMdd-HHmmss"
    }
    $runDirectory = Join-Path $scenarioRoot $RunId
    if (Test-Path -LiteralPath $runDirectory) {
        throw "Regression run already exists: $runDirectory"
    }

    $baselineDirectory = Join-Path $runDirectory "baseline"
    New-Item -ItemType Directory -Path $baselineDirectory -Force | Out-Null
    $capturedUtc = [datetime]::UtcNow
    Get-RouteManifest -Root $routePath |
        Export-Csv -LiteralPath (Join-Path $runDirectory "before.csv") `
            -NoTypeInformation -Encoding UTF8
    Copy-MutableRouteData -Root $routePath -Destination $baselineDirectory

    [pscustomobject]@{
        Scenario = $Scenario
        RunId = $RunId
        RouteRoot = $routePath
        CapturedUtc = $capturedUtc.ToString("o")
        Expectation = $Expectation
    } | ConvertTo-Json | Set-Content `
        -LiteralPath (Join-Path $runDirectory "run.json") -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $scenarioRoot "latest.txt") `
        -Value $RunId -Encoding ASCII

    Write-Host "Captured regression baseline."
    Write-Host "Scenario: $Scenario"
    Write-Host "Run ID:   $RunId"
    Write-Host "Evidence: $runDirectory"
    return
}

if ([string]::IsNullOrWhiteSpace($RunId)) {
    $latestPath = Join-Path $scenarioRoot "latest.txt"
    if (-not (Test-Path -LiteralPath $latestPath -PathType Leaf)) {
        throw "No captured run was found for scenario '$Scenario'."
    }
    $RunId = (Get-Content -LiteralPath $latestPath -Raw).Trim()
}

$runDirectory = Join-Path $scenarioRoot $RunId
$runInfoPath = Join-Path $runDirectory "run.json"
if (-not (Test-Path -LiteralPath $runInfoPath -PathType Leaf)) {
    throw "Run metadata was not found: $runInfoPath"
}
$runInfo = Get-Content -LiteralPath $runInfoPath -Raw | ConvertFrom-Json
if ($runInfo.RouteRoot -ne $routePath) {
    throw "Captured route '$($runInfo.RouteRoot)' does not match '$routePath'."
}
if (-not $PSBoundParameters.ContainsKey("Expectation")) {
    $Expectation = $runInfo.Expectation
}

$beforeRows = @(Import-Csv -LiteralPath (Join-Path $runDirectory "before.csv"))
$afterRows = @(Get-RouteManifest -Root $routePath)
$comparison = @(Get-Comparison -BeforeRows $beforeRows -AfterRows $afterRows)
$comparison | Export-Csv -LiteralPath (Join-Path $runDirectory "comparison.csv") `
    -NoTypeInformation -Encoding UTF8

$terrainResults = @()
$binaryResults = @()
$simisaResults = @()
$baselineDirectory = Join-Path $runDirectory "baseline"
foreach ($change in $comparison | Where-Object {
        $_.Status -eq "ContentChanged"
    }) {
    $beforePath = Join-Path $baselineDirectory $change.RelativePath
    $afterPath = Join-Path $routePath $change.RelativePath
    if (-not (Test-Path -LiteralPath $beforePath -PathType Leaf)) {
        continue
    }

    if ($change.RelativePath -match '_y\.raw$') {
        $result = Get-RawHeightDifference `
            -BeforePath $beforePath -AfterPath $afterPath
        $terrainResults += [pscustomobject]@{
            RelativePath = $change.RelativePath
            SamplesCompared = $result.SamplesCompared
            ChangedSamples = $result.ChangedSamples
            MaxRawDelta = $result.MaxRawDelta
            SumAbsoluteRawDelta = $result.SumAbsoluteRawDelta
            LengthChanged = $result.LengthChanged
        }
    } else {
        $result = Get-BinaryDifference `
            -BeforePath $beforePath -AfterPath $afterPath
        $binaryResults += [pscustomobject]@{
            RelativePath = $change.RelativePath
            ChangedBytes = $result.ChangedBytes
            FirstChangedOffset = $result.FirstChangedOffset
            LastChangedOffset = $result.LastChangedOffset
        }
    }

    if ($change.RelativePath -match '\.(pat|tdb|rdb|tit|rit|trk)$') {
        $beforeSummary = Get-SimisaSummary -Path $beforePath
        $afterSummary = Get-SimisaSummary -Path $afterPath
        $simisaResults += [pscustomobject]@{
            RelativePath = $change.RelativePath
            BeforeNormalizedSHA256 = $beforeSummary.NormalizedSHA256
            AfterNormalizedSHA256 = $afterSummary.NormalizedSHA256
            NormalizedTextChanged =
                $beforeSummary.NormalizedSHA256 -ne
                $afterSummary.NormalizedSHA256
            BeforeTrackNodes = $beforeSummary.TrackNodesDeclared
            AfterTrackNodes = $afterSummary.TrackNodesDeclared
            BeforeTrackNodeRecords = $beforeSummary.TrackNodeRecords
            AfterTrackNodeRecords = $afterSummary.TrackNodeRecords
            BeforeVectorNodes = $beforeSummary.VectorNodeRecords
            AfterVectorNodes = $afterSummary.VectorNodeRecords
            BeforeJunctionNodes = $beforeSummary.JunctionNodeRecords
            AfterJunctionNodes = $afterSummary.JunctionNodeRecords
            BeforeTrItems = $beforeSummary.TrItemsDeclared
            AfterTrItems = $afterSummary.TrItemsDeclared
            BeforeTrItemRecords = $beforeSummary.TrItemRecords
            AfterTrItemRecords = $afterSummary.TrItemRecords
            BeforePathPDPs = $beforeSummary.PathPdpRecords
            AfterPathPDPs = $afterSummary.PathPdpRecords
            BeforePathNodes = $beforeSummary.PathNodesDeclared
            AfterPathNodes = $afterSummary.PathNodesDeclared
            BeforePathNodeRecords = $beforeSummary.PathNodeRecords
            AfterPathNodeRecords = $afterSummary.PathNodeRecords
            BeforeReversePathNodes = $beforeSummary.ReversePathNodes
            AfterReversePathNodes = $afterSummary.ReversePathNodes
            BeforeWaitPathNodes = $beforeSummary.WaitPathNodes
            AfterWaitPathNodes = $afterSummary.WaitPathNodes
            BeforePathStart = $beforeSummary.PathStart
            AfterPathStart = $afterSummary.PathStart
            BeforePathEnd = $beforeSummary.PathEnd
            AfterPathEnd = $afterSummary.PathEnd
        }
    }
}

$terrainResults | Export-Csv `
    -LiteralPath (Join-Path $runDirectory "terrain-differences.csv") `
    -NoTypeInformation -Encoding UTF8
$binaryResults | Export-Csv `
    -LiteralPath (Join-Path $runDirectory "binary-differences.csv") `
    -NoTypeInformation -Encoding UTF8
$simisaResults | Export-Csv `
    -LiteralPath (Join-Path $runDirectory "simisa-summaries.csv") `
    -NoTypeInformation -Encoding UTF8

$capturedUtc = [datetime]::Parse(
    $runInfo.CapturedUtc, [Globalization.CultureInfo]::InvariantCulture,
    [Globalization.DateTimeStyles]::RoundtripKind)
$logSummary = Get-LogSummary -Directory $LogDirectory -SinceUtc $capturedUtc
$material = @($comparison | Where-Object {
    $_.Status -in @("Added", "Removed", "ContentChanged")
})
$timestampOnly = @($comparison | Where-Object {
    $_.Status -eq "TimestampOnly"
})

$failures = @()
$warnings = @()
if ($logSummary.Critical -gt 0) {
    $failures += "Runtime logs contain $($logSummary.Critical) critical line(s)."
}
if ($Expectation -eq "NoMaterialChange" -and $material.Count -gt 0) {
    $failures += "Expected no material changes; found $($material.Count)."
}
if ($Expectation -eq "UndoRestoresData") {
    $disallowed = @($material | Where-Object {
        $_.RelativePath -notmatch '\.t$'
    })
    if ($disallowed.Count -gt 0) {
        $failures +=
            "Undo left $($disallowed.Count) material non-.t file change(s)."
    }
    $changedMetadata = @($material | Where-Object {
        $_.RelativePath -match '\.t$'
    })
    if ($changedMetadata.Count -gt 0) {
        $warnings +=
            "Undo restored data but rewrote $($changedMetadata.Count) terrain metadata file(s)."
    }
    if ($logSummary.UndoBegin -ne $logSummary.Undo) {
        $failures +=
            "Undo log imbalance: $($logSummary.UndoBegin) begin, $($logSummary.Undo) undo."
    }
}
if ($Expectation -eq "PathEdit") {
    $pathChanges = @($material | Where-Object {
        $_.RelativePath -match '^paths\\.+\.pat$'
    })
    $unexpected = @($material | Where-Object {
        $_.RelativePath -notmatch '^paths\\.+\.pat$'
    })
    if ($pathChanges.Count -eq 0) {
        $failures += "Path Editor scenario did not change or add a .pat file."
    }
    if ($unexpected.Count -gt 0) {
        $failures +=
            "Path Editor scenario changed $($unexpected.Count) non-path file(s)."
    }
}
if ($Expectation -eq "TrackDbEdit") {
    $trackChanges = @($material | Where-Object {
        $_.RelativePath -match '\.(tdb|tit)$'
    })
    $roadChanges = @($material | Where-Object {
        $_.RelativePath -match '\.(rdb|rit)$'
    })
    if ($trackChanges.Count -eq 0) {
        $failures += "TrackDB scenario did not change TrackDB or its item table."
    }
    if ($roadChanges.Count -gt 0) {
        $failures +=
            "TrackDB scenario unexpectedly changed RoadDB or its item table."
    }
}
if ($Expectation -eq "RoadDbEdit") {
    $roadChanges = @($material | Where-Object {
        $_.RelativePath -match '\.(rdb|rit)$'
    })
    $trackChanges = @($material | Where-Object {
        $_.RelativePath -match '\.(tdb|tit)$'
    })
    if ($roadChanges.Count -eq 0) {
        $failures += "RoadDB scenario did not change RoadDB or its item table."
    }
    $unexpectedTrackChanges = @()
    foreach ($trackChange in $trackChanges) {
        if ($trackChange.RelativePath -notmatch '\.tdb$') {
            $unexpectedTrackChanges += $trackChange
            continue
        }
        $binary = $binaryResults | Where-Object {
            $_.RelativePath -eq $trackChange.RelativePath
        } | Select-Object -First 1
        $simisa = $simisaResults | Where-Object {
            $_.RelativePath -eq $trackChange.RelativePath
        } | Select-Object -First 1
        $stableStructure = $null -ne $simisa -and
            $simisa.BeforeTrackNodes -eq $simisa.AfterTrackNodes -and
            $simisa.BeforeTrackNodeRecords -eq $simisa.AfterTrackNodeRecords -and
            $simisa.BeforeVectorNodes -eq $simisa.AfterVectorNodes -and
            $simisa.BeforeJunctionNodes -eq $simisa.AfterJunctionNodes -and
            $simisa.BeforeTrItems -eq $simisa.AfterTrItems -and
            $simisa.BeforeTrItemRecords -eq $simisa.AfterTrItemRecords
        if (-not ($stableStructure -and $null -ne $binary -and
                [int]$binary.ChangedBytes -le 16)) {
            $unexpectedTrackChanges += $trackChange
        }
    }
    if ($trackChanges.Count -gt $unexpectedTrackChanges.Count) {
        $warnings +=
            "Ignored the known small TrackDB save-normalization rewrite."
    }
    if ($unexpectedTrackChanges.Count -gt 0) {
        $failures +=
            "RoadDB scenario unexpectedly changed TrackDB or its item table."
    }
}

$outcome = if ($failures.Count -eq 0) { "PASS" } else { "FAIL" }
$summary = [pscustomobject]@{
    Outcome = $outcome
    Scenario = $Scenario
    RunId = $RunId
    Expectation = $Expectation
    RouteFiles = $afterRows.Count
    MaterialChanges = $material.Count
    TimestampOnly = $timestampOnly.Count
    TerrainFilesAnalyzed = $terrainResults.Count
    BinaryFilesAnalyzed = $binaryResults.Count
    SimisaFilesAnalyzed = $simisaResults.Count
    LogFiles = $logSummary.Files
    UndoBegin = $logSummary.UndoBegin
    Undo = $logSummary.Undo
    Saves = $logSummary.Saves
    WarningsInLogs = $logSummary.Warnings
    CriticalInLogs = $logSummary.Critical
    Findings = @($failures + $warnings)
}
$summary | ConvertTo-Json -Depth 4 | Set-Content `
    -LiteralPath (Join-Path $runDirectory "summary.json") -Encoding UTF8

Write-Host ""
Write-Host "$outcome - $Scenario [$RunId]"
$comparison | Group-Object Status | Sort-Object Name |
    Select-Object Name, Count | Format-Table -AutoSize
if ($terrainResults.Count -gt 0) {
    Write-Host "Terrain sample analysis:"
    $terrainResults | Format-Table -AutoSize
}
if ($simisaResults.Count -gt 0) {
    Write-Host "Track, road, item-table, and path summaries:"
    $simisaResults |
        Select-Object RelativePath, BeforeTrackNodes, AfterTrackNodes,
            BeforeTrItems, AfterTrItems, BeforePathNodes, AfterPathNodes,
            BeforeReversePathNodes, AfterReversePathNodes,
            BeforeWaitPathNodes, AfterWaitPathNodes,
            NormalizedTextChanged |
        Format-Table -AutoSize
}
Write-Host "Undo states: $($logSummary.UndoBegin) begin / $($logSummary.Undo) undo"
Write-Host "Saves:       $($logSummary.Saves)"
foreach ($finding in $summary.Findings) {
    Write-Host "- $finding"
}
Write-Host "Evidence:    $runDirectory"

if ($outcome -eq "FAIL") {
    exit 2
}
