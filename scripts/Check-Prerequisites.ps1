[CmdletBinding()]
param()

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

$required = @(
    @{
        Name = "cmake"
        Candidates = @($(if ($qtRoot) { Join-Path $qtRoot "Tools\CMake_64\bin\cmake.exe" }))
    },
    @{
        Name = "mingw32-make"
        Candidates = @($(if ($qtRoot -and $mingwVersion) { Join-Path $qtRoot "Tools\$mingwVersion\bin\mingw32-make.exe" }))
    },
    @{
        Name = "g++"
        Candidates = @($(if ($qtRoot -and $mingwVersion) { Join-Path $qtRoot "Tools\$mingwVersion\bin\g++.exe" }))
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

foreach ($tool in $required) {
    $path = Find-Tool -Name $tool.Name -Candidates $tool.Candidates
    if ($null -eq $path) {
        Write-Host "[MISSING] $($tool.Name)" -ForegroundColor Red
        $failed = $true
    } else {
        Write-Host "[FOUND]   $($tool.Name): $path" -ForegroundColor Green
    }
}

if ($failed) {
    throw "One or more migration prerequisites are missing."
}

Write-Host "Prerequisite check passed." -ForegroundColor Green
exit 0
