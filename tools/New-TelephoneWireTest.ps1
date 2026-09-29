param(
    [Parameter(Mandatory)][string]$WorldFile,
    [Parameter(Mandatory)][string]$PoleShape,
    [Parameter(Mandatory)][int]$FirstUid,
    [Parameter(Mandatory)][int]$SecondUid,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [double]$SagPercent = 1.5,
    [double]$WidthMm = 20,
    [double]$MaxSpan = 75,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$BaseName = 'SCO_TelephoneWire_OR_Test'
)
# One-span Open Rails telephone-wire test. Does not modify a route or compile code.
# Uses the existing text-shape writer structure, with independent wire ownership.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Numerics
$culture = [Globalization.CultureInfo]::InvariantCulture
function Number([string]$value) { [double]::Parse($value, $culture) }
function Numbers([string]$value) { @($value.Trim() -split '\s+' | ForEach-Object { Number $_ }) }
function Vector([double]$x, [double]$y, [double]$z) { [Numerics.Vector3]::new($x,$y,$z) }
function FormatNumber([double]$value) { $value.ToString('0.########', $culture) }
function FormatVector([Numerics.Vector3]$value) { "$(FormatNumber $value.X) $(FormatNumber $value.Y) $(FormatNumber $value.Z)" }
function ShapeVector([Numerics.Vector3]$value) { Vector $value.X $value.Y (-$value.Z) }
if($FirstUid -eq $SecondUid -or $SagPercent -lt 0 -or $SagPercent -gt 10 -or $WidthMm -le 0) { throw 'Invalid span settings.' }
$world = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $WorldFile))
$shape = [IO.File]::ReadAllText((Resolve-Path -LiteralPath $PoleShape))
$shapeName = [IO.Path]::GetFileName($PoleShape)
$poles = @{}
foreach($match in [regex]::Matches($world, '(?ms)^\s*Static\s*\((.*?)^\s*\)')) {
    $body = $match.Groups[1].Value
    $idMatch = [regex]::Match($body, 'UiD\s*\(\s*(\d+)')
    if(!$idMatch.Success) { continue }
    $id = [int]$idMatch.Groups[1].Value
    if($id -ne $FirstUid -and $id -ne $SecondUid) { continue }
    $name = [regex]::Match($body, 'FileName\s*\(\s*"?([^\)"\r\n]+)').Groups[1].Value.Trim()
    if($name -ine $shapeName) { throw "Pole $id has a different shape: $name" }
    $position = Numbers ([regex]::Match($body, 'Position\s*\(([^)]+)').Groups[1].Value)
    $rotation = Numbers ([regex]::Match($body, 'QDirection\s*\(([^)]+)').Groups[1].Value)
    if($position.Count -ne 3 -or $rotation.Count -ne 4) { throw "Invalid pole $id transform." }
    $q = [Numerics.Quaternion]::Normalize([Numerics.Quaternion]::new($rotation[0],$rotation[1],-$rotation[2],$rotation[3]))
    $poles[$id] = @{ Position = (Vector $position[0] $position[1] (-$position[2])); Quaternion=$q; SavedPosition=$position }
}
if($poles.Count -ne 2) { throw 'Both specified poles must be present in this saved world file.' }
# This test accepts the current model's direct-child SNAP matrices only.
$matrixNames = @([regex]::Matches($shape,'matrix\s+([^\s(]+)\s*\(') | ForEach-Object {$_.Groups[1].Value})
$hierarchy = Numbers ([regex]::Match($shape,'hierarchy\s*\(([^)]+)').Groups[1].Value)
$local = @{}
foreach($match in [regex]::Matches($shape, '(?i)matrix\s+SNAP_([1-9][0-9]*)\s*\(([^)]+)')) {
    $id = [int]$match.Groups[1].Value
    if($local.ContainsKey($id)) { throw 'Duplicate SNAP attachment number.' }
    $matrixIndex = [array]::FindIndex($matrixNames, [Predicate[string]]{ param($name) $name -ieq "SNAP_$id" })
    if($hierarchy.Count -le $matrixIndex + 1 -or $hierarchy[$matrixIndex+1] -ne 0) { throw 'Test requires SNAP matrices directly under MAIN.' }
    $values = Numbers $match.Groups[2].Value
    if($values.Count -ne 12) { throw 'Invalid attachment matrix.' }
    $local[$id] = Vector $values[9] $values[10] $values[11]
}
if($local.Count -eq 0) { throw 'Test requires numbered SNAP_1, SNAP_2, ... attachments.' }
$attachmentIds = @($local.Keys | Sort-Object)
$main = Numbers ([regex]::Match($shape,'matrix\s+MAIN\s*\(([^)]+)').Groups[1].Value)
if(($main -join ',') -ne '1,0,0,0,1,0,0,0,1,0,0,0') { throw 'Test requires an identity MAIN matrix.' }
$origin = $poles[$FirstUid].Position
$delta = $poles[$SecondUid].Position - $origin
$spanLength = [math]::Sqrt($delta.X*$delta.X + $delta.Z*$delta.Z)
if($spanLength -lt 1 -or $spanLength -gt $MaxSpan) { throw "Span $spanLength is outside 1..$MaxSpan metres." }
$pointsA = @{}; $pointsB = @{}; $mapping = @{}
foreach($id in $attachmentIds) {
    # TSRE mirrors shape X and applies object Y=180; together these mirror Z.
    $attachment = ShapeVector $local[$id]
    $pointsA[$id] = [Numerics.Vector3]::Transform($attachment,$poles[$FirstUid].Quaternion)
    $pointsB[$id] = [Numerics.Vector3]::Transform($attachment,$poles[$SecondUid].Quaternion) + $delta
    $mapping[$id]=$id
}
$remaining = @($attachmentIds | Sort-Object { $local[$_].Y })
while($remaining.Count) {
    $height = $local[$remaining[0]].Y
    $row = @($remaining | Where-Object {[math]::Abs($local[$_].Y-$height) -lt 0.05} | Sort-Object {$local[$_].X})
    $remaining = @($remaining | Where-Object {$_ -notin $row})
    $straight=0.0; $reverse=0.0
    for($i=0;$i -lt $row.Count;$i++) {
        $straight += [Numerics.Vector3]::DistanceSquared($pointsA[$row[$i]],$pointsB[$row[$i]])
        $reverse += [Numerics.Vector3]::DistanceSquared($pointsA[$row[$i]],$pointsB[$row[$row.Count-1-$i]])
    }
    if($reverse + 0.001 -lt $straight) { for($i=0;$i -lt $row.Count;$i++) { $mapping[$row[$i]]=$row[$row.Count-1-$i] } }
}
$vertices = [Collections.Generic.List[object]]::new()
function AddTriangle($a,$b,$c) {
    $normal = [Numerics.Vector3]::Normalize([Numerics.Vector3]::Cross($b-$a,$c-$a))
    foreach($point in @($a,$b,$c)) { $vertices.Add(@{Point=(ShapeVector $point);Normal=(ShapeVector $normal)}) }
    foreach($point in @($c,$b,$a)) { $vertices.Add(@{Point=(ShapeVector $point);Normal=(ShapeVector (-$normal))}) }
}
function AddSolidTriangle($a,$b,$c,$outward) {
    $normal = [Numerics.Vector3]::Normalize([Numerics.Vector3]::Cross($b-$a,$c-$a))
    if([Numerics.Vector3]::Dot($normal,$outward) -lt 0) {
        $swap=$b; $b=$c; $c=$swap; $normal=-$normal
    }
    foreach($point in @($a,$b,$c)) { $vertices.Add(@{Point=(ShapeVector $point);Normal=(ShapeVector $normal)}) }
}
$lodMeshes = @()
foreach($lod in 0..1) {
$meshStart = $vertices.Count
foreach($id in $attachmentIds) {
    $a=$pointsA[$id]; $b=$pointsB[$mapping[$id]]
    $horizontal=Vector ($b.X-$a.X) 0 ($b.Z-$a.Z)
    $length=$horizontal.Length(); $sag=$length*$SagPercent/100
    $side=[Numerics.Vector3]::Normalize([Numerics.Vector3]::Cross($horizontal,(Vector 0 1 0)))
    # Fixed ten-segment fixture for visual evaluation of sag and silhouette.
    $segments=if($lod -eq 0) { 10 } else { 5 }
    if($lod -eq 0) {
        $rings=@(); $centers=@()
        for($i=0;$i -le $segments;$i++) {
            $t=[double]$i/$segments
            $center=[Numerics.Vector3]::Lerp($a,$b,$t)-(Vector 0 (4*$sag*$t*(1-$t)) 0)
            $tangent=($b-$a)-(Vector 0 (4*$sag*(1-2*$t)) 0)
            $up=[Numerics.Vector3]::Normalize([Numerics.Vector3]::Cross($side,$tangent))
            $ring=@()
            for($j=0;$j -lt 3;$j++) {
                $angle=2*[math]::PI*$j/3
                $ring += $center+($side*[float][math]::Cos($angle)+$up*[float][math]::Sin($angle))*[float]($WidthMm/2000)
            }
            $rings += ,$ring; $centers += $center
        }
        for($i=0;$i -lt $segments;$i++) {
            for($j=0;$j -lt 3;$j++) {
                $k=($j+1)%3
                $outward=($rings[$i][$j]+$rings[$i][$k])*0.5-$centers[$i]
                AddSolidTriangle $rings[$i][$j] $rings[$i+1][$j] $rings[$i+1][$k] $outward
                AddSolidTriangle $rings[$i][$j] $rings[$i+1][$k] $rings[$i][$k] $outward
            }
        }
        AddSolidTriangle $rings[0][0] $rings[0][1] $rings[0][2] ($centers[0]-$centers[1])
        AddSolidTriangle $rings[$segments][0] $rings[$segments][1] $rings[$segments][2] ($centers[$segments]-$centers[$segments-1])
        continue
    }
    for($i=0;$i -lt $segments;$i++) {
        $t=[double]$i/$segments; $u=[double]($i+1)/$segments
        $p=[Numerics.Vector3]::Lerp($a,$b,$t)-(Vector 0 (4*$sag*$t*(1-$t)) 0)
        $q=[Numerics.Vector3]::Lerp($a,$b,$u)-(Vector 0 (4*$sag*$u*(1-$u)) 0)
        $up=[Numerics.Vector3]::Normalize([Numerics.Vector3]::Cross($side,$q-$p))
        # Distant geometry is one upright ribbon following the sag arc.
        $axes=if($lod -eq 0) { @($side,$up) } else { ,(Vector 0 1 0) }
        foreach($axis in $axes) {
            $radius=$axis*[float]($WidthMm/2000)
            AddTriangle ($p-$radius) ($q-$radius) ($q+$radius)
            AddTriangle ($p-$radius) ($q+$radius) ($p+$radius)
        }
    }
}
$lodMeshes += @{Start=$meshStart;Count=($vertices.Count-$meshStart);Distance=$(if($lod -eq 0){300}else{750})}
}
$out = [IO.Path]::GetFullPath($OutputDirectory)
[IO.Directory]::CreateDirectory($out) | Out-Null
$textureName='SCO_TelephoneWire_OR_Test.dds'
$count=$vertices.Count; $triangles=$count/3
$radius=0.0
foreach($v in $vertices) { $radius=[math]::Max($radius,$v.Point.Length()) }
$text=[Text.StringBuilder]::new()
function Line([string]$value) { [void]$text.Append($value+"`r`n") }
Line 'SIMISA@@@@@@@@@@JINX0s1t______'
Line 'shape ('
Line ' shape_header ( 00000000 00000000 )'
Line " volumes ( 1 vol_sphere ( vector ( 0 0 0 ) $(FormatNumber ($radius*1.05)) ) )"
# Solid telephone wire uses ordinary diffuse lighting, not vegetation or alpha blending.
# Future fence textures need their own alpha-capable material selection.
Line ' shader_names ( 1 named_shader ( TexDiff ) )'
Line ' texture_filter_names ( 1 named_filter_mode ( MipLinear ) )'
Line " points ( $count"
foreach($v in $vertices) { Line " point ( $(FormatVector $v.Point) )" }; Line ' )'
Line ' uv_points ( 1 uv_point ( 0.5 0.5 ) )'
Line " normals ( $count"
foreach($v in $vertices) { Line " vector ( $(FormatVector $v.Normal) )" }; Line ' )'
Line ' sort_vectors ( 1 vector ( 0 0 0 ) ) colours ( 0 )'
Line ' matrices ( 1 matrix MAIN ( 1 0 0 0 1 0 0 0 1 0 0 0 ) )'
Line " images ( 1 image ( $textureName ) )"
Line ' textures ( 1 texture ( 0 0 0 ff000000 ) ) light_materials ( 0 )'
Line ' light_model_cfgs ( 1 light_model_cfg ( 00000000 uv_ops ( 1 uv_op_copy ( 1 0 ) ) ) )'
# Open Rails -6 selects the broader specular-25 highlight.
Line ' vtx_states ( 1 vtx_state ( 00000000 0 -6 0 00000002 ) )'
Line ' prim_states ( 1 prim_state telephone_wire ( 00000000 0 tex_idxs ( 1 0 ) 0 0 0 0 1 ) )'
Line ' lod_controls ( 1 lod_control ( distance_levels_header ( 0 ) distance_levels ( 3'
foreach($mesh in $lodMeshes) {
$meshCount=$mesh.Count; $meshTriangles=$meshCount/3
Line " distance_level ( distance_level_header ( dlevel_selection ( $($mesh.Distance) ) hierarchy ( 1 -1 ) )"
Line ' sub_objects ( 1 sub_object ('
Line ' sub_object_header ( 00000400 -1 -1 000001d2 000001c4'
Line " geometry_info ( $meshTriangles 1 0 $meshCount 0 0 1 0 0 0"
Line " geometry_nodes ( 1 geometry_node ( 1 0 0 0 0 cullable_prims ( 1 $meshTriangles $meshCount ) ) )"
Line ' geometry_node_map ( 1 0 ) ) subobject_shaders ( 1 0 ) subobject_light_cfgs ( 1 0 ) 0 )'
Line " vertices ( $meshCount"
for($i=0;$i -lt $meshCount;$i++) { $globalIndex=$mesh.Start+$i; Line " vertex ( 00000000 $globalIndex $globalIndex FFFFFFFF FF000000 vertex_uvs ( 1 0 ) )" }
Line " ) vertex_sets ( 1 vertex_set ( 0 0 $meshCount ) )"
Line ' primitives ( 2 prim_state_idx ( 0 ) indexed_trilist ('
Line " vertex_idxs ( $meshCount $((0..($meshCount-1)) -join ' ') )"
Line " normal_idxs ( $meshTriangles $((@('0 3') * $meshTriangles) -join ' ') )"
Line " flags ( $meshTriangles $((@('00000000') * $meshTriangles) -join ' ') ) ) )"
Line ' ) ) )'
}
# Keep a valid final sub-object for OR, whose viewing extension can retain the last LOD.
# All three indices reference the same point: no rasterized area and no visible wire.
Line ' distance_level ( distance_level_header ( dlevel_selection ( 2000 ) hierarchy ( 1 -1 ) )'
Line ' sub_objects ( 1 sub_object ('
Line ' sub_object_header ( 00000400 -1 -1 000001d2 000001c4'
Line ' geometry_info ( 1 1 0 3 0 0 1 0 0 0'
Line ' geometry_nodes ( 1 geometry_node ( 1 0 0 0 0 cullable_prims ( 1 1 3 ) ) )'
Line ' geometry_node_map ( 1 0 ) ) subobject_shaders ( 1 0 ) subobject_light_cfgs ( 1 0 ) 0 )'
Line ' vertices ( 3'
for($i=0;$i -lt 3;$i++) { Line ' vertex ( 00000000 0 0 FFFFFFFF FF000000 vertex_uvs ( 1 0 ) )' }
Line ' ) vertex_sets ( 1 vertex_set ( 0 0 3 ) )'
Line ' primitives ( 2 prim_state_idx ( 0 ) indexed_trilist ('
Line ' vertex_idxs ( 3 0 1 2 ) normal_idxs ( 1 0 3 ) flags ( 1 00000000 ) ) )'
Line ' ) ) ) ) ) ) )'
$shapeText=$text.ToString()
if(([regex]::Matches($shapeText,'\(')).Count -ne ([regex]::Matches($shapeText,'\)')).Count) { throw 'Unbalanced generated shape.' }
[IO.File]::WriteAllText((Join-Path $out "$baseName.s"),$shapeText,[Text.Encoding]::Unicode)
$descriptor="SIMISA@@@@@@@@@@JINX0t1t______`r`nshape ( $baseName.s ESD_Detail_Level ( 0 ) ESD_Alternative_Texture ( 0 ) )`r`n"
[IO.File]::WriteAllText((Join-Path $out "$baseName.sd"),$descriptor,[Text.Encoding]::Unicode)
# Tiny solid dark texture in uncompressed BGRA DDS, alpha=255. No texture artwork required.
$stream=[IO.File]::Create((Join-Path $out $textureName)); $writer=[IO.BinaryWriter]::new($stream)
try {
    $writer.Write([Text.Encoding]::ASCII.GetBytes('DDS '))
    # Explicit complete mip chain: some OR decoders upload zero levels when count=0.
    foreach($v in @(124,0x2100F,4,4,16,0,3)+(@(0)*11)+@(32,0x41,0,32,0x00FF0000,0x0000FF00,0x000000FF,4278190080,0x401008,0,0,0,0)) { $writer.Write([uint32]$v) }
    foreach($pixels in @(16,4,1)) {
        for($i=0;$i -lt $pixels;$i++) { $writer.Write([byte[]]@(64,62,60,255)) }
    }
} finally { $writer.Dispose() }
$position=$poles[$FirstUid].SavedPosition
$nextUid=1+([regex]::Matches($world,'UiD\s*\(\s*(\d+)') | ForEach-Object {[int]$_.Groups[1].Value} | Measure-Object -Maximum).Maximum
$fragment="`tStatic (`r`n`t`tUiD ( $nextUid )`r`n`t`tFileName ( $baseName.s )`r`n`t`tStaticFlags ( 00010000 )`r`n`t`tPosition ( $(($position | ForEach-Object {FormatNumber $_}) -join ' ') )`r`n`t`tQDirection ( 0 0 0 1 )`r`n`t)`r`n"
[IO.File]::WriteAllText((Join-Path $out 'wire-world-entry.txt'),$fragment,[Text.Encoding]::Unicode)
$manifest=@{WorldFile=[IO.Path]::GetFileName($WorldFile);WorldSha256=(Get-FileHash -LiteralPath $WorldFile).Hash;FirstUid=$FirstUid;SecondUid=$SecondUid;WireUid=$nextUid;SpanMetres=$spanLength;SagPercent=$SagPercent;WidthMm=$WidthMm;WireCount=$local.Count;Triangles=$triangles;Shape="$baseName.s";Texture=$textureName;Mapping=$mapping}
$manifest.Mapping = @($attachmentIds | ForEach-Object { @{From=$_;To=$mapping[$_]} })
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out 'test-manifest.json') -Encoding utf8
$manifest | ConvertTo-Json -Depth 4
