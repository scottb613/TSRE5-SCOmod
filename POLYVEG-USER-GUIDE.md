# PolyVeg Planter User Guide

This is the quick operating guide for the TSRE GenX PolyVeg system. PolyVeg
places ordinary static vegetation objects from a route-owned schema, either
inside SCO LIDEX map polygons or inside a ruler drawn in the Route Editor. Raw
objects can then be baked into fewer scenery blocks for better editor and route
performance.

> **Work on a backed-up route.** Planting can be undone before later edits, but
> baking deliberately clears the Undo history. Save and inspect a small test
> area before committing a large planting operation.

## Quick start

1. Run SCO LIDEX **Create Map Tiles** for the route. PolyVeg polygon planting
   requires `osm_data/polyveg-polygons.geojson`; Unrestricted ruler planting
   requires `osm_data/polyveg-exclusions.geojson`.
2. Open the Route Editor and press **Shift+F6** to open the PolyVeg Schema
   Editor.
3. Create a schema, add one or more route `.s` vegetation assets, set their
   size and distribution, and choose **SAVE SCHEMA**.
4. Choose **EXIT** to return to the Route Editor with the **F6 PolyVeg
   Planter** open.
5. Select the schema, Density, Cap, and Seed.
6. For mapped planting, point at a visible map polygon, right-click empty
   terrain, and choose **Plant PolyVeg**.
7. Inspect the raw objects. Use **Undo** immediately if the result is wrong.
8. Save the route after the planting looks right.
9. When ready to commit the result, use **Bake PolyVeg Tile** for the current
   camera tile or **Bake PolyVeg Area** for eligible loaded tiles in the current
   camera LOD. Inspect and save again.

## Required route files

PolyVeg uses these route-local inputs:

- `OpenRails/polyveg.json` — the PolyVeg schema catalog written by the Schema
  Editor.
- `shapes/*.s` — the vegetation shapes referenced by the catalog.
- `osm_data/polyveg-polygons.geojson` — SCO LIDEX contract-v2 planting
  polygons used by pointer and normal ruler planting.
- `osm_data/polyveg-exclusions.geojson` — SCO LIDEX contract-v2 exclusion
  polygons used by the **Unrestricted** ruler option.

If the map data is absent or stale, rerun SCO LIDEX **Create Map Tiles**. The
map imagery is only the visual guide; planting follows the route-local PolyVeg
GeoJSON and the live TSRE terrain and databases.

## Build a schema

Open the full-screen editor with **Shift+F6** or **EDIT SCHEMA** in the F6
panel. A catalog must contain at least one schema, and every saved schema must
contain at least one valid route shape.

### Schema controls

- **NEW** creates a schema with safe starting values.
- **DUPLICATE** copies the current schema, including all assets and settings.
  This is the quickest way to make related mixes such as dense woods, sparse
  woods, and scrub.
- **DELETE** removes the selected schema. The last schema cannot be deleted.
- **Name / Desc** identify the mix in the Planter.
- **RELOAD FROM DISK** discards unsaved editor changes and reloads
  `OpenRails/polyveg.json`.
- **SAVE SCHEMA** validates the whole catalog and writes it atomically. If a
  shape or required value is invalid, the save is blocked and the status area
  explains why.
- **EXIT** returns to the main display and refreshes the F6 schema list.

### Recipe defaults

These become the initial Planter values and the schema-controlled safety
clearances applied to its planting operations:

- **Density** — requested plants per square kilometre for random planting.
- **Cap** — maximum objects allowed in one planting operation. It limits both
  random and row planting.
- **Track clear** — minimum distance from TrackDB geometry.
- **Road clear** — minimum distance from RoadDB geometry.
- **Water clear** — excludes terrain that is below the tile's water surface,
  plus this setback distance. It uses the real terrain and water geometry, not
  the appearance of a map tile.

Clearances reject candidate positions; they do not move existing vegetation.
A larger value creates a wider empty strip.

### Recipe limits

- **Density minimum / maximum** define the range offered by the F6 Planter.
- **Cap minimum / maximum** define the allowed per-operation cap range.

The default value must lie inside its corresponding limits. Limits protect a
schema from accidental extreme settings; they are not extra planting zones.

### Distribution

- **Slope** — steepest terrain on which planting is allowed, in degrees.
- **Feather** — gradually thins planting near every planting boundary. `0`
  keeps a hard edge; a larger value makes a wider, softer transition inward
  from the edge. Feathering is applied when candidates are generated, so bake
  does not expand the planted boundary.
- **Spacing** — minimum centre-to-centre separation. The scaled footprint radii
  of two assets can require an even larger separation.

