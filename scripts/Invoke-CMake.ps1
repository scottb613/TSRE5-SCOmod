[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$CMakeArguments
)

$isMsvcPreset = $null -ne ($CMakeArguments | Where-Object { $_ -match "windows-msvc|^--list-presets" } | Select-Object -First 1)
$cmakePath = $null

if ($isMsvcPreset) {
    $vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswherePath) {
        $vsInstallPath = & $vswherePath -latest -products * -version "[18.0,19.0)" -property installationPath
        if ($vsInstallPath) {
            $candidate = Join-Path $vsInstallPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path -LiteralPath $candidate) {
                $cmakePath = $candidate
            }
        }
    }
}

if ($null -eq $cmakePath) {
    $presetPath = Join-Path $PSScriptRoot "..\CMakeUserPresets.json"
    if (Test-Path -LiteralPath $presetPath) {
        $presets = Get-Content -Raw -LiteralPath $presetPath | ConvertFrom-Json
        $localEnvironment = $presets.configurePresets |
            Where-Object { $_.name -eq "windows-local" } |
            Select-Object -First 1 -ExpandProperty environment

        if ($null -ne $localEnvironment.QT_ROOT) {
            $candidate = Join-Path $localEnvironment.QT_ROOT "Tools\CMake_64\bin\cmake.exe"
            if (Test-Path -LiteralPath $candidate) {
                $cmakePath = $candidate
            }
        }
    }
}

if ($null -eq $cmakePath -and -not $isMsvcPreset) {
    $cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
    if ($null -ne $cmakeCommand) {
        $cmakePath = $cmakeCommand.Source
    }
}

if ($null -eq $cmakePath) {
    if ($isMsvcPreset) {
        throw "Visual Studio 2026 CMake was not found. Install the Visual Studio C++ CMake tools component."
    }
    throw "CMake was not found through the Qt installation in CMakeUserPresets.json or on PATH."
}

if ($isMsvcPreset) {
    $cmakeVersion = (& $cmakePath --version | Select-Object -First 1) -replace '^cmake version\s+', ''
    if ([version]($cmakeVersion -replace '-.*$', '') -lt [version]'4.2') {
        throw "The Visual Studio 2026 generator requires CMake 4.2 or later; found $cmakeVersion."
    }
}

& $cmakePath @CMakeArguments
exit $LASTEXITCODE
