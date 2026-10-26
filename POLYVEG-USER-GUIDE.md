# PolyVeg Planter User Guide

This is the quick operating guide for the TSRE GenX PolyVeg system. PolyVeg
places ordinary static vegetation objects from a route-owned schema, either
inside SCO LIDEX map polygons or inside a ruler drawn in the Route Editor. Raw
objects can then be baked into fewer scenery blocks for better editor and route
performance.

> **Work on a backed-up route.** Planting can be undone before later edits, but
> baking deliberately clears the Undo history. Save and inspect a small test
> area before committing a large planting operation.

## View visibility

Use **View > PolyVeg**, beside **Forest Region**, to show or hide generated raw
PolyVeg, all vegetation shapes named by the route's PolyVeg schema, and baked
blocks (including saved/reopened placements). Checked means visible. Unchecking also hides bake
markers and prevents viewport picking of hidden vegetation. This session-only
view setting leaves planting, baking and saved route contents unchanged.

## Quick start

1. For mapped planting, run SCO LIDEX **Create Map Tiles** for the route.
   PolyVeg polygon planting requires `osm_data/polyveg-polygons.geojson`;
   OSM-assisted local planting also uses `osm_data/polyveg-exclusions.geojson`.
   Turn **Use OSM** off for local ruler/area/full-tile planting without those
   derivatives. Terrain, slope, water and database clearances still apply.
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
8. Save when planting looks right: Save automatically bakes owned raw PolyVeg on loaded tiles, together with active raw wires, before writing world placements.
9. For an explicit bake first, use **Bake PolyVeg Tile** or **Bake PolyVeg Area**, then Save and reopen to inspect the result.

## Replace legacy Forest regions across the route

Select the desired schema and planting settings in F6. Select an existing
Forest region, then choose **Hacks...** on its **Forest Region** card and
**Replace All Forest Regions** in HACKS.

The tool surveys the entire current route, merges overlapping Forest footprints,
and asks you to confirm the schema, region count, merged area, affected tiles,
and planting settings. The F6 **Cap applies per tile** for this action.
Original forest populations and textures do not determine the replacement mix.
Mapped forest polygons are not required.

TrackDB, RoadDB, water, slope, spacing and feather settings apply. Submerged
ground is always excluded, including at zero water setback. Existing PolyVeg
or conflicting bake assets on affected tiles block the operation and are
reported before replacement.

Progress and Cancel remain available while rendering and editing are paused.
Each tile is generated and baked before proceeding to the next; raw vegetation
for the entire route is never accumulated. Empty tiles are counted and skipped;
exclusions or a tiny fragment below the density threshold do not stop the batch.
If the entire route produces zero plants, its original forests remain unchanged.
Cancel/errors remove this batch's additions and restore its
generated files; original forests remain until all tiles succeed. Baking clears
Undo history. Save normally after success and inspect the route after reopening.
If file rollback reports an I/O error, keep the route open and resolve it before
saving. Work on a backed-up route.

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

Open the full-screen editor with **Shift+F6** or **Edit** in the F6
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

## F5 Auto Place: PolyVeg corridors

Select **PolyVeg Corridor** in F5's bordered two-row, three-column selector.
Objects, PolyVeg Corridor and Wires have labels above mutually exclusive
checkboxes; unselected labels dim. F5 and F6 share Schema, Density, Cap, Seed,
Use OSM and report settings. **Edit** opens the schema editor. Rows remains
available in F6 only; Auto Place corridors always use density planting.

1. Choose Follow: Tracks, Roads, or the nearer of Tracks & Roads.
2. Set Range (m), with the same click rules as AP Objects. A positive range
   starts at the pointer; zero uses the whole node-to-node section. Ctrl-click
   starts at the pointer even at zero; Shift-click runs backward. Shift takes
   precedence over Ctrl. Runs stop at section boundaries. Split runs longer
   than 100 km into smaller operations.
3. Choose Both Sides, Left or Right, relative to the planting direction. Shift-click
   reverses that direction and therefore reverses the meaning of Left/Right.
