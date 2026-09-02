# Terrain Improvements User Guide

This guide explains the improved terrain-editing functions in TSRE GenX v0.15,
with emphasis on the **F2 Terrain Mesh** panel, the Conform TDB/RDB and Waterbed
Offset brushes, new terrain keys, native 4 m and 8 m terrain behavior, and the
related F3 and F7 workflows.

> **Work on a backed-up route.** Terrain height, water flags, gaps, and generated
> TERRTEX files are route data. Test unfamiliar settings in a small area, use
> Undo immediately when a result is wrong, and save and reopen before committing
> a large terrain operation.

## What was improved

The GenX terrain work adds or strengthens:

- a reorganized **F2 Terrain Mesh** panel with synchronized numeric fields and
  sliders;
- six height-brush modes, including **Conform TDB/RDB** and **Waterbed Offset**;
- native editing of standard 256-sample/8 m terrain and experimental
  512-sample/4 m terrain;
- tile-border-safe TrackDB/RoadDB conforming;
- configurable TrackDB and RoadDB height bias;
- improved selected-object conforming and smoothing keys;
- F3 Auto Paint for track, roads, objects, and real water edges;
- F7 Water Tools for complete connected waterways;
- safer tile and route TERRTEX reset operations;
- route-wide **Water Tiles Off** cleanup;
- bounded map-overlay loading; and
- transactional terrain saves that do not report success after only part of a
  multi-file terrain change was written.

Terrain Mesh changes physical terrain height, water flags, or gaps. Terrain
Texture changes the painted surface. Water Tools fits and processes connected
water across multiple tiles. Keep those three jobs conceptually separate.

## Quick start: ordinary terrain shaping

1. Back up the route and open it in the Route Editor.
2. Press **F2** to open **Terrain Mesh**.
3. Select **Height +**.
4. Choose a **Tool** such as Add (Simple), Height, or Flatten.
5. Set **Size** and **Intensity**. Enter the target **Height** only when the
   selected tool enables that field.
6. Left-click or left-drag over terrain.
7. For an Add brush, press **Z** to change between the `+` and `-` directions.
8. Release the mouse and use **Ctrl+Z** immediately if the stroke is wrong.
9. Inspect tile borders and nearby scenery, then save with **Shift+Ctrl+S**.
10. Reopen the route and inspect the edited area again.

## F2 Terrain Mesh panel

### Edit Terrain Layers

- **Height +** activates the selected height-brush mode. Left-click paints one
  point; left-drag makes a continuous stroke.
- **Water +** turns water on for terrain patches under the pointer. Press **Z**
  to reverse the terrain-tool direction when water needs to be turned off
  locally.
- **Gaps +** creates terrain gaps under the pointer. Press **Z** to reverse the
  direction when restoring a gap locally.

The button text and editor status indicate the active direction where the tool
uses one. Do not assume that a visible water surface has been assigned simply
because a patch flag was enabled; water corner elevations must also be valid.
Use F7 Water Tools for a connected river or lake workflow.

### Brush Settings

- **Size** controls the brush footprint. The terrain height brush retains a
  comparable world-space radius on 4 m and 8 m grids; the 4 m grid simply has
  more editable posts inside that footprint.
- **Intensity** controls how much of the requested change is applied by each
  pass. Begin low for shaping and increase only when the result is predictable.
- **Height** is enabled for **Height** and **Waterbed Offset**. Its meaning is
  different for those two modes.
- **Tool** selects the height algorithm described below.

The text boxes and sliders are synchronized. Blank, intermediate, or invalid
text no longer silently changes the live brush to an unintended zero-effect
value. Stay within the displayed field ranges.

### Embankment Settings

These settings primarily control track-, road-, object-, and ruler-based
terrain conforming:

- **Size** is the track-bed half-width expressed in native terrain-post
  spacing.
- **Embankment** controls how many metres per post the terrain may fall away
  from a raised track bed. Larger values make a steeper embankment.
- **Cutting** controls how many metres per post the terrain may rise away from
  a track bed cut into high ground. Larger values make a steeper cutting.
- **Max Radius** limits how far the conforming envelope can extend from the
  centreline. It is interpreted using the native terrain spacing.
- **Set Pinpoint** sets Size and Intensity to their minimum values for a very
  small, gentle correction brush.
- **Reset Defaults** restores the configured terrain-tool defaults. It does
  not undo terrain or reset route files.

The **Size** half-width depends on the native mesh:

| Size | 4 m terrain half-width | 8 m terrain half-width |
|---:|---:|---:|
| 1 | 4 m | 8 m |
| 2 | 6 m | 12 m |
| 3 | 8 m | 16 m |
| 4 | 10 m | 20 m |
| 5 | 12 m | 24 m |
| 6 | 14 m | 28 m |
| 7 | 16 m | 32 m |

Sizes 1-3 preserve the familiar 8, 12, and 16 m half-widths on standard 8 m
terrain. Values 4-7 are available for wider work; test them carefully because
they affect a substantially wider standard-grid bed.

### Track Bias

Select **Track Bias...** to set independent:

- **TDB Height Bias (m)**; and
- **RDB Height Bias (m)**.

The allowed range is `-5.00` to `+5.00 m`. A positive value raises conformed
terrain above the database height; a negative value lowers it. Zero is the
normal starting value.

A non-zero bias is intentionally treated as non-standard. The first applicable
conform operation in a session asks for confirmation so an old experimental
bias cannot silently change a route. Use a small negative TDB bias only when a
deliberate rail-to-ground clearance is required, and verify the result under
points, crossings, bridges, and graded curves.

## Height-brush modes

### Add (Simple)

Raises or lowers terrain with a feathered circular brush. Size controls the
radius, Intensity controls the per-pass amount, and **Z** changes between `+`
and `-`. This is the general-purpose shaping brush.

Use repeated low-intensity passes instead of one strong stroke when forming
banks, shallow cuts, or natural transitions.

### Add (Radius)

Changes the centre first and lets surrounding posts follow that new centre
height without overshooting it. This produces a controlled mound or depression
around the pointer rather than independently adding the full amount to every
post.

Use it when the desired result should radiate from one locally established
height. Use Add (Simple) for freehand grading across a broader stroke.

### Height

Moves terrain toward the exact value entered in **Height**. Intensity controls
how quickly a stroke approaches the target. This is useful for platforms,
yards, formation pads, or matching a known elevation.

Start with moderate Size and low Intensity. A hard, high-intensity target can
produce an artificial edge that must then be smoothed.

### Flatten

Finds the average height inside the current brush and moves posts toward that
local average. It does not require a Height value.

Flatten is useful after rough Add work, but repeated strong passes can erase
intentional drainage, superelevation surroundings, banks, or small terrain
features.

### Conform TDB/RDB

Pulls terrain toward the nearest valid TrackDB or RoadDB height inside the
brush. The effect feathers outward according to Size and Intensity and uses the
appropriate TDB or RDB Height Bias.

This brush is intended for spot cleanup beside a known database vector:

1. Confirm that the visible track or road is correctly registered in its
   database.
2. Keep TDB/RDB Height Bias at `0.00` unless a deliberate offset is needed.
3. Choose **Conform TDB/RDB** and activate **Height +**.
4. Start with a small brush and modest Intensity.
5. Paint beside the vector, not across unrelated nearby track or roads.
6. Inspect both sides of a world-tile border when the stroke crosses one.

The brush searches only a bounded distance for a database height. If no valid
TDB/RDB vector is close enough, that sample is unchanged. At junctions,
parallel tracks, or closely adjacent roads, shrink the brush and work in short
strokes so the nearest vector is unambiguous.

GenX performs the distance calculation in a common tiled coordinate frame, so
the conform brush no longer jumps to the wrong position merely because a
stroke or vector crosses a world-tile boundary.

### Waterbed Offset

Moves eligible terrain toward a fixed negative offset below the tile's sloping
water surface. The **Height** field is the offset, not an absolute route
elevation. It must be negative; `-1.00` means a target one metre below the
interpolated water surface at each post.

Only water-enabled terrain patches are changed. Dry patches, invalid water
levels, and unavailable terrain are ignored. Because the target follows all
four water corner elevations, the brush maintains a consistent depth beneath
a sloping river surface instead of cutting one flat bed elevation.

Recommended procedure:

1. Verify that the intended patches already contain the correct water and
   corner elevations.
2. Select **Waterbed Offset** and begin with `-1.00 m`, a moderate Size, and a
   low Intensity.
3. Activate **Height +** and paint over shallow underwater terrain.
4. Inspect banks and islands; the brush protects dry patches, but an incorrectly
   water-enabled bank is eligible for change.
5. Undo immediately if the bed or shoreline is damaged.

For an entire river, use the F7 **Adjust Terrain** operation after processing
the complete Water Ruler. Waterbed Offset is the manual spot-correction brush.

## Selected-object terrain keys

These shortcuts operate in Select/Place context and complement the F2 brush:

| Key | Terrain action |
|---|---|
| **F** | Set/conform terrain to the selected track, object, or ruler using the F2 embankment settings. |
| **Ctrl+F** | Conform along the selected track/road database vector within the current tile. |
| **Shift+F** | Smooth terrain around the selected track, object, or ruler without reapplying the full conform cut. |
| **Ctrl+Z** | Undo the latest supported terrain edit. Use it immediately. |
| **Z** | With a terrain height/water/gap tool active, toggle the brush direction between `+` and `-`. |
| **Alt+A** | Select all terrain texture patches on the currently selected tile. |
| **M** | Toggle saved route map overlays. This changes only the visual guide. |
| **F2** | Open Terrain Mesh tools. |
| **F3** | Open Terrain Texture tools. |
| **F7** | Open Water Tools. |
| **Shift+Ctrl+S** | Save the route. |

Use **F** for a selected authored object or complete track shape, **Ctrl+F** for
the database vector on the current tile, and the **Conform TDB/RDB** brush for
small freehand repairs. At a tile edge, inspect the neighboring tile after all
three workflows.

## Native 8 m and experimental 4 m terrain

A detailed terrain tile is 2,048 m across:

- standard terrain uses 256 samples at 8 m spacing; and
- experimental detailed terrain uses 512 samples at 4 m spacing.

The 4 m grid supplies four times as many terrain cells per tile and permits
finer cuts, banks, shorelines, and object fitting. It also uses more memory and
requires more work for the same world area.

GenX v0.15 supports 4 m editing across height brushes, selected-object
conforming, Undo, gaps, patch outlines, coordinate math, rendering, and Water
Tools. Operations iterate the native posts rather than assuming every tile is
an 8 m grid. Track-conform bed width and influence radius also derive from the
native spacing.

Practical rules:

- Do not assume the same Embankment Size number represents the same physical
  half-width on both grids; use the table above.
- Height-brush world coverage remains comparable, but 4 m terrain changes more
  posts inside that area.
- Inspect every boundary between edited tiles. Shared edge posts are kept in
  agreement, but an unavailable or unsupported neighbor cannot be corrected.
- F7 Water Tools supports native 4 m and 8 m processing, but rejects an unsafe
  mixed-grid corridor rather than producing a mismatched shoreline.
- Treat 4 m terrain as experimental route content and verify simulator and
  route-tool compatibility on a copy before adopting it broadly.

## F3 terrain texture improvements related to terrain work

Press **F3** for Terrain Texture. Texture painting is separate from height
editing, but these functions help finish the terrain surface:

- **Auto Paint > Selected Object** paints along the selected object.
- **Auto Paint > Nearest Object** uses the nearest applicable object.
- **Auto Paint > Nearest Track or Road** follows nearby database geometry.
- **Auto Paint > Nearest TDB/RDB Vector** follows the nearest complete database
  vector operation.
- **Auto Paint > Track on Tile** paints all discovered track sections on the
  pointer tile.
- **Auto Paint > Roads on Tile** paints all discovered road sections on the
  pointer tile.
- **Auto Paint > Water on Tile** detects crossings between native terrain and
  the actual sloping water plane, then paints the shoreline with the active
  texture brush.
- **Reset Tile Paint** restores the pointer tile's terrain paint after a
  destructive confirmation and removes its generated per-tile texture files.

The Auto Paint menu appears when a Color or Texture paint tool is active and
the viewport is right-clicked. Water on Tile follows actual terrain/water
crossings, including shallow breaks inside a water patch; it does not merely
paint the border of each coarse patch.

Lock any terrain texture patches that an Auto Paint pass must leave unchanged.
Review the active texture, brush Size, Intensity, seasonal set, and Mirror
Season state before a tile-wide operation. For paired main/snow painting, see
[Seasonal Mirror Terrain Painting User Guide](SEASONAL-MIRROR-PAINTING-USER-GUIDE.md).

## F7 Water Tools versus F2 water functions

Use the tool that matches the job:

| Job | Preferred tool |
|---|---|
| Toggle one local water patch | F2 **Water + / -** |
| Lower a small shallow spot below existing water | F2 **Waterbed Offset** |
| Paint an existing tile's detected shoreline | F3 **Auto Paint > Water on Tile** |
| Create or refit a connected multi-tile river/lake | F7 **Process Water Tiles** |
| Lower shallow terrain beneath a processed complete waterway | F7 **Adjust Terrain** |

F7 Water Tools supports one complete long river pass: trace the full Water
Ruler, run Process Water Tiles once, and run Adjust Terrain once if necessary.
It reconciles shared wet corners and uses native-grid shoreline tests. See
[Water Ruler and Water Tools User Guide](WATER-RULER-USER-GUIDE.md) for the
complete procedure.

## Cleanup and reset functions

