# TERRTEX Textures User Guide

TSRE GenX expands terrain texturing into a dedicated **F3 Terrain Texture**
panel with safer texture loading, clearer previews, reusable brush presets,
arbitrary texture rotation, and tile-level Auto Paint for track, roads, and
shorelines.

This guide covers the general TERRTEX workflow. Seasonal pairing is summarized
only where it affects ordinary painting; see the separate
[Seasonal Mirror Painting User Guide](SEASONAL-MIRROR-PAINTING-USER-GUIDE.md)
for the complete Main/Snow procedure.

> **Back up the route first.** Terrain painting modifies terrain tile material
> records and creates or changes route-local TERRTEX files. Tile and route
> reset commands deliberately delete generated texture files.

## What TERRTEX Is

TERRTEX contains the textures used by the terrain tiles of an MSTS/Open Rails
route. The primary folder is:

```text
ROUTES\<route>\TERRTEX
```

The route normally contains reusable source textures such as grass, ballast,
soil, rock, or shoreline textures. When a source texture is painted onto a
terrain patch, TSRE creates or updates a generated, tile-specific ACE swatch
and updates the tile's `.t` material reference.

The `TERRTEX\SNOW` folder supplies the paired winter terrain set. Other
seasonal folders may be used for display fallback, but GenX's paired terrain
painting workflow is specifically Main TERRTEX and Snow TERRTEX.

Do not manually delete generated swatches while their terrain tiles still
refer to them. Use the supplied reset tools when cleanup is required.

## Open the Terrain Texture Panel

1. Load the route in the Route Editor.
2. Press **F3**, or choose `Tools > Terrain Texture`.
3. Press **F3** again to close the panel.

F3 is the texture-painting panel. **F2 Terrain Mesh** contains height,
conforming, waterbed, and embankment tools and is documented in the
[Terrain Improvements User Guide](TERRAIN-IMPROVEMENTS-USER-GUIDE.md).

## The Redesigned F3 Panel

The compact F3 panel is divided into work groups:

1. **Texture Set** — Summer, Spring, Autumn, Winter, or Night display set.
2. **Paint Texture** — Color, Texture, Lock, and Mirror Season modes.
3. **Texture Preview** — Pick, Put, Load, paired Main/Snow previews, Recent
   textures, and the Brush control.
4. **Hide Terrtex Textures** — filters generated tile swatches from the Load
   window.
5. **Presets** — Apply, Save, and Remove route-local paint presets.
6. **Brush Settings** — Color, brush shape, Size, Intensity, and Rotation.

The groups use the same dark panel hierarchy and orange active-button treatment
as the rest of GenX. Controls that are active are visually latched, so the
current editing mode is easier to identify.

## Texture Set

The Texture Set selector changes the editor's active seasonal display:

- Summer
- Spring
- Autumn
- Winter
- Night

Changing the set refreshes loaded terrain and seasonal bindings for route
objects, transfers, Dynamic Track, and generated vegetation so the viewport
uses the closest available route textures. Individual missing seasonal files
fall back to the corresponding root texture when possible rather than rendering
black.

GenX refuses to switch texture sets while terrain tiles have unsaved changes,
including unsaved terrain paint. Save first with **Shift+Ctrl+S**, then change
the set. This prevents a seasonal reload from discarding or confusing the
active paint buffers.

The selected set also determines which side of the paired Main/Snow preview is
active. For detailed mirrored painting, use the dedicated seasonal guide.

## Choose a Painting Mode

Only one editing tool is active at a time.

### Texture

`Texture` is the normal blended terrain-paint tool.

1. Select a valid source texture using Load, Pick, Recent, or a preset.
2. Click `Texture` so it is latched.
3. Adjust Size, Intensity, brush shape, and Rotation.
4. Hold the left mouse button and paint in the 3D viewport.

The brush blends the selected texture into generated patch swatches. It does
not merely change the terrain patch's material name.

### Color

`Color` paints with the selected color instead of sampling a source texture.
Click the Color swatch in Brush Settings to choose the paint color, then drag
with the left mouse button.

