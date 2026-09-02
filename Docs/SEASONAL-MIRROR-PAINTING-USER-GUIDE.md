# Seasonal Mirror Terrain Painting User Guide

This is the quick operating guide for paired seasonal terrain painting in the
TSRE GenX Route Editor. **Mirror Season** applies each terrain-texture stroke to
both the route's main `TERRTEX` set and its `TERRTEX/SNOW` set in one operation,
using the matching source texture for each side.

> **Work on a backed-up route.** Terrain painting changes the route's generated
> per-tile ACE textures. Test a small area, use Undo immediately if a stroke is
> wrong, and save and reopen the route before committing a large paint job.

## Quick start

1. Make sure the route contains a valid same-name texture pair, for example:
   - `ROUTES/<route>/TERRTEX/grass.ace`
   - `ROUTES/<route>/TERRTEX/SNOW/grass.ace`
2. Open the Route Editor and press **F3** to show **Terrain Texture** tools.
3. In **Texture Set**, choose the side from which to work: **Summer** for main
   TERRTEX or **Winter** for snow TERRTEX.
4. Select **Load...** and load the source texture for the displayed side, or
   select it from **Recent** or a route paint preset.
5. Confirm that both **MAIN TERRTEX** and **SNOW TERRTEX** previews show the
   intended same-name pair.
6. Select **Texture**, then select **Mirror Season**. Both preview borders
   become active when the pair is ready.
7. Set brush **Size**, **Intensity**, **Rotation**, and brush shape.
8. Left-click or left-drag over terrain. Each stroke paints the visible side
   and its paired side in the same operation.
9. Inspect the visible result. Use **Ctrl+Z** immediately if the latest stroke
   is wrong.
10. Save the route with **Shift+Ctrl+S**. Change Texture Set to the other side,
    inspect it, then save and reopen for a final check.

## What Mirror Season paints

Mirror Season has two terrain-paint outputs:

| Displayed Texture Set | Visible paint target | Mirrored paint target |
|---|---|---|
| Summer, Spring, Autumn, or Night | `TERRTEX` | `TERRTEX/SNOW` |
| Winter | `TERRTEX/SNOW` | `TERRTEX` |

The operation uses the same brush position, shape, size, intensity, and
rotation on both sides. It uses the main source image for the main output and
the snow source image for the snow output.

Spring, Autumn, and Night are editor display choices with seasonal lookup and
fallback behavior. Mirror Season does **not** create or paint separate
`TERRTEX/SPRING`, `TERRTEX/AUTUMN`, `TERRTEX/FALL`, or `TERRTEX/NIGHT`
outputs. Seasonal object and shape texture fallback is also separate from this
terrain-paint feature.

## Prepare the texture pair

Mirror Season requires both source textures to:

- have exactly the same filename, including extension;
- exist in main `TERRTEX` and `TERRTEX/SNOW` respectively; and
- decode successfully as terrain textures.

For example, loading `TERRTEX/grass.ace` requires a usable
`TERRTEX/SNOW/grass.ace`. Loading `TERRTEX/SNOW/rock.dds` requires a usable
`TERRTEX/rock.dds`.

The Load window accepts multiple selections and supports ACE, DDS, JPG/JPEG,
PNG, BMP, and TGA source files. **Hide Terrtex Textures** hides generated
per-tile files whose names begin with `mosaic` or `-`, making authored paint
sources easier to find. The filter does not delete or disable those generated
files.

Do not rely on a fallback image for mirror painting. The two previews show the
actual files that will supply the paired stroke. Hover a preview to check its
source path or filename.

## Read the paired previews

The **Texture Preview** card shows:

- **MAIN TERRTEX** — the same-name source in the route's main `TERRTEX`
  folder;
- **SNOW TERRTEX** — the same-name source in `TERRTEX/SNOW`;
- **RECENT** — the six most recently selected paint textures; and
- **BRUSH** — the current brush-shape swatch.

With Mirror Season off, the highlighted preview border identifies the current
visible output side. With a valid mirror pair and Mirror Season on, both
previews are highlighted because both outputs will be painted.

If either paired preview is unavailable, Mirror Season cannot be enabled. If
an active pair becomes invalid after another texture is selected, the editor
turns Mirror Season off and briefly reports **No Mirror**. It does not silently
continue as if both sides were being painted.

## Paint from main or snow

### Paint from the main side

1. Save any current terrain changes.
2. Choose **Summer** in Texture Set.
3. Load or select the main texture.
4. Verify both previews and enable **Texture** and **Mirror Season**.
5. Paint normally with the left mouse button.

The visible stroke uses the main source texture. The same stroke is applied
off-screen to `TERRTEX/SNOW` with the paired snow source.

### Paint from the snow side

