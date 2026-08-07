[CmdletBinding()]
param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$CMakeArguments
)

$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakePath = if ($null -ne $cmakeCommand) { $cmakeCommand.Source } else { $null }

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

if ($null -eq $cmakePath) {
    throw "CMake was not found on PATH or through CMakeUserPresets.json."
}

& $cmakePath @CMakeArguments
exit $LASTEXITCODE