Use Color for tinting and localized tonal work. Use Texture when the source
image detail itself must be introduced.

### Lock

`Lock` toggles the texture-paint lock on the clicked terrain patch. Locked
patches are skipped by ordinary painting and automated traces.

Use locks to protect finished areas before running Auto Paint through a dense
junction, station, road crossing, or shoreline. Click the patch again with
Lock active to unlock it.

Locks are an editor working control, not a substitute for saving the route.

### Mirror Season

`Mirror Season` applies the same brush or Auto Paint trace to matching Main and
Snow sources. It activates only when both same-named source textures are
present and decodable. Invalid pairing turns the mode off and reports
`No Mirror` rather than performing a misleading one-sided mirror operation.

For folder preparation, preview-border states, placeholders, and seasonal
save validation, follow the dedicated Seasonal Mirror Painting guide.

## Select and Load Textures

### Load

`Load...` opens a multi-file texture chooser. GenX accepts and validates:

- ACE
- DDS
- JPG/JPEG
- PNG
- BMP
- TGA

You may select several files in one operation. Valid files are added to Recent
history; the last successfully loaded texture becomes the current paint texture
and activates Texture painting. Files that cannot be decoded are named in a
warning instead of becoming an unusable black or stale swatch.

Source images may be convenient for editing, but generated terrain swatches are
saved in the MSTS/Open Rails terrain texture workflow. Verify the final result
in Open Rails rather than assuming an external image extension alone proves
runtime compatibility.

### Hide Terrtex Textures

The `Hide Terrtex Textures` checkbox is enabled by default. In the Load window
it hides generated files whose names begin with `mosaic` or `-`, keeping the
chooser focused on reusable source textures.

Turn it off only when a generated tile swatch must be inspected or deliberately
reused. In most painting workflows it should remain enabled.

### Pick

`Pick` samples the terrain texture under the pointer.

1. Click `Pick`.
2. Click a painted terrain patch in the viewport.
3. GenX assigns the sampled texture to the Brush and enters Texture mode.

Pick is useful when continuing an existing texture treatment without finding
the original source manually.

### Put

`Put` assigns the current paint texture to terrain patches rather than blending
it through the normal paint brush. Click a patch to place it; dragging can put
the texture across additional patches.

While Put is active, its right-click menu provides orientation choices:

- Random
- Present/current orientation
- Rotate 0°
- Rotate 90°
- Rotate 180°
- Rotate 270°

Use Put carefully: it changes the patch texture assignment and is more direct
than blended painting.

## Preview, Recent, and Brush Controls

### Paired source previews

The large side-by-side previews are labeled:

- `MAIN TERRTEX`
- `SNOW TERRTEX`

Each shows the exact decoded source for that side, not a silent seasonal
substitute. Hover a preview to see its source filename. A missing, unreadable,
or incomplete source is shown as unavailable. The border identifies the active
texture set; both borders become active only when a valid mirror pair is in use.

### Recent

`RECENT` holds the six most recently used textures in two rows of three. Click
a Recent swatch to return it to the active paint source. Hover to confirm the
filename before painting.

Recent is history, not the authoritative active state. Always check the large
active-side preview and its filename before an automated operation.

### Brush

The `BRUSH` control is also the brush-shape selector. Clicking it cycles through
the available brush-shape masks. The shape is retained in paint presets. Use
the large Main/Snow preview and filename tooltip to verify the active texture
source.

### Clear Recent

`Clear Recent` performs a complete tool reset:

- clears the six Recent slots;
- clears the active paint texture;
- clears both large previews and their tooltips;
- disables Texture painting if it was active;
- disables Mirror Season and clears its brush state.

This prevents an invisible or stale texture from remaining active after the
history is cleared. It does not erase already painted terrain or delete files.

## Brush Settings

### Size

Size controls the area covered by the brush and the width of automated traces.
The range is 1–99. Begin small in yards and at crossings; increase it for broad
ballast, roadside, or shoreline work.

### Intensity

