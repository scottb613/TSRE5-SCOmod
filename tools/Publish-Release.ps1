#requires -Version 5.1

<#
.SYNOPSIS
Builds, synchronizes, packages, and optionally publishes a TSRE GenX release.

.DESCRIPTION
Without -Publish, this script performs all local release preparation: it builds
SCOmodWIP, synchronizes Git-tracked source into the repository, propagates
documentation and splash assets, updates SCOmodTST and the release directory,
rejects generated garbage, creates the ZIP, and verifies it.

With -Publish, it additionally stages and commits the repository, pushes the
branch and tag, creates or updates the GitHub release, uploads the ZIP, and
verifies the uploaded asset's size and SHA-256 digest.

.EXAMPLE
.\tools\Publish-Release.ps1 -Version v0.7

.EXAMPLE
.\tools\Publish-Release.ps1 -Version v0.7 -Publish -CommitMessage "Publish v0.7"

.EXAMPLE
.\tools\Publish-Release.ps1 -Version v0.6 -Publish -MoveExistingTag `
    -CommitMessage "Refresh v0.6 release"

.EXAMPLE
.\tools\Publish-Release.ps1 -Version v0.6 -ValidateOnly
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [ValidatePattern('^v[0-9]+\.[0-9]+(?:\.[0-9]+)?(?:[-+][0-9A-Za-z.-]+)?$')]
    [string]$Version = 'v0.6',

    [string]$Branch,

    [string]$CommitMessage,

    [string]$AppDataVersion = '0.697',

    [string]$ReleaseTitle = 'TSRE GenX',

    [ValidateRange(1, 64)]
    [int]$Jobs = 6,

    [switch]$SkipBuild,

    [switch]$ValidateOnly,

    [switch]$Publish,

    [switch]$MoveExistingTag
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
if ($WhatIfPreference) {
    $ValidateOnly = $true
    Write-Warning '-WhatIf implies -ValidateOnly; no build, copy, package, Git, or GitHub changes will be made.'
}

function Write-Step {
    param([Parameter(Mandatory)][string]$Message)
    Write-Host "`n==> $Message" -ForegroundColor Cyan
}

function Assert-Path {
    param(
        [Parameter(Mandatory)][string]$LiteralPath,
        [Parameter(Mandatory)][string]$Description,
        [ValidateSet('Any', 'Leaf', 'Container')][string]$PathType = 'Any'
    )

    $testArgs = @{ LiteralPath = $LiteralPath }
    if ($PathType -ne 'Any') { $testArgs.PathType = $PathType }
    if (-not (Test-Path @testArgs)) {
        throw "$Description was not found: $LiteralPath"
    }
}

function Resolve-RequiredTool {
    param(
        [Parameter(Mandatory)][string]$Name,
        [string[]]$Fallback = @()
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }

    foreach ($candidate in $Fallback) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return $candidate
        }
    }
    throw "Required tool '$Name' was not found. Install it or add it to PATH."
}

