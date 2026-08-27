[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^v[0-9]+\.[0-9]+$')]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$Destination,

    [switch]$Stage
)

$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$worktreeRoot = [IO.Path]::GetFullPath((Join-Path $repoRoot 'tmp\release-worktrees'))
$destinationPath = [IO.Path]::GetFullPath($Destination)
$requiredPrefix = $worktreeRoot.TrimEnd('\') + '\'

function Get-ReleaseRelativePath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $fullPath = [IO.Path]::GetFullPath($Path)
    $prefix = $destinationPath.TrimEnd('\') + '\'
    if (-not $fullPath.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside the release worktree: $fullPath"
    }
    return $fullPath.Substring($prefix.Length)
}

if (-not $destinationPath.StartsWith($requiredPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Release worktree must be below $worktreeRoot"
}

$gitMarker = Join-Path $destinationPath '.git'
if (-not (Test-Path -LiteralPath $gitMarker -PathType Leaf) -or
    -not (Get-Content -Raw -LiteralPath $gitMarker).StartsWith('gitdir:')) {
    throw "Destination is not a linked Git worktree: $destinationPath"
}

$branch = (& git -C $destinationPath branch --show-current).Trim()
if ($LASTEXITCODE -ne 0 -or $branch -ne 'tsre-scomod-wip') {
    throw "Release worktree must be on tsre-scomod-wip; found '$branch'."
}

$status = @(& git -C $destinationPath status --porcelain=v1)
if ($LASTEXITCODE -ne 0 -or $status.Count -ne 0) {
    throw 'Release worktree must be clean before export.'
}

$versionNumber = $Version.Substring(1) + '.0'
$releaseCopy = Join-Path $repoRoot "docsMaster\ReleaseCopies\$Version"
$sourceRoot = Join-Path $repoRoot 'TSREvcWIP'

foreach ($required in @(
    (Join-Path $sourceRoot 'main.cpp'),
    (Join-Path $sourceRoot 'Game.cpp'),
    (Join-Path $releaseCopy "RELEASE-NOTES-$Version.md"),
    (Join-Path $releaseCopy 'MANIFEST-SHA256.txt')
)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required release input is missing: $required"
    }
}

if (-not (Select-String -Quiet -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt') -Pattern "VERSION $([regex]::Escape($versionNumber))")) {
    throw "CMakeLists.txt does not identify $versionNumber."
}
if (-not (Select-String -Quiet -LiteralPath (Join-Path $sourceRoot 'Game.cpp') -Pattern ([regex]::Escape("AppVersion = `"$Version`"")))) {
    throw "Game.cpp does not identify $Version."
}

$manifestFailures = @()
Get-Content -LiteralPath (Join-Path $releaseCopy 'MANIFEST-SHA256.txt') | ForEach-Object {
    if ($_ -match '^([0-9A-F]{64})  (.+)$') {
        $documentPath = Join-Path $releaseCopy $matches[2]
        if (-not (Test-Path -LiteralPath $documentPath -PathType Leaf) -or
            (Get-FileHash -Algorithm SHA256 -LiteralPath $documentPath).Hash -ne $matches[1]) {
            $manifestFailures += $matches[2]
        }
    }
}
if ($manifestFailures.Count -ne 0) {
    throw "Release document manifest mismatch: $($manifestFailures -join ', ')"
}

& git -C $destinationPath rm -r -q --ignore-unmatch .
if ($LASTEXITCODE -ne 0) {
    throw 'Unable to clear the tracked public source tree.'
}

Get-ChildItem -Force -LiteralPath $sourceRoot | ForEach-Object {
    Copy-Item -Recurse -Force -LiteralPath $_.FullName -Destination $destinationPath
}

foreach ($directory in @('.github', 'cmake', 'tests', 'toolchains')) {
    $sourceDirectory = Join-Path $repoRoot $directory
    $targetDirectory = Join-Path $destinationPath $directory
    New-Item -ItemType Directory -Force -Path $targetDirectory | Out-Null
    Get-ChildItem -Force -LiteralPath $sourceDirectory | ForEach-Object {
        Copy-Item -Recurse -Force -LiteralPath $_.FullName -Destination $targetDirectory
    }
}
foreach ($file in @('CMakeLists.txt', 'CMakePresets.json', 'vcpkg.json')) {
    Copy-Item -Force -LiteralPath (Join-Path $repoRoot $file) -Destination (Join-Path $destinationPath $file)
}

$scriptsDestination = Join-Path $destinationPath 'scripts'
New-Item -ItemType Directory -Force -Path $scriptsDestination | Out-Null
foreach ($script in @(
    'Check-Prerequisites.ps1', 'Invoke-CMake.ps1',
    'Invoke-RouteRegression.ps1', 'Prepare-SourceRelease.ps1',
    'SourceRelease.gitattributes', 'SourceRelease.gitignore'
)) {
    Copy-Item -Force -LiteralPath (Join-Path $PSScriptRoot $script) -Destination (Join-Path $scriptsDestination $script)
}

$publicCMakePath = Join-Path $destinationPath 'CMakeLists.txt'
$publicCMake = [IO.File]::ReadAllText($publicCMakePath)
$publicCMake = $publicCMake.Replace(
    'DESCRIPTION "Qt6/CMake migration of TSRE GenX"',
    'DESCRIPTION "TSRE GenX Qt6 route editor"'
)
$publicCMake = $publicCMake.Replace(
    'set(TSRE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/TSREvcWIP")',
    'set(TSRE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")'
)
$scaffoldPattern = '(?ms)\r?\n# Keep the scaffold configurable before the frozen source is imported\..*?^endif\(\)\r?\n'
$publicCMake = [regex]::Replace($publicCMake, $scaffoldPattern, "`r`n")
$publicCMake = $publicCMake.Replace('TSREvcWIP/', '')
$publicCMake = $publicCMake.Replace(
    '${CMAKE_CURRENT_SOURCE_DIR}/docsInternal/proposals/forest.v1.example.json',
    '${CMAKE_CURRENT_SOURCE_DIR}/tests/data/forest.v1.example.json'
)
$publicCMake = $publicCMake.Replace(
    '# Preserve the imported root-level source layout during the first migration.',
    '# Keep the established public source layout at the repository root.'
)
if ($publicCMake.Contains('TSREvcWIP') -or $publicCMake.Contains('docsInternal/')) {
    throw 'Public CMake transformation left a development-layout path behind.'
}
[IO.File]::WriteAllText($publicCMakePath, $publicCMake, [Text.UTF8Encoding]::new($false))

$testsDataDestination = Join-Path $destinationPath 'tests\data'
New-Item -ItemType Directory -Force -Path $testsDataDestination | Out-Null
Copy-Item -Force -LiteralPath (
    Join-Path $repoRoot 'docsInternal\proposals\forest.v1.example.json'
) -Destination (Join-Path $testsDataDestination 'forest.v1.example.json')

$publicVcpkgPath = Join-Path $destinationPath 'vcpkg.json'
$publicVcpkg = [IO.File]::ReadAllText($publicVcpkgPath).Replace(
    '"description": "Qt6/CMake migration of TSRE GenX"',
    '"description": "TSRE GenX Qt6 route editor"'
)
[IO.File]::WriteAllText($publicVcpkgPath, $publicVcpkg, [Text.UTF8Encoding]::new($false))

$testsReadmePath = Join-Path $destinationPath 'tests\README.md'
if (Test-Path -LiteralPath $testsReadmePath -PathType Leaf) {
    $testsReadme = [IO.File]::ReadAllText($testsReadmePath).Replace(
        '`docsInternal/03-test-matrix.md`',
        ('`Docs/TEST-MATRIX-' + $Version + '.md`')
    )
    [IO.File]::WriteAllText($testsReadmePath, $testsReadme, [Text.UTF8Encoding]::new($false))
}

$publicHarnessPath = Join-Path $destinationPath 'scripts\Invoke-RouteRegression.ps1'
$publicHarness = [IO.File]::ReadAllText($publicHarnessPath)
$harnessDefaultsPattern = '(?ms)    \[string\]\$RouteRoot\s*=.*?    \[string\]\$LogDirectory\s*=.*?\r?\n\r?\n(?=    \[ValidateSet)'
$harnessDefaults = @'
    [Parameter(Mandatory = $true)]
    [string]$RouteRoot,

    [string]$EvidenceRoot =
        (Join-Path $PSScriptRoot "..\.route-regression-evidence"),

    [string]$LogDirectory = $EvidenceRoot,

'@
$publicHarness = [regex]::Replace($publicHarness, $harnessDefaultsPattern, $harnessDefaults)
if ($publicHarness -match '[A-Z]:\\' -or $publicHarness.Contains('TSREvcTST')) {
    throw 'Public route-regression harness still contains a machine-specific default.'
}
[IO.File]::WriteAllText($publicHarnessPath, $publicHarness, [Text.UTF8Encoding]::new($false))

$docsDestination = Join-Path $destinationPath 'Docs'
New-Item -ItemType Directory -Force -Path $docsDestination | Out-Null
Get-ChildItem -Force -LiteralPath $releaseCopy | ForEach-Object {
    Copy-Item -Recurse -Force -LiteralPath $_.FullName -Destination $docsDestination
}

foreach ($document in @(
    'LICENSE.md', 'README.md', "RELEASE-NOTES-$Version.md",
    "TEST-MATRIX-$Version.md", 'THIRD-PARTY-NOTICES.txt',
    'scoFileEdit.txt', 'scoGitRelease.txt', 'scoKeyList.txt',
    'scoUiStyle.txt', 'scoWorkList.txt', 'workList.rtf'
)) {
    $documentPath = Join-Path $releaseCopy $document
    if (Test-Path -LiteralPath $documentPath -PathType Leaf) {
        Copy-Item -Force -LiteralPath $documentPath -Destination (Join-Path $destinationPath $document)
    }
}

# Private route recovery evidence belongs only in the development checkout.
# Fail closed if its known identifiers enter any public path or text input.
$privateMarkers = @(
    ('SCO_' + 'LHR4'),
    ('SCO_' + 'LHR_' + 'Bad'),
    ('ORTS' + 'mini_F'),
    ('polyveg-removal-' + 'backup-')
)
$publicFiles = @(Get-ChildItem -Recurse -Force -File -LiteralPath $destinationPath)
$privateLeaks = [Collections.Generic.HashSet[string]]::new(
    [StringComparer]::OrdinalIgnoreCase
)
foreach ($file in $publicFiles) {
    $relative = Get-ReleaseRelativePath -Path $file.FullName
    foreach ($marker in $privateMarkers) {
        if ($relative.IndexOf($marker, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
            [void]$privateLeaks.Add($relative)
        }
    }

    if ($file.Extension -notin @(
        '.bat', '.cmake', '.cpp', '.h', '.ini', '.json', '.md', '.ps1',
        '.qrc', '.rc', '.txt', '.xml', '.yml', '.yaml'
    )) {
        continue
    }
    $text = [IO.File]::ReadAllText($file.FullName)
    foreach ($marker in $privateMarkers) {
        if ($text.IndexOf($marker, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
            [void]$privateLeaks.Add($relative)
        }
    }
}
if ($privateLeaks.Count -ne 0) {
    throw "Private route evidence was exported: $(@($privateLeaks) -join ', ')"
}

Copy-Item -Force -LiteralPath (Join-Path $PSScriptRoot 'SourceRelease.gitignore') -Destination (Join-Path $destinationPath '.gitignore')
Copy-Item -Force -LiteralPath (Join-Path $PSScriptRoot 'SourceRelease.gitattributes') -Destination (Join-Path $destinationPath '.gitattributes')

Get-ChildItem -Recurse -Force -File -LiteralPath $destinationPath |
    Where-Object { $_.Name -match '\.backup\.' } |
    Remove-Item -Force

$forbiddenPattern = '(^|[\\/])(dist|build|out|TSREvcWIP|TSREvcTST|masterDocs|docs|refs|snapshots|tmp|\.test-temp|\.vscode)([\\/]|$)|(^|[\\/])(AGENTS\.md|AI_HouseRules\.txt|CMakeUserPresets\.json)$|(^|[\\/])AAA_|\.(exe|dll|pdb|zip|7z|log|pro)$|\.backup\.'
$forbidden = Get-ChildItem -Recurse -Force -File -LiteralPath $destinationPath |
    Where-Object {
        $relative = Get-ReleaseRelativePath -Path $_.FullName
        $relative -ne '.git' -and $relative -cmatch $forbiddenPattern
    } |
    ForEach-Object { Get-ReleaseRelativePath -Path $_.FullName }
if ($forbidden.Count -ne 0) {
    throw "Forbidden release files were exported: $($forbidden -join ', ')"
}

if ($Stage) {
    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & git -c core.autocrlf=false -c core.safecrlf=false -C $destinationPath add -A
    $addExitCode = $LASTEXITCODE
    $ErrorActionPreference = $previousErrorActionPreference
    if ($addExitCode -ne 0) {
        throw 'Unable to stage the public source export.'
    }
    & git -C $destinationPath diff --cached --check
    if ($LASTEXITCODE -ne 0) {
        throw 'The staged public source export has whitespace errors.'
    }
    $manifestPath = Join-Path $repoRoot "AAA_Git-$Version-staged-manifest.txt"
    & git -C $destinationPath diff --cached --name-status | Set-Content -Encoding UTF8 -LiteralPath $manifestPath
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to write the staged-file manifest.'
    }
    Write-Output "Staged-file manifest: $manifestPath"
    & git -C $destinationPath diff --cached --shortstat
    & git -C $destinationPath status --short | Group-Object { $_.Substring(0, 2) } |
        Sort-Object Name | ForEach-Object { '{0}: {1}' -f $_.Name, $_.Count }
} else {
    & git -C $destinationPath status --short
}