Intensity controls how strongly each pass blends into the generated patch
texture. The range is 1–99.

Use lower intensity for gradual transitions and repeated feathering. Use higher
intensity for decisive texture replacement. Several light passes are usually
easier to control than one heavy pass.

### Brush shape

The brush-shape mask controls the falloff pattern of manual and automatic
painting. Click the Brush control to cycle shapes. Save commonly used shapes in
presets so track, road, and shoreline passes remain consistent.

### Rotation

Rotation accepts any angle from 0° through 360°. It is useful for directional
textures such as crop rows, plowed fields, drainage patterns, rock strata, and
long ballast detail.

The Put context menu's 0/90/180/270 choices update the same orientation state.
Zero is the normal default. Inspect seams after using non-square or strongly
directional textures.

### Color

The color swatch supplies the Color tool's paint value. It is independent of
the selected source texture but is stored with the active brush state.

## Route-Local Paint Presets

A preset stores:

- preset name;
- texture filename;
- brush size;
- brush intensity;
- brush shape;
- texture rotation.

Presets are separated by MSTS/ORTS root and route and are stored in the user's
TSRE AppData area, not as extra JSON files inside the route. This keeps route
workflows separate without adding editor-only files to a distributable route.

### Save a preset

1. Select a valid texture.
2. Set Size, Intensity, brush shape, and Rotation.
3. Click `Save` in Presets.
4. Enter a name of up to 48 characters.
5. Confirm if replacing an existing same-named preset.

### Apply a preset

1. Select it from the preset list.
2. Click `Apply`.
3. Confirm the large texture preview, brush shape, and settings before painting.

If the stored texture cannot be found or decoded, GenX warns instead of
silently painting with a different brush.

### Remove a preset

Select the preset, click `Remove`, and confirm. Removing a preset does not
delete its source texture or alter terrain already painted with it.

## Auto Paint

Auto Paint follows route geometry using the active Color or Texture brush. It
uses the current texture/color, Size, Intensity, shape, Rotation, locks, and
valid Mirror Season state.

### Open the Auto Paint menu

1. Press **F3**.
2. Select a valid paint texture or color.
3. Activate `Texture` or `Color`.
4. Point at the intended working location or tile.
5. Right-click the 3D viewport.
6. Open the `Auto Paint` submenu.

The menu is available while a paint tool is active.

### Selected Object

Paints along the currently selected line-based object. Select the intended
track or other supported linear object first, then point nearby and choose
`Selected Object`.

Use this when the exact world object is known and nearby parallel geometry
must not be chosen accidentally.

### Nearest Object

Finds the nearest supported line-based object to the pointer and paints along
its line. Use a close camera position and inspect the selection area first in
dense scenes.

### Nearest Track or Road

Finds the nearest applicable TrackDB or RoadDB route at the pointer and paints
along it. This is useful for a local ballast or roadside pass without painting
the rest of the tile.

### Nearest TDB/RDB Vector

Paints the database vector associated with the nearest track or road rather
than limiting the operation to the immediately detected piece. Use it when a
continuous vector should receive one consistent trace.

Inspect junctions and tile boundaries afterward; a database vector can cover
more geometry than one visible world shape.

### Track on Tile

Processes every TrackDB vector section on the **tile under the pointer** and
paints a trace using the active brush.

1. Point into the intended tile.
2. Choose `Auto Paint > Track on Tile`.
3. Read the status message for the tile coordinates and number of processed
   track sections.
4. Inspect yards, turnouts, crossings, and tile edges before continuing.

If no TrackDB sections exist on that tile, GenX reports that nothing was found.

### Roads on Tile

Processes every RoadDB vector section on the **tile under the pointer** using
the same brush workflow as Track on Tile. Only the database source differs.

Use locks or a smaller Size when a road closely parallels track or crosses an
already-finished terrain patch.

### Water on Tile

Water on Tile paints the real shoreline contour rather than filling every
terrain patch marked as water.

For the tile under the pointer, GenX compares detailed terrain elevations with
the tile's sloping water plane and traces locations where terrain crosses the
water surface. This follows exposed shoreline, including shallow-water areas
where terrain rises through the surface.