4. Set Setback (m) from the selected centreline, then Width Per Side (m) outward
   from that setback. Both + setback 10 + width 40 plants bands 10–50 m from the
   centreline on each side. The corridor's Setback controls clearance from the
   followed track/road node; the schema clearance is not applied a second time
   to that node. Schema clearances still apply to other nearby tracks and roads.
5. Select **Plant** to latch it, then click track/road to plant directly. There
   is no separate preview step. Plant stays latched for more clicks, like AP
   Objects; click Plant again, press Escape, switch tools/modes or close F5 to
   stop. Changing corridor geometry also cancels the armed tool.
6. Plants always use ground height at their final position with schema planting
   depth. Terrain, slope, water, clearance and spacing checks remain active.
   The corridor can cross tiles; Cap limits the complete run. Use OSM requires
   mapped forest coverage. Turn it off to plant the corridor alone while
   retaining live terrain and schema clearance checks.
7. Inspect. Undo Last uses the editor Undo, as Ctrl+Z. Use it immediately after
   planting to remove that run; later edits change what Undo Last affects.

Corridor plants are owned raw PolyVeg and immediately join the pending raw
counts and existing bake pipeline. Bake Tile, Bake Area and automatic baking on
Save include them. F5 Bake area has the same loaded camera Tile LOD scope as F6;
Save includes pending loaded corridor tiles beyond that scope. Existing baked
tiles reject new planting. Existing raw plants enforce footprint spacing.
Baking clears Undo. No new route file or bake format is introduced.

The corridor status shows Left and Right plant counts and exclusions. Both Sides
requests both sides; terrain, slope, track/road clearance, water and OSM coverage
still apply. One side can have no accepted plants; the status calls this out.

This corridor workflow supports local editable routes. F6 Rows settings do not
change Auto Place corridor generation.

## F6 PolyVeg Planter controls

Press **F6** to toggle the Planter. Select **Map Polygons**, **Ruler / Area**,
or **Full Tile** to show only that method's controls. Switching methods cancels
armed tools and removes the temporary ruler. Density and Cap use aligned
label/slider/value rows; Seed has its own narrower column with its label above.
Use OSM/No Report and Closed Area/Unrestricted use matching
bordered cells. Hover over labels or fields for the current allowed ranges.
**No Report** sits beside Use OSM and defaults on for the revised panel, then
remembers your choice. **Rows** expands to enable row planting and collapses to
use Density; its spacing values are retained. **Bake Tile** and **Clear Bake** are always visible side by side, with
**Bake Area** full-width below; there is no Bake dropdown. The compact status row shows loaded Raw, Blocks
and baked Tiles. F5/F6 shortcuts are unchanged. **Edit** is a full-width button below the schema
selector. Numeric counters are centered and use dim orange inside their frame.
Rows starts collapsed/off for a new route/editor session; row spacing values
are retained. An explicit Rows choice survives F5/F6 switching in that session.
The obsolete Jump controls are removed. Collapsing a control section preserves
its values. Zero-result reports identify the tested tile, OSM/coverage state,
requested targets and exclusion counts; they scroll and offer **Copy Report**.
No Report suppresses those details while keeping validation errors visible.

- **Schema** selects what vegetation mix to plant.
- **Density (/km2)** requests random plants per square kilometre within the
  selected schema's limits.
- **Cap** limits this complete planting operation, not each polygon fragment.
- **Seed** selects a repeatable layout. The same schema, boundary, settings,
  and seed reproduce the same candidate pattern; change the seed to rearrange
  it.
- **Use OSM** is selected by default and saved with the panel settings. On applies
  the usual OSM coverage and exclusions. Off needs no OSM cache: ruler/area
  planting uses its drawn boundary and **Plant PolyVeg** uses the pointer tile.
  Terrain, slope, TDB/RDB/water clearances and baked-tile protection still apply.
  Plant Tool uses a bordered two-row, three-column grid: Map Polygons,
  Ruler / Area and Full Tile labels above centered, mutually exclusive checkboxes.
  Unselected labels are dimmed. Map Polygons stays visible but disabled when
  Use OSM is off. If it was selected, the panel switches to Ruler / Area.
  Enabling OSM makes it available without changing the current selection.
