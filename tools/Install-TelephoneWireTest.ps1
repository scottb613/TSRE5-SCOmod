param(
    [Parameter(Mandatory)][string]$RoutePath,
    [Parameter(Mandatory)][string]$PackageDirectory,
    [switch]$Remove
)
# Installs/removes exactly one prepared wire span. Never edits existing poles.
$ErrorActionPreference='Stop'
if(Get-Process -Name TSRE5,RunActivity -ErrorAction SilentlyContinue) {
    throw 'Close TSRE and the Open Rails simulation before changing the test world file.'
}
$route=(Resolve-Path -LiteralPath $RoutePath).Path
$package=(Resolve-Path -LiteralPath $PackageDirectory).Path
$manifest=Get-Content -LiteralPath (Join-Path $package 'test-manifest.json') -Raw | ConvertFrom-Json
foreach($name in @($manifest.WorldFile,$manifest.Shape,$manifest.Texture)) {
    if([IO.Path]::GetFileName($name) -ne $name) { throw 'Package names must be plain filenames.' }
}
$worldPath=Join-Path (Join-Path $route 'WORLD') $manifest.WorldFile
$backup=Join-Path $package 'world-before-test.backup.w'
$receipt=Join-Path $package 'installation.json'
$descriptor=[IO.Path]::ChangeExtension($manifest.Shape,'.sd')
$assets=@(
    @{Source=(Join-Path $package $manifest.Shape);Destination=(Join-Path (Join-Path $route 'SHAPES') $manifest.Shape)},
    @{Source=(Join-Path $package $descriptor);Destination=(Join-Path (Join-Path $route 'SHAPES') $descriptor)},
    @{Source=(Join-Path $package $manifest.Texture);Destination=(Join-Path (Join-Path $route 'TEXTURES') $manifest.Texture)}
)
if($Remove) {
    $installed=Get-Content -LiteralPath $receipt -Raw | ConvertFrom-Json
    if($installed.WorldPath -ne $worldPath -or (Get-FileHash -LiteralPath $worldPath).Hash -ne $installed.WorldSha256) {
        throw 'World changed since installation. Refusing to overwrite subsequent route edits.'
    }
    if((Get-FileHash -LiteralPath $backup).Hash -ne $manifest.WorldSha256) { throw 'Backup hash mismatch.' }
    foreach($asset in $assets) {
        if((Get-FileHash -LiteralPath $asset.Destination).Hash -ne (Get-FileHash -LiteralPath $asset.Source).Hash) {
            throw 'Test asset was modified; refusing removal.'
        }
    }
    Copy-Item -LiteralPath $backup -Destination $worldPath -Force
    foreach($asset in $assets) { Remove-Item -LiteralPath $asset.Destination }
    Write-Output 'Test wire removed; original world restored. Existing poles were unchanged.'
    return
}
if((Get-FileHash -LiteralPath $worldPath).Hash -ne $manifest.WorldSha256) {
    throw 'Saved world changed since bake. Regenerate the test before installing.'
}
foreach($asset in $assets) {
    if(!(Test-Path -LiteralPath $asset.Source -PathType Leaf)) { throw 'Package asset missing.' }
    if(Test-Path -LiteralPath $asset.Destination) { throw "Destination already exists: $($asset.Destination)" }
}
$worldBytes=[IO.File]::ReadAllBytes($worldPath)
if($worldBytes.Length -lt 2 -or $worldBytes[0] -ne 255 -or $worldBytes[1] -ne 254) { throw 'Expected a UTF-16LE world file.' }
$world=[IO.File]::ReadAllText($worldPath)
if([regex]::IsMatch($world,"UiD\s*\(\s*$($manifest.WireUid)\s*\)")) { throw 'Wire UID is already in use.' }
$end=$world.LastIndexOf(')')
if($end -lt 0) { throw 'Invalid world file.' }
$fragment=[IO.File]::ReadAllText((Join-Path $package 'wire-world-entry.txt'))
$updated=$world.Insert($end,$fragment)
Copy-Item -LiteralPath $worldPath -Destination $backup -Force
$created=[Collections.Generic.List[object]]::new()
try {
    foreach($asset in $assets) {
        Copy-Item -LiteralPath $asset.Source -Destination $asset.Destination
        $created.Add($asset)
    }
    # The original is backed up before this single world-file replacement.
    [IO.File]::WriteAllText($worldPath,$updated,[Text.Encoding]::Unicode)
    @{WorldPath=$worldPath;WorldSha256=(Get-FileHash -LiteralPath $worldPath).Hash;WireUid=$manifest.WireUid} |
        ConvertTo-Json | Set-Content -LiteralPath $receipt -Encoding utf8
} catch {
    Copy-Item -LiteralPath $backup -Destination $worldPath -Force
    foreach($asset in $created) {
        if((Get-FileHash -LiteralPath $asset.Destination).Hash -eq (Get-FileHash -LiteralPath $asset.Source).Hash) {
            Remove-Item -LiteralPath $asset.Destination
        }
    }
    throw
}
Write-Output "Installed 16 telephone wires as one static shape (UID $($manifest.WireUid)). Existing poles unchanged."
