[CmdletBinding()]
param(
    [ValidateSet("MinGW", "VS2026", "All")]
    [string]$Profile = "MinGW"
)

$failed = $false

$presetPath = Join-Path $PSScriptRoot "..\CMakeUserPresets.json"
$localEnvironment = $null
if (Test-Path -LiteralPath $presetPath) {
    $presets = Get-Content -Raw -LiteralPath $presetPath | ConvertFrom-Json
    $localEnvironment = $presets.configurePresets |
        Where-Object { $_.name -eq "windows-local" } |
        Select-Object -First 1 -ExpandProperty environment
}

function Find-Tool {
    param(
        [string]$Name,
        [string[]]$Candidates
    )

    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) {
            return $candidate
        }
    }

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    return $null
}

$qtRoot = if ($null -ne $localEnvironment) { $localEnvironment.QT_ROOT } else { $null }
$mingwVersion = if ($null -ne $localEnvironment) { $localEnvironment.QT_MINGW_VERSION } else { $null }
$vcpkgRoot = if ($null -ne $localEnvironment) { $localEnvironment.VCPKG_ROOT } else { $null }

$commonRequired = @(
    @{
        Name = "cmake"
        Candidates = @($(if ($qtRoot) { Join-Path $qtRoot "Tools\CMake_64\bin\cmake.exe" }))
    },
    @{
        Name = "vcpkg"
        Candidates = @($(if ($vcpkgRoot) { Join-Path $vcpkgRoot "vcpkg.exe" }))
    },
    @{
        Name = "git"
        Candidates = @()
    }
)

if ($Profile -in @("MinGW", "All")) {
    $commonRequired += @(
        @{
            Name = "mingw32-make"
            Candidates = @($(if ($qtRoot -and $mingwVersion) { Join-Path $qtRoot "Tools\$mingwVersion\bin\mingw32-make.exe" }))
        },
        @{
            Name = "g++"
            Candidates = @($(if ($qtRoot -and $mingwVersion) { Join-Path $qtRoot "Tools\$mingwVersion\bin\g++.exe" }))
        }
    )
}

foreach ($tool in $commonRequired) {
    $path = Find-Tool -Name $tool.Name -Candidates $tool.Candidates
    if ($null -eq $path) {
        Write-Host "[MISSING] $($tool.Name)" -ForegroundColor Red
        $failed = $true
    } else {
        Write-Host "[FOUND]   $($tool.Name): $path" -ForegroundColor Green
    }
}

if ($Profile -in @("VS2026", "All")) {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $vsInstallPath = $null
    if (Test-Path -LiteralPath $vswherePath) {
        $vsInstallPath = & $vswherePath -latest -products * -version "[18.0,19.0)" -property installationPath
    }

    $vsChecks = @(
        @{ Name = "Visual Studio 2026"; Path = $vsInstallPath },
        @{ Name = "Visual Studio CMake"; Path = $(if ($vsInstallPath) { Join-Path $vsInstallPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" }) },
        @{ Name = "MSVC v143 x64"; Path = $(if ($vsInstallPath) { Get-ChildItem -LiteralPath (Join-Path $vsInstallPath "VC\Tools\MSVC") -Directory -Filter "14.4*" -ErrorAction SilentlyContinue | Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName "bin\Hostx64\x64\cl.exe" } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1 }) },
        @{ Name = "Qt 6.11.1 MSVC 2022 64-bit"; Path = $(if ($qtRoot) { Join-Path $qtRoot "6.11.1\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake" }) }
    )

    foreach ($check in $vsChecks) {
        if (-not $check.Path -or -not (Test-Path -LiteralPath $check.Path)) {
            Write-Host "[MISSING] $($check.Name)" -ForegroundColor Red
            $failed = $true
        } else {
            Write-Host "[FOUND]   $($check.Name): $($check.Path)" -ForegroundColor Green
        }
    }
}

if ($failed) {
    throw "One or more migration prerequisites are missing."
}

Write-Host "$Profile prerequisite check passed." -ForegroundColor Green
exit 0