- **Flood Fill** is disabled while Use OSM is off. When enabled, it plants every matching polygon on the pointer tile with the
  same LIDEX category and fill style. Off plants only the top visible polygon
  beneath the pointer. Both modes are clipped to the pointer tile.
- **No Report** suppresses planting detail reports and the Bake Tile completion report. It
  does not suppress errors, the success sound, or the operation itself.

### Rows

Expand **Rows** to enable aligned layouts. Collapse **Rows** to return to
density-based random planting; there is no separate Enable rows checkbox.

- **Row Separation (m)** is the distance between parallel rows.
- **Plant Spacing (m)** is the distance between plants along a row.
- **Direction** rotates the global row grid clockwise from route-plan north.
- **Auto** (`0`) uses the schema's minimum separation for Row separation or
  Plant spacing.

Rows remain aligned across clipped pieces and neighboring operations because
they use one route-global grid. In row mode, Row separation and Plant spacing determine
the requested count instead of Density; Cap still applies.

## Plant from SCO LIDEX polygons

1. Open F6, choose Map polygons, and select the schema and settings.
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
polygon is unsuitable. Select **Ruler / Area** in F6 first.

### Corridor

1. Leave **Closed Area** off and set **Width (m)** to the full corridor width.
2. Select **New Ruler**, then click terrain to add at least two
   centreline points.
3. Use **Add Points** to continue the ruler or **Edit Points** to drag its
   orange handles. Moved points snap back to terrain when released.
4. Select **Plant Ruler** to plant. After successful planting, the ruler and
   its point-tool latches are cleared; Undo restores the ruler with the planting batch.

### Area

1. Enable **Closed Area** and set **Width (m)**. The clicked polygon interior is
   filled; Width adds an exterior buffer around it. Use `0` for no exterior
   buffer.
2. Select **New Ruler** and place at least three boundary points.
3. Keep the light-green closed outline simple—edges must not cross or overlap.
4. Edit as needed, then select **Plant Ruler** to plant immediately.

Successful Plant Ruler/Area removes the temporary ruler. Empty or failed
planting keeps it available for adjustment. The PolyVeg ruler is tile-locked to its first point. Long boundaries must be
handled as separate tile operations. Starting a new special water, vegetation,
or grade ruler replaces the existing special ruler.

### Plant Full Tile

Choose the schema, density, cap and seed, choose **Full Tile**, then click **Plant Full Tile**. Its label and border turn orange while armed.
Click terrain to plant its entire 2048 m tile once. The button turns off as soon
as the click is consumed. Click the button again or press Escape to cancel;
hiding the panel or choosing another tool also cancels.

This operation needs no ruler and works independently of **Closed Area**, ruler Width
and **Unrestricted**. With **Use OSM** on, it requires the SCO LIDEX `polyveg-exclusions.geojson`
cache, retains track, road, water, slope, land and baked-tile exclusions, and
uses the current row settings. Candidates stay within the clicked tile.
**No Report** suppresses the single post-plant report; there is no pre-plant confirmation.
Use Undo to reverse the planting operation.

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

## Inspect and correct raw planting

If planting produces no objects, the result report identifies the tested tile,
OSM mode and local cache polygon count, and whether an existing bake was
actually detected. It separates missing terrain/slope samples, slope, track,
road, water, boundary, feathering, spacing, OSM and baked-tile rejection counts.
It also identifies a zero density/area target and placement failures. Ruler/area
planting uses the first control point's tile; portions on other tiles are not
processed. A zero count for a later check can mean that it was never reached.
Mapped OSM exclusions may already be carved out of the planting boundary,
reducing its area rather than generating rejected candidates.

Use **Copy Report** to copy the complete diagnostic text. The report scrolls
when needed. **No Report** also suppresses zero-result reports; invalid
definitions and other input errors still appear.

The Status card reports objects on currently loaded world tiles:

- **RAW OBJECTS** — individual vegetation objects whose shapes are listed in
  the current route catalog.