function Invoke-Native {
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [Parameter(Mandatory)][string[]]$ArgumentList,
        [string]$WorkingDirectory
    )

    $oldLocation = Get-Location
    try {
        if ($WorkingDirectory) { Set-Location -LiteralPath $WorkingDirectory }
        & $FilePath @ArgumentList
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $LASTEXITCODE`: $FilePath $($ArgumentList -join ' ')"
        }
    }
    finally {
        Set-Location -LiteralPath $oldLocation
    }
}

function Copy-ReleaseFile {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    Assert-Path $Source 'Source file' Leaf
    $parent = Split-Path -Parent $Destination
    if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Remove-IfPresent {
    param([Parameter(Mandatory)][string[]]$LiteralPath)
    foreach ($path in $LiteralPath) {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Force }
    }
}

function Get-SplashFiles {
    param([Parameter(Mandatory)][string]$Directory)

    $extensions = @('.png', '.jpg', '.jpeg', '.bmp', '.webp')
    return @(Get-ChildItem -LiteralPath $Directory -File |
        Where-Object { $_.Name -like 'Splash_*' -and $extensions -contains $_.Extension.ToLowerInvariant() } |
        Sort-Object Name)
}

function Sync-SplashDirectory {
    param(
        [Parameter(Mandatory)][System.IO.FileInfo[]]$SourceFiles,
        [Parameter(Mandatory)][string]$Destination
    )

    if (-not (Test-Path -LiteralPath $Destination -PathType Container)) {
        New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    }

    foreach ($existing in Get-SplashFiles $Destination) {
        Remove-Item -LiteralPath $existing.FullName -Force
    }
    Remove-IfPresent @(Join-Path $Destination 'load.png')

    foreach ($source in $SourceFiles) {
        Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $Destination $source.Name) -Force
    }
}

function Sync-TrackedWipFiles {
    param(
        [Parameter(Mandatory)][string]$Git,
        [Parameter(Mandatory)][string]$Repository,
        [Parameter(Mandatory)][string]$Wip
    )

    $tracked = @(& $Git -C $Repository ls-files)
    if ($LASTEXITCODE -ne 0) { throw 'Could not read the repository file manifest.' }
    $trackedSet = @{}
    foreach ($path in $tracked) { $trackedSet[$path.Replace('\', '/').ToLowerInvariant()] = $true }

    # These files are maintained in masterDocs, by this script, or only in the
    # repository/release package. Everything else already tracked by Git follows
    # the WIP copy, including intentional deletions.
    $repositoryOwned = @(
        '^\.gitignore$',
        '^README\.md$',
        '^sco(?:GitRelease|WorkList|FileEdit|KeyList)\.txt$',
        '^docs/',
        '^tools/',
        '^AddShortcutDesktop\.cmd$',
        '^THIRD-PARTY-NOTICES\.txt$',
        '^Makefile$',
        '^moc_[^/]*\.cpp$',
        '^object_script\.TSRE5$',
        '^content/(?:SCOclick\.wav|SNOW/terrain\.ace|terrain\.ace|tsre\.ico)$',
        '^tsre_appdata/'
    )

    foreach ($relative in $tracked) {
        $normalized = $relative.Replace('\', '/')
        $owned = $false
        foreach ($pattern in $repositoryOwned) {
            if ($normalized -match $pattern) { $owned = $true; break }
        }
        if ($owned) { continue }

        $source = Join-Path $Wip $relative
        $destination = Join-Path $Repository $relative
        if (Test-Path -LiteralPath $source -PathType Leaf) {
            if (Test-Path -LiteralPath $destination -PathType Leaf) {
                & $Git -c core.autocrlf=false diff --no-index --quiet --ignore-space-at-eol -- $destination $source
                $compareExit = $LASTEXITCODE
                if ($compareExit -eq 0) { continue }
                if ($compareExit -gt 1) { throw "Could not compare $relative before synchronization." }
            }
            Copy-ReleaseFile $source $destination
        }
        elseif (Test-Path -LiteralPath $destination -PathType Leaf) {
            Remove-Item -LiteralPath $destination -Force
        }
    }

    # Include genuinely new source files automatically. The exclusions are
    # long-standing IDE/toolchain files in SCOmodWIP that are intentionally not
    # part of the repository.
    $sourceExtensions = @('.c', '.cpp', '.h', '.pro', '.pri', '.qrc', '.rc')
    $wipOnly = @(
        '^(?:al|alc|alext|efx|efx-creative|efx-presets)\.h$',
        '^(?:c|cpp)_standard_headers_indexer\.(?:c|cpp)$',
        '^qt-.*\.pro$',
        '^TerrainTrackMathTest\.cpp$'
    )
    $newSources = @(Get-ChildItem -LiteralPath $Wip -File -Recurse | Where-Object {
        $relative = $_.FullName.Substring($Wip.Length + 1).Replace('\', '/')
        if ($relative -match '^(?:build|dist|nbproject)/' -or $_.Name -match '^moc_') { return $false }
        if ($sourceExtensions -notcontains $_.Extension.ToLowerInvariant()) { return $false }
        foreach ($pattern in $wipOnly) {
            if ($relative -match $pattern) { return $false }
        }
        return -not $trackedSet.ContainsKey($relative.ToLowerInvariant())
    })
    foreach ($source in $newSources) {
        $relative = $source.FullName.Substring($Wip.Length + 1)
        Copy-ReleaseFile $source.FullName (Join-Path $Repository $relative)
        Write-Host "New source: $relative" -ForegroundColor Yellow
    }
}

function Write-WorkListRtf {
    param(
        [Parameter(Mandatory)][string]$Source,
        [Parameter(Mandatory)][string]$Destination
    )

    Add-Type -AssemblyName System.Windows.Forms
    $box = New-Object System.Windows.Forms.RichTextBox
    try {
        $box.Font = New-Object System.Drawing.Font('Consolas', 10)
        $lines = Get-Content -LiteralPath $Source
        for ($index = 0; $index -lt $lines.Count; $index++) {
            $line = [string]$lines[$index]
            $start = $box.TextLength
            $box.AppendText($line)
            if ($index -lt ($lines.Count - 1)) { $box.AppendText("`r`n") }

            $isTitle = $index -eq 0
            $isHeading = $line -match '^[A-Z][A-Z0-9 /&()''.,:+-]{3,}$'
            $isSection = $line -match '^--- .+ ---$'
            if ($isTitle -or $isHeading -or $isSection) {
                $box.Select($start, $line.Length)
                $box.SelectionFont = New-Object System.Drawing.Font('Consolas', 10, [System.Drawing.FontStyle]::Bold)
            }
        }
        $box.Select(0, 0)
        $box.SaveFile($Destination, [System.Windows.Forms.RichTextBoxStreamType]::RichText)
    }
    finally {
        $box.Dispose()
    }
}

function Assert-NoGarbage {
    param(
        [Parameter(Mandatory)][AllowEmptyCollection()][string[]]$RelativePaths,
        [Parameter(Mandatory)][string]$Scope
    )

    $badPatterns = @(
        '(^|/)(build|dist)(/|$)',
        '(^|/)moc_[^/]*\.cpp$',
        '(^|/)(?:compile-last|tsre-log|splash-cycle\.json)(?:\.|$)',
        '(^|/)Forum_NoPush(/|$)',
        '(^|/)bbsTags\.txt$',
        '(^|/)[^/]*(?:backup|\.bak$|\.log$|\.tmp$|~$)',
        '(^|/)load\.png$'
    )
    $bad = @($RelativePaths | Where-Object {
        $normalized = $_.Replace('\', '/')
        foreach ($pattern in $badPatterns) {
            if ($normalized -match $pattern) { return $true }
        }
        return $false
    } | Sort-Object -Unique)

    if ($bad.Count -gt 0) {
        throw "Generated or obsolete files were found in ${Scope}:`n  $($bad -join "`n  ")"
    }
}

