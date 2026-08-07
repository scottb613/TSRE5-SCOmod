[CmdletBinding()]
param(
    [string]$SourceRoot = (Join-Path $PSScriptRoot "..\TSREvcWIP"),
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\reports\qt6-audit.txt")
)

$resolvedSource = [IO.Path]::GetFullPath($SourceRoot)
if (-not (Test-Path -LiteralPath $resolvedSource)) {
    throw "Source directory not found: $resolvedSource"
}

$patterns = @(
    @{ Name = "QRegExp"; Expression = "QRegExp|QRegExpValidator" },
    @{ Name = "QDesktopWidget"; Expression = "QDesktopWidget|desktop\(\)" },
    @{ Name = "QTextStream codec"; Expression = "setCodec\s*\(" },
    @{ Name = "Removed stream device API"; Expression = "unsetDevice\s*\(" },
    @{ Name = "Removed string references"; Expression = "\bQStringRef\b|\.midRef\s*\(" },
    @{ Name = "Unqualified QTextStream endl"; Expression = "<<\s*endl\b" },
    @{ Name = "Legacy event positions"; Expression = "globalPos\s*\(|localPos\s*\(|posF\s*\(|->delta\s*\(|->orientation\s*\(|\b(?:event|e)->[xy]\s*\(" },
    @{ Name = "QGL legacy API"; Expression = "QGLWidget|QGLFormat|QGLContext" },
    @{ Name = "Removed containers"; Expression = "QLinkedList|QListIterator|QMutableListIterator" },
    @{ Name = "Qt5 compatibility"; Expression = "Core5Compat|QT_VERSION_CHECK\s*\(\s*5" }
)

$reportDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $reportDirectory | Out-Null

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add("TSREvc Qt6 migration audit")
$lines.Add("Source: $resolvedSource")
$lines.Add("Generated: $(Get-Date -Format o)")
$lines.Add("")

$rg = Get-Command rg -ErrorAction SilentlyContinue

foreach ($pattern in $patterns) {
    $lines.Add("=== $($pattern.Name) ===")

    if ($null -ne $rg) {
        $matches = & $rg.Source -n --glob "*.cpp" --glob "*.h" --glob "*.hpp" `
            --glob "*.cxx" $pattern.Expression $resolvedSource 2>$null
    } else {
        $files = Get-ChildItem -LiteralPath $resolvedSource -Recurse -File |
            Where-Object { $_.Extension -in @(".cpp", ".h", ".hpp", ".cxx") }
        $matches = $files | Select-String -Pattern $pattern.Expression |
            ForEach-Object { "$($_.Path):$($_.LineNumber):$($_.Line)" }
    }

    if ($matches) {
        foreach ($match in $matches) {
            $lines.Add([string]$match)
        }
    } else {
        $lines.Add("(none)")
    }

    $lines.Add("")
}

$lines | Set-Content -LiteralPath $OutputPath -Encoding utf8
Write-Host "Audit written to $([IO.Path]::GetFullPath($OutputPath))"