### Reset Tile Paint

Available from the viewport context menu while a texture paint tool is active.
It requires pending changes to be saved first, asks for confirmation, resets
the pointer tile's painted materials, and deletes matching generated per-tile
texture files. This is not the same as Undo.

### Reset All TERRTEX

The shared **Hacks** cleanup control is route-wide and destructive. It:

- resets detailed and distant terrain patches to `terrain.ace`;
- collapses obsolete material-table entries;
- removes matching generated per-tile TERRTEX files from seasonal folders; and
- removes saved terrain-map files.

It refuses to start while route changes are pending, asks for destructive
confirmation, and cannot be undone in TSRE. Back up the route first.

### Water Tiles Off

The shared **Hacks** cleanup control turns off every water patch across the
route. It does not change terrain heights, textures, gaps, or unrelated patch
flags. It also requires saved work, asks for destructive confirmation, and
cannot be undone in TSRE.

Use it only for a deliberate route-wide water reset, not to correct one local
patch.

## Map overlays and terrain inspection

Saved route maps are visual guides; they do not replace live terrain, TrackDB,
RoadDB, or water geometry. Press **M** to toggle them.

GenX keeps saved overlays lazy and bounded to the camera Tile LOD instead of
loading the route's entire map collection permanently. Turning overlays off
purges their resident resources. This reduces long-travel memory growth while
retaining the full-resolution guide near the camera.

Always verify a conform or water result with the overlay both on and off. The
map may be stale, while the terrain operation follows current route data.

## Undo, save, and recovery

- One continuous terrain-brush stroke is the safest unit to review and undo.
- Release the left mouse button before using **Ctrl+Z**.
- Undo before switching to unrelated work; do not assume a much older terrain
  operation remains the active Undo state.
- Route-wide reset commands are explicitly not Undo operations.
- Save with **Shift+Ctrl+S** after the result is acceptable.

Terrain saves are transactional across the descriptor, height data, flags,
metadata, and modified seasonal textures involved in the save. If any required
component fails, earlier staged replacements are rolled back and the terrain
remains dirty for another save attempt. Do not close the editor or discard the
route backup until the save succeeds and the route reopens correctly.

## Troubleshooting

| Symptom | Check |
|---|---|
| Height brush appears to do nothing | Confirm Height + is active, Size and Intensity are valid, and the selected Tool is appropriate. Height and Waterbed Offset also require a valid Height value. |
| Add brush moves the wrong direction | Press Z to toggle the terrain brush between `+` and `-`. |
| Conform brush changes nothing | Move closer to a valid TDB/RDB vector, increase Size modestly, and confirm the route database is loaded and correct. |
| Conform follows the wrong nearby line | Reduce Size and use short strokes; inspect parallel track, roads, and junction vectors. |
| Conformed ground is consistently too high or low | Check TDB and RDB Height Bias. Return them to `0.00` unless the offset is intentional. |
| A conform result breaks at a tile edge | Confirm the neighbor terrain is present and supported, Undo, then retry with both tiles available. |
| Waterbed Offset changes nothing | The pointer may be on a dry patch, the water corner levels may be invalid, or Height is not a negative value. |
| Waterbed Offset damages a bank | The bank's patch is water-enabled. Undo, correct the water mask or use a smaller brush, then retry. |
| 4 m terrain looks correct but another tool fails | The other tool may not support experimental 512-sample content. Preserve the backup and verify the complete route-tool chain before continuing. |
| Auto Paint misses a tile | Confirm the appropriate track/road database or water geometry exists on the pointer tile and that the active texture brush is valid. |
| Cannot run Reset Tile, Reset All TERRTEX, or Water Tiles Off | Save or settle all pending route changes first. |
| Save fails after a terrain operation | Read the reported file error, keep the route open and dirty, correct the path/permission/storage problem, and retry. The transactional save should not be treated as complete until it reports success. |

## Safe production sequence

1. Back up the route.
2. Confirm whether the work is on standard 8 m or experimental 4 m terrain.
3. Set F2 brush and embankment values for that native spacing.
4. Keep TDB/RDB Height Bias at zero unless a deliberate offset is required.
5. Make one small test stroke or one selected-object conform operation.
6. Inspect the centre, shoulders, scenery, and every affected tile border.
7. Undo immediately if the result is wrong; adjust one setting at a time.
8. Complete the terrain height work before applying final F3 surface paint.
9. Use F7 for a complete connected waterway rather than repeating local patch
   operations across a long river.
10. Save the route and confirm that no terrain save error remains.
11. Reopen and inspect the terrain, water, texture, tile borders, and route in
   the target simulator.