function Assert-SameHash {
    param(
        [Parameter(Mandatory)][string]$Expected,
        [Parameter(Mandatory)][string[]]$Actual,
        [Parameter(Mandatory)][string]$Description
    )

    $expectedHash = (Get-FileHash -LiteralPath $Expected -Algorithm SHA256).Hash
    foreach ($path in $Actual) {
        Assert-Path $path $Description Leaf
        $actualHash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ($actualHash -ne $expectedHash) {
            throw "$Description is out of sync: $path"
        }
    }
}

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$workspaceRoot = Split-Path -Parent $repoRoot
$wipRoot = Join-Path $workspaceRoot 'SCOmodWIP'
$testRoot = Join-Path $workspaceRoot 'SCOmodTST'
$masterDocs = Join-Path $workspaceRoot 'masterDocs'
$distRoot = Join-Path $workspaceRoot 'dist'
$releaseName = "tsre-scomod-$Version"
$releaseDirectory = Join-Path $distRoot $releaseName
$releaseZip = Join-Path $distRoot "$releaseName.zip"
$sourceAppData = Join-Path (Join-Path $wipRoot 'tsre_appdata') $AppDataVersion
$repoAppData = Join-Path (Join-Path $repoRoot 'tsre_appdata') $AppDataVersion
$testAppData = Join-Path (Join-Path $testRoot 'tsre_appdata') $AppDataVersion
$releaseAppData = Join-Path (Join-Path $releaseDirectory 'tsre_appdata') $AppDataVersion
$builtExecutable = Join-Path $wipRoot 'dist\Release_x64\MinGW_64b-Windows\TSRE5.exe'
$testExecutable = Join-Path $testRoot 'TSRE5.exe'
$releaseExecutable = Join-Path $releaseDirectory 'TSRE5.exe'
$releaseNotes = Join-Path $masterDocs 'README.md'
$bbsDraft = Join-Path $masterDocs 'Forum_NoPush\bbsTags.txt'
$bbsUpdater = Join-Path $PSScriptRoot 'Update-BbsDraft.ps1'
$uiSoundNames = @('SCOtic.wav')
$shaderRelativePaths = @('shaders\StandardFog.fs', 'shaders330\StandardFog.fs')

