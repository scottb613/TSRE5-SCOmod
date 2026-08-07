[CmdletBinding()]
param(
    [string]$RouteRoot =
        "M:\ORTSmini_F\SCO_CLEAN\Train Simulator\routes\SCO_LHR",
    [string]$BaselineManifest =
        (Join-Path $PSScriptRoot "..\Gate5Evidence\pre-save\SCO_LHR-before-save.csv"),
    [string]$OutputDirectory =
        (Join-Path $PSScriptRoot "..\Gate5Evidence\post-save")
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $RouteRoot -PathType Container)) {
    throw "Route folder was not found: $RouteRoot"
}
if (-not (Test-Path -LiteralPath $BaselineManifest -PathType Leaf)) {
    throw "Baseline manifest was not found: $BaselineManifest"
}

$routePath = (Resolve-Path -LiteralPath $RouteRoot).Path.TrimEnd("\")
$baselineRows = Import-Csv -LiteralPath $BaselineManifest
$baselineByPath = @{}
foreach ($row in $baselineRows) {
    $baselineByPath[$row.RelativePath.ToLowerInvariant()] = $row
}

$currentByPath = @{}
foreach ($file in Get-ChildItem -LiteralPath $routePath -File -Recurse |
        Sort-Object FullName) {
    $relativePath = $file.FullName.Substring($routePath.Length).TrimStart("\")
    $currentByPath[$relativePath.ToLowerInvariant()] = [pscustomobject]@{
        RelativePath = $relativePath
        Length = $file.Length
        LastWriteTimeUtc = $file.LastWriteTimeUtc.ToString("o")
        SHA256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash
    }
}

$allKeys = @($baselineByPath.Keys + $currentByPath.Keys |
    Sort-Object -Unique)
$results = foreach ($key in $allKeys) {
    $before = $baselineByPath[$key]
    $after = $currentByPath[$key]
    $status = if ($null -eq $before) {
        "Added"
    } elseif ($null -eq $after) {
        "Removed"
    } elseif ($before.SHA256 -ne $after.SHA256) {
        "ContentChanged"
    } elseif ($before.LastWriteTimeUtc -ne $after.LastWriteTimeUtc) {
        "TimestampOnly"
    } else {
        "Unchanged"
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
        BeforeTimeUtc = if ($null -ne $before) {
            $before.LastWriteTimeUtc
        } else {
            ""
        }
        AfterTimeUtc = if ($null -ne $after) {
            $after.LastWriteTimeUtc
        } else {
            ""
        }
    }
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$reportPath = Join-Path $OutputDirectory "SCO_LHR-comparison-$timestamp.csv"
$results | Export-Csv -LiteralPath $reportPath -NoTypeInformation -Encoding UTF8

$summary = $results | Group-Object Status | Sort-Object Name |
    Select-Object Name, Count
$summary | Format-Table -AutoSize
Write-Host ""
Write-Host "Detailed report: $reportPath"

$materialChanges = @($results | Where-Object {
    $_.Status -in @("Added", "Removed", "ContentChanged")
})
if ($materialChanges.Count -gt 0) {
    Write-Host ""
    Write-Host "Material file changes:"
    $materialChanges |
        Select-Object Status, RelativePath, BeforeLength, AfterLength |
        Format-Table -AutoSize
}
