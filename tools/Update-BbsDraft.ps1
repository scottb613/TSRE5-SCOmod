#requires -Version 5.1

<#
.SYNOPSIS
Synchronizes the BBCode announcement draft from the authoritative work list.

.DESCRIPTION
Updates only the Highlights and Work Details sections in masterDocs\Forum_NoPush\
bbsTags.txt. The introduction, download text, future-work note, closing, and all
other hand-written announcement material are preserved.
#>
[CmdletBinding()]
param(
    [string]$WorkList,
    [string]$BbsDraft,
    [switch]$Check
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$workspaceRoot = Split-Path -Parent $repoRoot
if (-not $WorkList) { $WorkList = Join-Path $workspaceRoot 'masterDocs\scoWorkList.txt' }
if (-not $BbsDraft) { $BbsDraft = Join-Path $workspaceRoot 'masterDocs\Forum_NoPush\bbsTags.txt' }

foreach ($path in @($WorkList, $BbsDraft)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file was not found: $path" }
}

function Convert-InlineBbs {
    param([Parameter(Mandatory)][AllowEmptyString()][string]$Text)

    $converted = [regex]::Replace($Text, '\[([^\[\]\r\n]+)\]', '[b]$1[/b]')
    $converted = [regex]::Replace($converted, '`([^`\r\n]+)`', '[code]$1[/code]')
    return $converted
}

function Convert-WorkDetailBbs {
    param([Parameter(Mandatory)][string]$Text)

    $output = New-Object System.Collections.Generic.List[string]
    $inList = $false
    $sectionSeen = $false

    foreach ($rawLine in ($Text -split "`n")) {
        $line = $rawLine.TrimEnd("`r")
        if ($line -match '^\s*---\s+(.+?)\s+---\s*$') {
            if ($inList) { $output.Add('[/list]'); $output.Add(''); $inList = $false }
            if ($sectionSeen) { $output.Add('[hr]'); $output.Add(''); $output.Add('') }
            $heading = [regex]::Replace($matches[1], '\[([^\]]+)\]', '$1')
            $output.Add("[b][size=4]$heading[/size][/b]")
            $output.Add('')
            $sectionSeen = $true
            continue
        }

        if ($line -match '^\s*-\s+(.+)$') {
            if (-not $inList) { $output.Add('[list]'); $inList = $true }
            $output.Add('[*]' + (Convert-InlineBbs $matches[1]))
            continue
        }

        if ($inList) {
            $output.Add('[/list]')
            $output.Add('')
            $inList = $false
        }
        $output.Add((Convert-InlineBbs $line))
    }
    if ($inList) { $output.Add('[/list]') }

    while ($output.Count -gt 0 -and [string]::IsNullOrWhiteSpace($output[$output.Count - 1])) {
        $output.RemoveAt($output.Count - 1)
    }
    return ($output -join "`r`n")
}

function Replace-BetweenMarkers {
    param(
        [Parameter(Mandatory)][string]$Text,
        [Parameter(Mandatory)][string]$StartMarker,
        [Parameter(Mandatory)][string]$EndMarker,
        [Parameter(Mandatory)][string]$Replacement
    )

    $start = $Text.IndexOf($StartMarker, [StringComparison]::Ordinal)
    $end = $Text.IndexOf($EndMarker, [StringComparison]::Ordinal)
    if ($start -lt 0 -or $end -lt 0 -or $end -le $start) {
        throw "Could not locate BBCode section markers: $StartMarker / $EndMarker"
    }
    return $Text.Substring(0, $start) + $Replacement + $Text.Substring($end)
}

$workText = [IO.File]::ReadAllText($WorkList)
$bbsText = [IO.File]::ReadAllText($BbsDraft)
$normalizedWork = $workText.Replace("`r`n", "`n")

$highlightsMatch = [regex]::Match($normalizedWork,
    '(?ms)^HIGHLIGHTS\s*\n(?<body>.*?)^={10,}\s*\n\s*GITHUB')
$detailsMatch = [regex]::Match($normalizedWork,
    '(?ms)^WORK DETAIL\s*\n(?<body>.*?)^={10,}\s*\n\s*DISTRIBUTION NOTES')
if (-not $highlightsMatch.Success -or -not $detailsMatch.Success) {
    throw 'Could not find the HIGHLIGHTS and WORK DETAIL boundaries in scoWorkList.txt.'
}

$highlightLines = @($highlightsMatch.Groups['body'].Value -split "`n" |
    Where-Object { $_ -match '^\s*-\s+' } |
    ForEach-Object { '[*]' + (Convert-InlineBbs ($_ -replace '^\s*-\s+', '')) })
$highlightBlock = @(
    '[b][size=4]Highlights[/size][/b]',
    '',
    'The following major updates and fixes are included in TSRE GenX:',
    '',
    '[list]'
) + $highlightLines + @(
    '[/list]',
    '',
    '[hr]',
    '',
    ''
)

$updated = Replace-BetweenMarkers $bbsText '[b][size=4]Highlights[/size][/b]' `
    '[b][size=4]Download and Test Branch[/size][/b]' ($highlightBlock -join "`r`n")

$detailsBody = Convert-WorkDetailBbs $detailsMatch.Groups['body'].Value
$detailsBlock = @(
    '[b][size=4]Work Details[/size][/b]',
    '',
    '[hr]',
    '',
    '',
    $detailsBody,
    '',
    '[hr]',
    '',
    ''
) -join "`r`n"
$updated = Replace-BetweenMarkers $updated '[b][size=4]Work Details[/size][/b]' '[/spoiler]' $detailsBlock

if ($Check) {
    if ($updated -cne $bbsText) {
        throw 'The BBS draft is out of date. Run tools\Update-BbsDraft.ps1 to synchronize it.'
    }
    Write-Host 'BBS draft is synchronized.' -ForegroundColor Green
    return
}

$utf8NoBom = New-Object Text.UTF8Encoding($false)
[IO.File]::WriteAllText($BbsDraft, $updated, $utf8NoBom)
Write-Host "Updated BBS draft: $BbsDraft" -ForegroundColor Green