Write-Step 'Checking workspace layout and required tools'
foreach ($item in @(
    @($repoRoot, 'Git repository'),
    @($wipRoot, 'SCOmodWIP directory'),
    @($testRoot, 'SCOmodTST directory'),
    @($masterDocs, 'masterDocs directory'),
    @($releaseDirectory, 'release directory'),
    @($sourceAppData, 'source app-data directory')
)) {
    Assert-Path $item[0] $item[1] Container
}
Assert-Path $bbsDraft 'local BBS announcement draft' Leaf
Assert-Path $bbsUpdater 'BBS draft formatter' Leaf

$git = Resolve-RequiredTool 'git.exe' @('C:\Program Files\Git\cmd\git.exe')
$sevenZip = Resolve-RequiredTool '7z.exe' @('C:\Program Files\7-Zip\7z.exe')
$bash = Resolve-RequiredTool 'bash.exe' @('Y:\msys64\usr\bin\bash.exe')
$gh = $null
if ($Publish) {
    $gh = Resolve-RequiredTool 'gh.exe' @('C:\Program Files\GitHub CLI\gh.exe')
}

if (-not $Branch) {
    $Branch = (& $git -C $repoRoot branch --show-current).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $Branch) { throw 'Could not determine the current Git branch.' }
}
if (-not $CommitMessage) { $CommitMessage = "Prepare $Version release" }

$splashFiles = Get-SplashFiles $sourceAppData
if ($splashFiles.Count -eq 0) {
    throw "No supported Splash_* images were found in $sourceAppData"
}
Write-Host "Branch: $Branch"
Write-Host "Release: $Version ($($splashFiles.Count) splash images)"