- **BAKED BLOCKS** — generated PolyVeg block objects.
- **BAKED TILES** — loaded tiles containing at least one bake block.

Raw planting is ordinary static scenery and can be selected, deleted, or
undone before baking. Correct schema or planting settings, remove the unwanted
raw result, and replant before committing it.

## Save and exit with pending work

Owned raw planting has a 25% blue tint while keeping texture detail and alpha cutouts. Save bakes eligible pending vegetation on loaded tiles and active raw wires. Exit offers **Bake & Save / Discard / Cancel** when generated work remains. Discard preserves saved references/shared assets and keeps the editor open if cleanup fails. Malformed world files stop cleanup before asset deletion; resolve the reported problem and retry. Individual file writes are atomic; the whole route is not one transaction.

## Clear Bake on one tile

Press **Clear Bake** beside Bake Tile. The button latches on. Click terrain
on the intended tile once: all PolyVeg bake blocks on that tile are removed and
the latch resets. A completed tile click plays the standard `SCOsuccess.wav` when
UI Sounds is enabled, including an already-empty tile. Failed tile loads stay silent.
The loaded-world counters refresh immediately and once more after the click
settles. Raw plants and neighboring tiles remain. Escape, another tool,
or a method/panel change also cancels the latch.

This is one Undo operation. **Save** commits the deletion and removes unused
blocks and source records from `OpenRails/forest-bakes.json`, along with the
now-unreferenced generated shape/descriptor assets. Clearing the final bake also
removes an empty residual manifest. Metadata and assets stay intact until Save
so Undo and discarding the unsaved route remain safe. Inspect the tile before
saving; use Undo immediately if the wrong tile was cleared.

## Bake and save

Baking combines owned raw generated vegetation into 4-by-4 patch blocks on each
world tile. Manually placed copies of the same assets remain ordinary scenery.
The right-click Bake PolyVeg Tile command retains its pointer-tile behavior.

**Bake Tile** is a one-shot latched tool. Press it, then click terrain on the
tile containing the raw PolyVeg to bake. The latch resets on that click, before
processing begins. Clicking the button again, Escape, another tool, or a method/
panel change cancels arming. The clicked tile is captured, so moving the pointer
or having the camera on a different tile does not redirect the bake.

**Bake Area**, full-width below Bake Tile/Clear Bake, runs immediately over
loaded pending tiles inside the current camera Tile LOD. It does not load the
rest of the route. Choosing it cancels any armed tile action. Save also bakes
pending raw PolyVeg on loaded tiles.

Bake Tile starts without a pre-bake confirmation/report. With **No Report** on,
there is no completion report; with it off, one report appears after completion.
Transient processing progress and genuine errors remain visible. Success sound
and counters still update in either case.

Inspect a small planting first, bake the intended tile/area, then save normally
when satisfied.

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

Use Clear Bake on the intended tile, or delete all its bake objects. Use Hacks only when the
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
7. Inspect all raw planting before saving; use Undo now if it needs correction.
8. Save to bake eligible pending loaded vegetation and write world placements.
   For an explicit bake first, use Bake Tile/Bake Area before Save.
9. Reopen and inspect boundaries, clearances and the baked result.
10. Validate the result in Open Rails.

## Duplicate planting protection

All PolyVeg planting tools reject a request that overlaps an already planted
area: Auto Place corridors, Map Polygons/Flood Fill, Ruler/Closed Area, Full Tile,
and forest replacement. Rejection adds nothing, plays the standard error buzz
(when editor sounds are enabled), and shows **Already Planted** even with
**No Report** selected. Changing the seed, schema or planting tool does not
bypass the check. Partial overlap rejects the entire request; adjoining areas
with only a shared edge are allowed.

While plants are raw, their operation footprint follows the live plants and
Undo. Removing all raw plants in the relevant tile or undoing that planting
releases its footprint. After Bake/Save, the existing baked-tile guard applies,
including after reload. Clear Bake before replanting a baked tile. No additional
route file or MSTS world token is used. An operation that produces no plants
reserves no area. Existing exclusions and holes remain outside the footprint.