### Asset library and asset cards

Choose a route shape in **Asset Library** and select **ADD ASSET**. The picker
searches the route `shapes` directory. **BUILD THUMBNAILS** creates cached
previews for the assets in the selected schema; thumbnails are optional and do
not affect planting.

Each asset card controls:

- **Name** — friendly name for the asset.
- **Weight** — relative frequency in the mix. A weight of 2 is selected twice
  as often as a weight of 1; weights do not need to total 100.
- **Radius** — the unscaled footprint radius used to prevent overlapping
  placements. Set it to match the useful width of the model, not its bounding
  box if that contains large empty space.
- **Yaw minimum / maximum** — random rotation range in degrees.
- **Scale minimum / maximum** — random uniform size range. `1.0` is the
  original model size.
- **Plant depth** — optional positive downward offset in metres. `0` leaves the
  shape origin on the terrain; `1.5` sinks it 1.5 m. Do not use a negative
  number. The visual result depends on where the model author placed the
  shape's origin.
- **REMOVE ASSET** — removes only this asset entry from the selected schema; it
  does not delete the `.s` file.

Always test a new asset at a moderate density before planting a large area.
Incorrect Radius, Scale, or Plant depth values become much harder to diagnose
after baking.

## F6 PolyVeg Planter controls

Press **F6** to toggle the Planter.

- **Schema** selects what vegetation mix to plant.
- **Density (km2)** requests random plants per square kilometre within the
  selected schema's limits.
- **Cap** limits this complete planting operation, not each polygon fragment.
- **Seed** selects a repeatable layout. The same schema, boundary, settings,
  and seed reproduce the same candidate pattern; change the seed to rearrange
  it.
- **Flood Fill** plants every matching polygon on the pointer tile with the
  same LIDEX category and fill style. Off plants only the top visible polygon
  beneath the pointer. Both modes are clipped to the pointer tile.
- **Disable Report** suppresses the successful post-plant detail dialog. It
  does not suppress errors, the success sound, or the operation itself.

### Rows

Enable **Rows** for orchards, crops, plantations, or other aligned layouts.

- **Row Width** is the distance between parallel rows.
- **Spacing** is the distance between plants along a row.
- **Direction** rotates the global row grid clockwise from route-plan north.
- **Auto** (`0`) uses the schema's minimum separation for Row Width or
  Spacing.

Rows remain aligned across clipped pieces and neighboring operations because
they use one route-global grid. In row mode, Row Width and Spacing determine
the requested count instead of Density; Cap still applies.

## Plant from SCO LIDEX polygons

1. Open F6 and select the schema and settings.
2. Put the terrain pointer inside the visible polygon to plant.
3. Right-click empty terrain and choose **Plant PolyVeg**.
4. Read the confirmation report. It lists placement totals and rejected
   candidates, including slope, TrackDB, RoadDB, water, and other limitations.
5. Inspect the result from several angles. Use **Undo** before continuing if it
   is unsuitable.

The selected schema decides *what* is planted; the visible top polygon beneath
the pointer decides *where*. A baked tile cannot accept more raw PolyVeg. Delete
its bake first if it must be replanted.

## Plant with a ruler

Rulers are useful for corridors, hand-drawn areas, and places where the map
polygon is unsuitable.

### Corridor

1. Leave **Area** off and set **Ruler Width** to the full corridor width.
2. Select **New Ruler (PolyVeg)**, then click terrain to add at least two
   centreline points.
3. Use **Add Points** to continue the ruler or **Edit Points** to drag its
   orange handles. Moved points snap back to terrain when released.
4. Select **Plant Ruler (PolyVeg)**, review the candidate summary, and confirm.

### Area

1. Enable **Area** and set **Ruler Width**. The clicked polygon interior is
   filled; Width adds an exterior buffer around it. Use `0` for no exterior
   buffer.
2. Select **New Ruler (PolyVeg)** and place at least three boundary points.
3. Keep the light-green closed outline simple—edges must not cross or overlap.
4. Edit as needed, then select **Plant Ruler (PolyVeg)** and confirm.

The PolyVeg ruler is tile-locked to its first point. Long boundaries must be
handled as separate tile operations. Starting a new special water, vegetation,
or grade ruler replaces the existing special ruler.

### Unrestricted — the ruler “hack” option

**Unrestricted** lets the ruler plant outside the normal mapped PolyVeg
coverage. It becomes available only when SCO LIDEX has supplied
`polyveg-exclusions.geojson`.

It is not an “ignore all safety” switch. Unrestricted still retains:

- SCO LIDEX building and developed-land exclusions;
- baked-tile protection;
- schema Slope and Spacing rules;
- TrackDB and RoadDB clearances; and
- submerged-water exclusion plus the schema Water clear setback.