Before running it:

- make sure the tile has correct water flags and corner elevations;
- select a suitable sand, mud, rock, grass, or bank texture;
- start with moderate Size and low Intensity;
- lock finished patches that must not be touched.

If no water/terrain edges are found, GenX reports that result without painting
the whole water area.

### Auto Paint is intentionally tile-local

`Track on Tile`, `Roads on Tile`, and `Water on Tile` process one pointer tile
per command. Move tile by tile, inspect each result, and save in controlled
stages. This keeps a mistaken brush or an unexpected database branch from
changing the entire route in one operation.

## Alt+A: Select Every Patch on a Tile

Press **Alt+A** after selecting any terrain patch. GenX selects the complete
16×16 patch grid—all 256 patches—on that selected tile.

This is useful for tile-wide terrain-patch operations and inspection. It is not
required by the Auto Paint tile commands, which use the tile under the pointer.
If no terrain patch is selected, Alt+A does nothing.

## Undo and Save

### Undo

Press **Ctrl+Z** after an unwanted manual left-drag paint stroke. A continuous
stroke is grouped by the editor's mouse operation state, and changed texture
buffers are sent to Undo before painting.

Do **not** rely on Ctrl+Z for right-click Auto Paint commands. Those commands do
not establish the same mouse-stroke Undo boundary. Save a known-good checkpoint
before running Track, Road, or Water Auto Paint; if the result is unacceptable,
restore that checkpoint or route backup. Undo also does not apply to the
destructive disk-level reset commands described below.

### Save

Press **Shift+Ctrl+S** to save the route. Terrain paint is part of the route
save transaction: the modified tile records and generated texture files must
remain synchronized.

Recommended save cycle:

1. Paint one controlled area or pointer tile.
2. Inspect at close and distant camera ranges.
3. Undo and correct any overspray, seam, or wrong-source problem.
4. Save the route.
5. Move away far enough to unload/reload the terrain, or close and reopen the
   route.
6. Verify the same texture, rotation, and blend after reload.
7. Test the route in Open Rails.

GenX protects editable terrain buffers from display-quality downsampling, so a
1024×1024 editable TERRTEX swatch is not intentionally saved back at 512×512
merely because a lower viewing texture-quality setting was used.

## Reset Tile Paint

`Reset Tile Paint` appears as a separate right-click command below the Auto
Paint submenu. It resets the **tile under the pointer** to `terrain.ace`, removes
that tile's generated ACE/DDS swatches, and reloads the tile.

This is a destructive disk-level cleanup, not an Undo brush operation.

1. Save or discard every pending route change first. The command refuses to
   run while unsaved route changes exist.
2. Back up the route.
3. Point at the exact tile.
4. Right-click and choose `Reset Tile Paint`.
5. Read the tile coordinates in the confirmation.
6. Confirm only when the entire tile should return to the default material.
7. Review the reported deleted and failed files.

Shared source textures such as `terrain.ace` and `microtex.ace` are not the
generated tile files this command is intended to remove.

## Reset All TERRTEX

The route-wide reset is intentionally kept out of the everyday F3 panel. It is
available from the shared `Hacks`/Route Cleanup controls as `Reset All TERRTEX`.

It resets all route terrain materials, removes generated TERRTEX output and
saved terrain-map imagery covered by the confirmation, and writes affected
tile files directly. It refuses to begin while route changes are pending and
reports read/save failures separately.

Use it only for a deliberate full-route cleanup with a verified backup. It is
not a normal step after experimenting on one tile; use Reset Tile Paint for
that narrower case.

## Recommended Track Auto-Paint Procedure

1. Back up the route and press **F3**.
2. Select the intended Texture Set.
3. Load or Pick the ballast/ground source.
4. Verify the filename in the active-side preview and confirm the brush shape.
5. Activate `Texture`.
6. Choose a moderate Size, low-to-medium Intensity, appropriate shape, and
   correct Rotation.