Write-Step 'Checking repository and release contents for garbage'
$porcelain = @(& $git -C $repoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed.' }
$repoPaths = @($porcelain | ForEach-Object {
    if ($_.Length -ge 4) {
        $path = $_.Substring(3)
        if ($path -match ' -> ') { $path = ($path -split ' -> ', 2)[1] }
        $path.Trim('"')
    }
})
Assert-NoGarbage $repoPaths 'the Git working tree'

$releaseFiles = @(Get-ChildItem -LiteralPath $releaseDirectory -File -Recurse | ForEach-Object {
    $_.FullName.Substring($releaseDirectory.Length + 1)
})
Assert-NoGarbage $releaseFiles 'the release directory'

if ($ValidateOnly) {
    Write-Step 'Validating current release artifacts without modifying them'
    & $bbsUpdater -WorkList (Join-Path $masterDocs 'scoWorkList.txt') -BbsDraft $bbsDraft -Check
    Assert-Path $builtExecutable 'built executable' Leaf
    Assert-Path $testExecutable 'test executable' Leaf
    Assert-Path $releaseExecutable 'release executable' Leaf
    Assert-Path $releaseZip 'release ZIP' Leaf
    Invoke-Native $sevenZip @('t', '-bso0', '-bsp0', $releaseZip)
    $zipHash = Get-FileHash -LiteralPath $releaseZip -Algorithm SHA256
    Write-Host "Validation passed. ZIP SHA-256: $($zipHash.Hash)" -ForegroundColor Green
    return
}

if (-not $SkipBuild) {
    $running = @(Get-Process -Name 'TSRE5' -ErrorAction SilentlyContinue)
    if ($running.Count -gt 0) {
        throw 'TSRE5 is running. Close it before building and synchronizing the release.'
    }

    Write-Step "Building SCOmodWIP with $Jobs parallel jobs"
    $msysWip = $wipRoot.Replace('\', '/').Replace('Y:', '/y')
    $buildCommand = "export PATH=/mingw64/bin:/usr/bin:`$PATH; cd `"$msysWip`" && mingw32-make -j$Jobs CC='ccache gcc' CCC='ccache g++' CXX='ccache g++'"
    Invoke-Native $bash @('-lc', $buildCommand)
}
Assert-Path $builtExecutable 'built executable' Leaf

Write-Step 'Synchronizing Git-tracked source from SCOmodWIP'
Sync-TrackedWipFiles $git $repoRoot $wipRoot

Write-Step 'Propagating authoritative documentation'
& $bbsUpdater -WorkList (Join-Path $masterDocs 'scoWorkList.txt') -BbsDraft $bbsDraft
$docNames = @('README.md', 'scoGitRelease.txt', 'scoWorkList.txt', 'scoFileEdit.txt', 'scoKeyList.txt')
$docDestinations = @(
    (Join-Path $masterDocs 'Docs'),
    $repoRoot,
    (Join-Path $repoRoot 'Docs'),
    $wipRoot,
    $releaseDirectory
)
foreach ($name in $docNames) {
    $source = Join-Path $masterDocs $name
    Assert-Path $source "authoritative $name" Leaf
    foreach ($destination in $docDestinations) {
        Copy-ReleaseFile $source (Join-Path $destination $name)
    }
}

$testDocMap = [ordered]@{
    'README.md'         = @('README.txt')
    'scoGitRelease.txt' = @('gitRelease.txt')
    'scoWorkList.txt'   = @('scoWorkList.txt', 'worklist.txt')
    'scoFileEdit.txt'   = @('fileEdit.txt')
    'scoKeyList.txt'    = @('scoKeyList.txt')
}
foreach ($sourceName in $testDocMap.Keys) {
    foreach ($destinationName in $testDocMap[$sourceName]) {
        Copy-ReleaseFile (Join-Path $masterDocs $sourceName) (Join-Path $testRoot $destinationName)
    }
}
Write-WorkListRtf (Join-Path $masterDocs 'scoWorkList.txt') (Join-Path $testRoot 'workList.rtf')
Write-WorkListRtf (Join-Path $masterDocs 'scoWorkList.txt') (Join-Path $releaseDirectory 'workList.rtf')

Write-Step 'Synchronizing executable and splash artwork'
Copy-ReleaseFile $builtExecutable $testExecutable
Copy-ReleaseFile $builtExecutable $releaseExecutable
foreach ($destination in @($repoAppData, $testAppData, $releaseAppData)) {
    Sync-SplashDirectory $splashFiles $destination
}
foreach ($soundName in $uiSoundNames) {
    $source = Join-Path (Join-Path $wipRoot 'content') $soundName
    foreach ($destination in @(
        (Join-Path (Join-Path $repoRoot 'content') $soundName),
        (Join-Path (Join-Path $testRoot 'content') $soundName),
        (Join-Path (Join-Path $releaseDirectory 'content') $soundName)
    )) {
        Copy-ReleaseFile $source $destination
    }
}
foreach ($relative in $shaderRelativePaths) {
    $source = Join-Path $sourceAppData $relative
    foreach ($destinationRoot in @($repoAppData, $testAppData, $releaseAppData)) {
        Copy-ReleaseFile $source (Join-Path $destinationRoot $relative)
    }
}
Remove-IfPresent @(
    (Join-Path (Join-Path $repoRoot 'tsre_appdata') 'load.png'),
    (Join-Path (Join-Path $testRoot 'tsre_appdata') 'load.png'),
    (Join-Path (Join-Path $releaseDirectory 'tsre_appdata') 'load.png')
)

Write-Step 'Verifying synchronized files'
Assert-SameHash $builtExecutable @($testExecutable, $releaseExecutable) 'TSRE5.exe'
foreach ($name in $docNames) {
    $master = Join-Path $masterDocs $name
    $copies = @()
    foreach ($destination in $docDestinations) { $copies += Join-Path $destination $name }
    Assert-SameHash $master $copies $name
}
foreach ($source in $splashFiles) {
    Assert-SameHash $source.FullName @(
        (Join-Path $repoAppData $source.Name),
        (Join-Path $testAppData $source.Name),
        (Join-Path $releaseAppData $source.Name)
    ) $source.Name
}
foreach ($soundName in $uiSoundNames) {
    $source = Join-Path (Join-Path $wipRoot 'content') $soundName
    Assert-SameHash $source @(
        (Join-Path (Join-Path $repoRoot 'content') $soundName),
        (Join-Path (Join-Path $testRoot 'content') $soundName),
        (Join-Path (Join-Path $releaseDirectory 'content') $soundName)
    ) $soundName
}
foreach ($relative in $shaderRelativePaths) {
    $source = Join-Path $sourceAppData $relative
    Assert-SameHash $source @(
        (Join-Path $repoAppData $relative),
        (Join-Path $testAppData $relative),
        (Join-Path $releaseAppData $relative)
    ) $relative
}

$releaseFiles = @(Get-ChildItem -LiteralPath $releaseDirectory -File -Recurse | ForEach-Object {
    $_.FullName.Substring($releaseDirectory.Length + 1)
})
Assert-NoGarbage $releaseFiles 'the synchronized release directory'

Write-Step 'Building and testing release ZIP'
$tempZip = Join-Path $distRoot (".{0}.{1}.tmp.zip" -f $releaseName, [Guid]::NewGuid().ToString('N'))
try {
    Invoke-Native $sevenZip @('a', '-tzip', '-mx=9', '-bso0', '-bsp0', $tempZip, '*') $releaseDirectory
    Invoke-Native $sevenZip @('t', '-bso0', '-bsp0', $tempZip)

    $zipListing = @(& $sevenZip l -slt $tempZip)
    if ($LASTEXITCODE -ne 0) { throw 'Could not inspect the release ZIP.' }
    $archivePaths = @($zipListing | Where-Object { $_ -like 'Path = *' } | ForEach-Object {
        $_.Substring(7).Replace('\', '/')
    } | Where-Object { $_ -notmatch '\.tmp\.zip$' })
    foreach ($required in @('TSRE5.exe', 'README.md', 'scoWorkList.txt')) {
        if ($archivePaths -notcontains $required) { throw "Release ZIP is missing $required" }
    }
    foreach ($splash in $splashFiles) {
        $required = "tsre_appdata/$AppDataVersion/$($splash.Name)"
        if ($archivePaths -notcontains $required) { throw "Release ZIP is missing $required" }
    }
    Assert-NoGarbage $archivePaths 'the release ZIP'

    Move-Item -LiteralPath $tempZip -Destination $releaseZip -Force
}
finally {
    if (Test-Path -LiteralPath $tempZip) { Remove-Item -LiteralPath $tempZip -Force }
}

$zipInfo = Get-Item -LiteralPath $releaseZip
$zipHash = Get-FileHash -LiteralPath $releaseZip -Algorithm SHA256
Write-Host "ZIP: $($zipInfo.FullName)"
Write-Host "Size: $($zipInfo.Length) bytes"
Write-Host "SHA-256: $($zipHash.Hash)" -ForegroundColor Green

Write-Step 'Rechecking repository changes'
$porcelain = @(& $git -C $repoRoot status --porcelain=v1 --untracked-files=all)
if ($LASTEXITCODE -ne 0) { throw 'git status failed.' }
$repoPaths = @($porcelain | ForEach-Object {
    if ($_.Length -ge 4) {
        $path = $_.Substring(3)
        if ($path -match ' -> ') { $path = ($path -split ' -> ', 2)[1] }
        $path.Trim('"')
    }
})
Assert-NoGarbage $repoPaths 'the prepared Git working tree'
& $git -C $repoRoot status --short

if (-not $Publish) {
    Write-Host "`nLocal release preparation passed. Re-run with -Publish to commit, tag, push, and upload." -ForegroundColor Green
    return
}

if (-not $PSCmdlet.ShouldProcess("origin/$Branch and GitHub release $Version", 'Commit, push, tag, and publish release')) {
    return
}

Write-Step 'Authenticating and publishing to GitHub'
Invoke-Native $gh @('auth', 'status', '--hostname', 'github.com')
$currentBranch = (& $git -C $repoRoot branch --show-current).Trim()
if ($currentBranch -ne $Branch) {
    throw "Current branch '$currentBranch' does not match requested branch '$Branch'."
}

Invoke-Native $git @('-C', $repoRoot, 'add', '-A')
Invoke-Native $git @('-C', $repoRoot, 'diff', '--cached', '--check')
$stagedNames = @(& $git -C $repoRoot diff --cached --name-only)
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect staged changes.' }
if ($stagedNames.Count -gt 0) {
    Invoke-Native $git @('-C', $repoRoot, 'commit', '-m', $CommitMessage)
}
else {
    Write-Host 'No repository changes needed a new commit.'
}

Invoke-Native $git @('-C', $repoRoot, 'push', 'origin', $Branch)
$head = (& $git -C $repoRoot rev-parse HEAD).Trim()
$tagExists = $false
& $git -C $repoRoot rev-parse --verify --quiet "refs/tags/$Version" *> $null
if ($LASTEXITCODE -eq 0) { $tagExists = $true }

if ($tagExists) {
    $tagCommit = (& $git -C $repoRoot rev-list -n 1 $Version).Trim()
    if ($tagCommit -ne $head) {
        if (-not $MoveExistingTag) {
            throw "Tag $Version points to $tagCommit, not HEAD $head. Re-run with -MoveExistingTag to update it."
        }
        Invoke-Native $git @('-C', $repoRoot, 'tag', '-f', '-a', $Version, '-m', "$ReleaseTitle $Version")
        Invoke-Native $git @('-C', $repoRoot, 'push', '--force', 'origin', "refs/tags/$Version")
    }
}
else {
    Invoke-Native $git @('-C', $repoRoot, 'tag', '-a', $Version, '-m', "$ReleaseTitle $Version")
    Invoke-Native $git @('-C', $repoRoot, 'push', 'origin', "refs/tags/$Version")
}

& $gh release view $Version --repo scottb613/TSRE5-SCOmod *> $null
if ($LASTEXITCODE -eq 0) {
    Invoke-Native $gh @('release', 'edit', $Version, '--repo', 'scottb613/TSRE5-SCOmod',
        '--title', $ReleaseTitle, '--notes-file', $releaseNotes, '--latest')
    Invoke-Native $gh @('release', 'upload', $Version, $releaseZip, '--clobber',
        '--repo', 'scottb613/TSRE5-SCOmod')
}
else {
    Invoke-Native $gh @('release', 'create', $Version, $releaseZip,
        '--repo', 'scottb613/TSRE5-SCOmod', '--verify-tag', '--title', $ReleaseTitle,
        '--notes-file', $releaseNotes, '--latest')
}

Write-Step 'Verifying uploaded GitHub release asset'
$releaseJson = & $gh release view $Version --repo scottb613/TSRE5-SCOmod --json assets,tagName,url
if ($LASTEXITCODE -ne 0) { throw 'Could not read the published GitHub release.' }
$release = $releaseJson | ConvertFrom-Json
$asset = @($release.assets | Where-Object { $_.name -eq $zipInfo.Name }) | Select-Object -First 1
if (-not $asset) { throw "GitHub release does not contain $($zipInfo.Name)." }
if ([int64]$asset.size -ne [int64]$zipInfo.Length) {
    throw "Uploaded asset size mismatch: local $($zipInfo.Length), remote $($asset.size)."
}
$expectedDigest = "sha256:$($zipHash.Hash.ToLowerInvariant())"
if (-not $asset.digest -or $asset.digest.ToLowerInvariant() -ne $expectedDigest) {
    throw "Uploaded asset digest mismatch: expected $expectedDigest, received $($asset.digest)."
}

$finalStatus = @(& $git -C $repoRoot status --porcelain=v1)
if ($finalStatus.Count -gt 0) {
    throw "Publication completed, but the repository is not clean:`n$($finalStatus -join "`n")"
}

Write-Host "Published and verified: $($release.url)" -ForegroundColor Green
Write-Host "Commit: $head"
Write-Host "Asset SHA-256: $($zipHash.Hash)"
