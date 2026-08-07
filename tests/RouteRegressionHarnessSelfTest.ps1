[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Harness,

    [Parameter(Mandatory = $true)]
    [string]$WorkingDirectory
)

$ErrorActionPreference = "Stop"

if (Test-Path -LiteralPath $WorkingDirectory) {
    Remove-Item -LiteralPath $WorkingDirectory -Recurse -Force
}
$route = Join-Path $WorkingDirectory "route"
$evidence = Join-Path $WorkingDirectory "evidence"
$logs = Join-Path $WorkingDirectory "logs"
New-Item -ItemType Directory -Path `
    (Join-Path $route "world"), (Join-Path $route "tiles"),
    (Join-Path $route "lo_tiles"), (Join-Path $route "paths"), $logs `
    -Force | Out-Null

[IO.File]::WriteAllBytes(
    (Join-Path $route "tiles\fixture_y.raw"),
    [byte[]](0, 0, 1, 0, 2, 0, 3, 0))
Set-Content -LiteralPath (Join-Path $route "tiles\fixture.t") `
    -Value "terrain metadata" -Encoding ASCII
Set-Content -LiteralPath (Join-Path $route "world\fixture.w") `
    -Value "world record" -Encoding Unicode
Set-Content -LiteralPath (Join-Path $route "fixture.tdb") `
    -Value @"
SIMISA@@@@@@@@@@JINX0T0t______
TrackDB (
    TrackNodes ( 1
        TrackNode ( 1 )
    )
)
"@ -Encoding Unicode

& $Harness -Action Capture -Scenario "self-test" -RouteRoot $route `
    -EvidenceRoot $evidence -LogDirectory $logs `
    -Expectation NoMaterialChange -RunId "unchanged"
& $Harness -Action Compare -Scenario "self-test" -RouteRoot $route `
    -EvidenceRoot $evidence -LogDirectory $logs -RunId "unchanged"
$unchangedSummary = Get-Content -LiteralPath `
    (Join-Path $evidence "self-test\unchanged\summary.json") -Raw |
    ConvertFrom-Json
if ($unchangedSummary.Outcome -ne "PASS" `
        -or $unchangedSummary.MaterialChanges -ne 0) {
    throw "Unchanged fixture was not reported as an exact pass."
}

& $Harness -Action Capture -Scenario "self-test" -RouteRoot $route `
    -EvidenceRoot $evidence -LogDirectory $logs `
    -Expectation AllowChanges -RunId "terrain-change"
$rawPath = Join-Path $route "tiles\fixture_y.raw"
$raw = [IO.File]::ReadAllBytes($rawPath)
$raw[4] = 9
[IO.File]::WriteAllBytes($rawPath, $raw)
$tdbPath = Join-Path $route "fixture.tdb"
$tdbText = Get-Content -LiteralPath $tdbPath -Raw
$tdbText = $tdbText.Replace(
    "TrackNodes ( 1",
    "TrackNodes ( 2").Replace(
    "TrackNode ( 1 )",
    "TrackNode ( 1 )`r`n        TrackNode ( 2 )")
Set-Content -LiteralPath $tdbPath -Value $tdbText -Encoding Unicode
& $Harness -Action Compare -Scenario "self-test" -RouteRoot $route `
    -EvidenceRoot $evidence -LogDirectory $logs -RunId "terrain-change"
$terrainCsv = @(Import-Csv -LiteralPath `
    (Join-Path $evidence "self-test\terrain-change\terrain-differences.csv"))
if ($terrainCsv.Count -ne 1 `
        -or [int]$terrainCsv[0].ChangedSamples -ne 1 `
        -or [int]$terrainCsv[0].MaxRawDelta -ne 7) {
    throw "Terrain sample difference was not measured correctly."
}
$simisaCsv = @(Import-Csv -LiteralPath `
    (Join-Path $evidence "self-test\terrain-change\simisa-summaries.csv"))
$tdbSummary = @($simisaCsv | Where-Object {
    $_.RelativePath -eq "fixture.tdb"
})
if ($tdbSummary.Count -ne 1 `
        -or [int]$tdbSummary[0].BeforeTrackNodes -ne 1 `
        -or [int]$tdbSummary[0].AfterTrackNodes -ne 2 `
        -or [int]$tdbSummary[0].BeforeTrackNodeRecords -ne 1 `
        -or [int]$tdbSummary[0].AfterTrackNodeRecords -ne 2) {
    throw "TrackDB structural summary was not measured correctly."
}

Remove-Item -LiteralPath $WorkingDirectory -Recurse -Force
Write-Host "Route regression harness self-test passed."