1. Save any current terrain changes.
2. Choose **Winter** in Texture Set.
3. Load or select the snow texture.
4. Verify both previews and enable **Texture** and **Mirror Season**.
5. Paint normally with the left mouse button.

The visible stroke uses the snow source texture. The same stroke is applied
off-screen to main `TERRTEX` with the paired main source.

The editor blocks a Texture Set change while terrain changes are unsaved. Save
first; this protects the current paint buffers and makes the other side safe to
reload for inspection.

## Brush controls and presets

- **Size** controls the painted footprint.
- **Intensity** controls how strongly each stroke blends the source texture.
- **Rotation** supports the full 0-360 degree range for directional textures.
- The **BRUSH** swatch shows the current brush shape; select it to advance to
  the next available shape.
- **Color** affects color painting and tint behavior; use **Texture** for the
  paired source-image workflow described here.

Route-local paint presets can store the texture path, Size, Intensity, Rotation,
and brush shape. Applying a preset selects its texture and painting controls,
but Mirror Season still validates the current same-name main/snow pair before
it can turn on. Recheck both previews after applying a preset.

**Clear Recent** is a full texture-paint reset. It clears the Recent bank,
active texture and Brush swatches, paired previews and tooltips, the Texture
tool state, and Mirror Season. Load or select a texture again before painting.

## New terrain-patch behavior

The first paint stroke on a terrain patch can create a generated per-tile ACE
file for the visible Texture Set. GenX also creates the missing same-name file
on the opposite main/snow side from that side's route `terrain.ace`, or from
the bundled GenX placeholder if the route copy is unavailable.

This safety behavior occurs even when Mirror Season is off. It prevents a new
painted patch from becoming blank when the other Texture Set is displayed, but
it is **not** mirrored texture painting. Only an enabled, validated Mirror
Season operation applies the selected main and snow source textures to both
generated patches.

Editable 1024-by-1024 TERRTEX files retain their working resolution during
painting; they are not intentionally reduced to the older 256-by-256 size.

## Undo, save, and verify

Treat one continuous left-button stroke as one paint action. If it is wrong,
release the mouse and use **Ctrl+Z** before doing unrelated work. With Mirror
Season active, review the result as a paired operation rather than correcting
only the visible side.

Use this verification sequence:

1. Paint a small recognizable test stroke with Mirror Season on.
2. Inspect the visible Texture Set at normal camera height and close range.
3. Save the route with **Shift+Ctrl+S**.
4. Change Texture Set from Summer to Winter, or Winter to Summer.
5. Confirm the same coverage and brush geometry with the correct seasonal
   source appearance.
6. Save if required, close the route, reopen it, and inspect both sets again.

Route Save writes both modified generated terrain textures as part of the same
terrain save transaction. A save failure must be resolved before treating the
paired result as complete.

## Troubleshooting

| Symptom | Check |
|---|---|
| Mirror Season turns off and reports **No Mirror** | One same-name source is missing, has a different extension/name, or cannot be decoded. Check both preview paths and files. |
| Only one paired preview has an image | Put a valid exact-name counterpart in the other main/snow folder, then reload or reselect the texture. |
| Both previews appear, but only one border is active | Mirror Season is off. Enable it and confirm it remains selected. |
| The opposite season shows a plain placeholder | A new patch counterpart was created for safety, but the stroke was made with Mirror Season off or without a valid pair. Undo or restore as appropriate, fix the pair, and repaint with Mirror Season on. |
| Cannot change Texture Set | Terrain changes are unsaved. Save the route, then switch sets. |
| Wrong seasonal image is painted | Confirm whether Summer/main or Winter/snow is displayed and verify the exact files shown in both previews. |
| A preset will not mirror | Its texture path may not resolve in the current Texture Set, or the same-name counterpart is absent. Reload the intended side and recheck both previews. |
| Recent textures and previews disappeared | Clear Recent reset the complete texture-paint state. Load or select the texture again and re-enable Mirror Season. |
| A 1024 texture appears wrong after save | Stop painting that area, preserve the route backup, and verify the source and generated ACE files before another save; do not substitute a lower-resolution file as a workaround. |

## Safe operating sequence

For production route work, use this order:

1. Back up the route.
2. Prepare and validate every same-name main/snow source pair.
3. Open F3 and choose the Texture Set from which to work.
4. Load the texture and confirm both exact paired previews.
5. Enable Texture and Mirror Season; confirm both preview borders are active.
6. Set Size, Intensity, Rotation, and brush shape.
7. Make and inspect a small paired test stroke.
8. Undo immediately if the test is wrong; otherwise paint the intended area.
9. Save before changing Texture Set.
10. Inspect the complete work in both Summer and Winter.
11. Save, reopen the route, and repeat the two-set inspection.