7. Lock station platforms, roads, or finished terrain patches that must remain
   untouched.
8. Point into the intended tile.
9. Right-click and choose `Auto Paint > Track on Tile`.
10. Inspect every turnout, parallel track, crossing, and tile edge.
11. If the result is wrong, restore the known-good checkpoint, adjust the brush,
    and repeat.
12. Save with **Shift+Ctrl+S**.
13. Reload the route and inspect again in GenX and Open Rails.

## Recommended Shoreline Auto-Paint Procedure

1. Complete and verify the tile's water surface first.
2. Press **F3** and choose a shoreline source texture.
3. Set a modest brush Size and low Intensity.
4. Lock finished patches where the shoreline trace must stop.
5. Point at the water tile.
6. Choose `Auto Paint > Water on Tile`.
7. Inspect the complete water/terrain contour, especially corners and sloping
   water edges.
8. Add restrained manual paint where artistic blending is still needed.
9. Save, reload, and test in Open Rails.

## Troubleshooting

### Texture painting is active but nothing changes

- Confirm that the active-side preview contains a valid texture.
- Make sure `Texture`, not Pick, Put, or Lock, is latched.
- Check whether the terrain patch is locked.
- Increase Size or Intensity slightly.
- Zoom close enough that the pointer is actually on detailed terrain.

### The Load window is missing a generated texture

Clear `Hide Terrtex Textures` temporarily. Generated names beginning with
`mosaic` or `-` are hidden while the filter is enabled.

### A file appears in the folder but will not load

The extension may be supported while the image data is malformed, incomplete,
or undecodable. GenX lists failed filenames. Repair or re-export the source; do
not paint from a black or stale preview.

### A Recent swatch does not match the large preview

Recent is history; the large active-side preview shows the selected paint
source. Click the intended Recent swatch and confirm the large preview and
filename before painting.

### Auto Paint chose the wrong track or object

Restore the pre-Auto-Paint checkpoint. Use `Selected Object` when possible,
point closer, or use a tile-level Track/Road operation after locking nearby
finished patches.

### Track on Tile reports no sections

The pointer may be over an adjacent tile, the relevant database may be RoadDB
rather than TrackDB, or the visible object may not have a valid database vector.
Move the pointer, try Roads on Tile where appropriate, and inspect TrackDB/RoadDB
health before painting manually around a broken database.

### Water on Tile reports no edges

The tile may have no enabled water patches, the water surface may be malformed,
or terrain may not cross the water plane. Verify water flags and elevations
with the F7 Water Tools before retrying.

### Texture rotation looks correct nearby but produces seams

Directional detail may not wrap cleanly at the chosen angle. Return to 0° or a
90° increment, use a seamless source, lower intensity, or manually feather the
patch boundary.

### The texture changes after selecting another season

The selected seasonal file may be missing, causing per-file fallback to the
root texture. Hover the Main/Snow previews, inspect the route folders, save all
terrain changes, and use the dedicated seasonal guide to validate a pair.

### The opposite season contains a default-looking patch

With Mirror Season off, GenX may create a paired placeholder when a new active
side swatch is created. The placeholder prevents a missing/blank seasonal tile;
it is not a mirrored paint result.

### Reset Tile Paint is disabled or refuses to run

Save or discard all pending route changes first. The reset writes terrain files
directly and therefore will not run over an unsaved editing session.

## Final Checklist

- Route backup exists.
- F3, not F2, is the active panel.
- Correct Texture Set is selected.
- Active-side preview filename and brush shape are correct.
- Texture/Color mode is intentional.
- Size, Intensity, shape, and Rotation are appropriate.
- Mirror Season is off unless a validated pair is intentionally required.
- Finished patches are locked where needed.
- Auto Paint is aimed at the correct object, database, or pointer tile.
- Track/Road/Water result has been inspected at boundaries and crossings.
- A known-good save checkpoint exists before each Auto Paint operation.
- Route save succeeds with **Shift+Ctrl+S**.
- Painted terrain reloads correctly in GenX.
- Final appearance is tested in Open Rails.