Use it for deliberate manual planting, not as a cure for missing or stale map
data. Confirm the exclusion cache is current before using it near buildings,
roads, track, or water.

## Inspect, jump, and correct raw planting

The Status card reports objects on currently loaded world tiles:

- **RAW OBJECTS** — individual vegetation objects whose shapes are listed in
  the current route catalog.
- **BAKED BLOCKS** — generated PolyVeg block objects.
- **BAKED TILES** — loaded tiles containing at least one bake block.

Select **PolyVeg Raw** or **PolyVeg Bake** to cycle through matching loaded
tiles, starting nearest the current view. Double-click a jump button to reset
its cycle from the current view.

Raw planting is ordinary static scenery and can be selected, deleted, or
undone before baking. Correct schema or planting settings, remove the unwanted
raw result, and replant before committing it.

## Bake and save

Baking combines configured raw vegetation on each world tile into generated
4-by-4 patch blocks. It replaces only static shapes currently listed in
`polyveg.json`.

- **Bake PolyVeg Tile** bakes the current camera tile.
- The right-click **Bake PolyVeg Tile** command bakes the pointer tile.
- **Bake PolyVeg Area** bakes pending loaded tiles inside the current camera
  Tile LOD. It does not load or scan the entire route.

Before confirming a bake:

1. Save or otherwise settle unrelated work.
2. Inspect the raw vegetation and schema carefully.
3. Confirm that the intended tile or LOD area is loaded.
4. Read the confirmation count.

Successful bake writes generated `.s` and `.sd` files in the route `shapes`
directory, places the bake objects, updates
`OpenRails/forest-bakes.json`, clears Undo, and purges only the successfully
baked tiles' raw source objects from memory. A failed partial bake is rolled
back and leaves its raw sources intact.

> **Bake cannot be undone.** To revise a baked tile, delete every
> **PolyVeg - Bake** object on that tile, save so unused generated assets can be
> cleaned safely, then plant and bake again. Never delete generated bake files
> manually while their objects are still referenced.

Save the route normally after a successful bake. Reopen the route and inspect
the baked boundary, especially feathered edges, clearances, water shorelines,
and tile borders.

## Hacks: delete every bake

The shared **HACKS** popup contains **Route Cleanup > Delete All PolyVeg
Bakes**. This loads every world tile and removes every PolyVeg bake object
across the route.

This is a route-wide destructive cleanup tool:

1. Make a full route backup.
2. Open **HACKS** from a supported Object Properties panel.
3. Choose **Delete All PolyVeg Bakes** and confirm.
4. The deletion remains one Undo operation until Save.
5. Save only after confirming the correct objects were removed. On Save, only
   generated bake assets that are no longer referenced are pruned.

Use ordinary object deletion for one tile. Use the Hacks command only when the
intent really is to remove all route bakes.

## Troubleshooting

| Symptom | Check |
|---|---|
| No schemas in F6 | Open Shift+F6, correct the status error, add at least one valid route asset, and save. |
| No polygon beneath pointer | Rerun SCO LIDEX Create Map Tiles and confirm the pointer is over the visible intended polygon. |
| Unrestricted is disabled | `osm_data/polyveg-exclusions.geojson` is missing; rerun SCO LIDEX. |
| Very few plants appear | Check Cap, Density or row spacing, Slope, Feather, asset Radius, schema Spacing, and the rejection report. |
| Trees appear in water | Confirm the water tile and terrain bed are processed and saved, then increase Water clear if a wider shoreline setback is wanted. |
| Trees are too high or buried | Correct the asset's positive Plant depth and verify the model origin and Scale. |
| Planting stops at a tile edge | Pointer and ruler operations are intentionally tile-clipped; continue from the neighboring tile. |
| Tile refuses new planting | It already contains a PolyVeg bake. Delete all bake objects on that tile, save, and replant. |
| Bake finds no raw objects | Confirm the tile is loaded and the raw objects' shape names are still listed in `polyveg.json`. |
| Bake Area misses tiles | It processes only loaded pending tiles inside the current camera Tile LOD. Move/load the area and retry. |

## Safe operating sequence

For production route work, use this order:

1. Back up the route.
2. Refresh SCO LIDEX map data.
3. Create or validate the schema.
4. Plant one small sample.
5. Inspect, Undo, and tune until correct.
6. Plant the required tile or ruler areas.
7. Save and reopen while the vegetation is still raw.
8. Bake one tile and inspect it.
9. Bake the remaining loaded area.
10. Save, reopen, and perform a final visual inspection.
