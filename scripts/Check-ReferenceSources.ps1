[CmdletBinding()]
param(
    [string]$WorkspaceRoot = (Join-Path $PSScriptRoot "..")
)

$root = [IO.Path]::GetFullPath($WorkspaceRoot)
$references = @(
    @{ Name = "GenX v0.8 source"; Path = "..\TSRE\SCOmodWIP"; Expected = "77c69f1" },
    @{ Name = "Goku TSRE5vc"; Path = "..\TSRE\GokuMK-TSRE5vc-review"; Expected = "ad0b0dd" },
    @{ Name = "Original TSRE5"; Path = "..\TSRE\.tsre5-src"; Expected = "af99c14" },
    @{ Name = "Open Rails"; Path = "..\TSRE\.openrails-src"; Expected = "ded433da4" },
    @{ Name = "Peter Qt6 fork"; Path = "refs\pgroenbaek-tsre5-qt6"; Expected = "3c3e22a" },
    @{ Name = "Eric v8.006n Qt6"; Path = "refs\eric-tsre5-qt6"; Expected = "190b11e" }
)

$failed = $false

foreach ($reference in $references) {
    $path = [IO.Path]::GetFullPath((Join-Path $root $reference.Path))
    $gitMarker = Join-Path $path ".git"

    if (-not (Test-Path -LiteralPath $path)) {
        Write-Host "[MISSING] $($reference.Name): $path" -ForegroundColor Red
        $failed = $true
        continue
    }

    if (-not (Test-Path -LiteralPath $gitMarker)) {
        Write-Host "[NOT GIT] $($reference.Name): $path" -ForegroundColor Yellow
        $failed = $true
        continue
    }

    $commit = & git -c "safe.directory=$($path.Replace('\', '/'))" -C $path rev-parse --short HEAD 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[GIT ERROR] $($reference.Name): $path" -ForegroundColor Red
        $failed = $true
        continue
    }

    Write-Host "[FOUND] $($reference.Name): $commit" -ForegroundColor Green
    Write-Host "        $path"

    if ($null -ne $reference.Expected -and -not $commit.StartsWith($reference.Expected)) {
        Write-Host "        [PIN MISMATCH] expected $($reference.Expected)" -ForegroundColor Red
        $failed = $true
    }
}

if ($failed) {
    throw "One or more reference source trees are unavailable."
}

Write-Host "All reference source trees are available." -ForegroundColor Green
