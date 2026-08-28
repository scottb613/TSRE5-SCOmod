# Water Ruler and Water Tools User Guide

This is the quick operating guide for the TSRE GenX Water Ruler and F7 Water
Tools. The ruler traces the bed of a river, stream, canal, or lake. Water Tools
then finds the connected low terrain around that trace, fits a water surface
across the affected tiles, and can optionally lower shallow terrain beneath the
processed water.

> **Work on a backed-up route.** Process Water Tiles changes terrain water
> masks and corner elevations. Adjust Terrain changes terrain height data. Each
> operation has one immediate Undo. Validate unfamiliar settings on a backed-up
> copy if needed, then trace and process the complete river in one production
> pass.

## Quick start

1. Back up the route and open it in the Route Editor.
2. Press **F7** to open **Water Tools**.
3. Select **New Water Ruler** and trace the complete river from one end of the
   intended waterway to the other. At least two points are required.
4. Use **Add Points** to continue the trace and **Edit Points** to move its
   orange control handles.
5. Leave **Above bed** at `0.75 m` for the first test and **Scan distance** at
   `1 tile`.
6. Select **Process Water Tiles** once. Water Tools processes the complete
   traced river in that one operation; wait for the Status cell to report
   completion.
7. Inspect the full result, especially shorelines, islands, tributaries, and
   tile borders. Use **Undo Last** immediately if it is wrong.
8. If shallow terrain shows through the water, select **Adjust Terrain**. This
   optional pass uses the same Above bed value as the required clearance below
   the processed water.
9. Inspect again from several camera heights.
10. Press **F7** to close Water Tools and remove its temporary ruler. Generated
    water and adjusted terrain remain.
11. Save the route, reopen it, and check the complete water surface and
    shoreline again.

## Open Water Tools

Press **F7** or choose **Tools > Water Tools**. The panel occupies the standard
right tools column and is mutually exclusive with the other main tool panels.
Opening another exclusive panel or pressing F7 again closes Water Tools.

Plain **M** toggles saved route map overlays without closing Water Tools or
changing the ruler. A map overlay can help with tracing, but it is only a visual
guide: processing follows the live terrain and water data.

Closing Water Tools removes the Water Ruler. It does **not** remove water made
by Process Water Tiles or restore terrain changed by Adjust Terrain.

## Draw the Water Ruler

### Start a ruler

1. Select **New Water Ruler**.
2. Click the terrain bed to place the first point.
3. Continue clicking along the centre of the river or lake bed.
4. Use enough points to follow changes in direction and bed elevation, but do
   not add unnecessary points to a straight, even section.

The ruler is blue, with orange control handles. Every point snaps to terrain.
The traced point elevations plus Above bed guide the fitted water surface.

Only one special ruler can exist at a time. Starting a Water Ruler replaces an
existing water, PolyVeg, or grade ruler. The ordinary measurement Ruler is
independent and is not removed.

While **New Water Ruler** remains active, selecting it a second time removes
the new ruler and returns the panel to idle. After a processing action, the
completed ruler remains visible for review even though placement mode ends.

### Add more points

Select **Add Points** to resume the existing Water Ruler. New points continue
from its last point. Use this after reviewing the route ahead or after leaving
placement mode.

If no Water Ruler exists, the Status cell reports **No ruler. Choose New Water
Ruler.**

### Edit points

1. Select **Edit Points**.
2. Click an orange control handle.
3. Drag it to the corrected bed position.
4. Release it over valid terrain so it snaps to the bed.

Editing a point changes both the horizontal trace and its terrain-derived
elevation. Review adjacent segments after moving a point. If terrain is not
available or is invalid at release, the point returns to its original position.

Moving a ruler point after a water operation intentionally clears the panel's
**Undo Last** latch. This prevents the water-operation button from undoing the
newer ruler edit by mistake. Finish the ruler geometry before processing when
possible.

### Trace effectively

- Put points in the intended connected low ground, not on a bank, bridge deck,
  island, or nearby drainage ditch.
- Trace bends and meaningful changes in bed elevation. Long straight reaches
  need fewer points.
- Trace the full connected river before processing. Long, multi-tile rulers are
  supported so the complete river can be handled by one Process Water Tiles
  action and, if needed, one Adjust Terrain action.
- For a lake, trace through representative low bed areas. The ruler is a guide
  into connected low terrain; it is not a shoreline polygon.
- Avoid running the ruler close to an unrelated connected water feature unless
  that feature should be included.
- If the settings are unfamiliar, test a short ruler on a backed-up route copy
  or use Undo Last, then redraw the full river for the production pass.

## Process Water controls

### Above bed

**Above bed** is measured in metres and defaults to `0.75 m`. It has two linked
uses:

- **Process Water Tiles** fits the water surface Above bed metres over the
  terrain-snapped ruler trace.
- **Adjust Terrain** treats the same value as the desired terrain clearance
  below the processed water.

A larger value creates deeper water relative to the traced bed and permits a
larger optional terrain correction. A smaller value keeps the water closer to
the bed. Use one value for both actions on a given result so water placement
and bed clearance stay consistent.

Above bed does not set one flat elevation for the complete waterway. The
surface follows the elevation trend of the ruler and is fitted across tile
corners so neighboring seams agree.

### Scan distance

**Scan distance** is the terrain-tile radius searched around every tile crossed
by the ruler. It defaults to `1 tile`.

- `0` restricts the search to tiles directly crossed by the ruler.
- `1` includes the immediate neighboring tiles and is the normal starting
  value.
- Larger values allow a wider connected shoreline search but load and examine
  more terrain, take longer, use more memory, and can reach unintended nearby
  low ground.

Increase Scan distance only when a legitimate connected water area is being
cut off by the current search boundary. Making it large does not improve the
water elevation calculation.

## Process Water Tiles

Select **Process Water Tiles** after the ruler has at least two points. The
operation:

- loads the bounded terrain corridor around the complete ruler;
- finds low terrain connected to the traced seed path;
- stops at raised shoreline or the configured search boundary;
- replaces the connected previous Water Tools result inside that corridor;
- writes water flags and fitted corner elevations; and
- reconciles shared wet tile corners so neighboring surfaces meet exactly.

The Status area reports stages such as loading tiles and finding shoreline,
then reports the number of water patches processed and old patches cleared.
The command completes before another Save command can run.

Processing is bounded for safety. Invalid ruler points, unavailable terrain,
unsupported terrain grids, a corridor exceeding 4,096 tiles, or a runaway
shoreline stop the operation before mutation and leave a concise Status
message. The guard is deliberately large: the normal workflow is one ruler and
one operation for the complete river, including a long multi-tile river.

After completion, placement mode ends but the ruler remains visible. Inspect
the complete connected result before changing the ruler or closing the panel.

## Adjust Terrain

**Adjust Terrain** is optional and must follow Process Water Tiles. It reads the
processed water mask and lowers eligible shallow terrain posts to provide the
selected Above bed clearance beneath the fitted water surface.

The pass changes detailed terrain only. It does not create water, raise
terrain, or indiscriminately flatten banks and islands. Definite exposed land
is protected, while water-mask edges receive a tapered clearance to soften the
transition into the corrected bed.

Use Adjust Terrain when shallow shelves or terrain posts visibly break through
processed water:

1. Keep the same Above bed and Scan distance used for Process Water Tiles.
2. Select **Adjust Terrain**.
3. Wait for Status to report the number of terrain posts lowered.
4. Inspect banks, islands, bridge approaches, crossings, and tile seams.
5. Select **Undo Last** immediately if too much terrain changed.

If Status says **Terrain already meets the selected clearance**, no terrain is
changed. If it says **No processed water found. Process Water Tiles first**,
the current ruler corridor does not contain a usable processed result.

## Undo, review, and save

**Undo Last** restores the water or terrain state from immediately before the
latest successful Water Tools operation. It is a single-use, operation-specific
Undo:

- Process Water Tiles owns one water-state Undo.
- Adjust Terrain owns a separate terrain-state Undo and becomes the latest
  operation after it succeeds.
- A later ruler edit invalidates the Water Tools Undo button.
- **Nothing to undo** means no current Water Tools operation is available to
  restore.

Review before saving:

1. Follow the shoreline along the full ruler.
2. Look for dry gaps, flooded high ground, raised shelves, or lost islands.
3. Inspect every crossed tile border for a visible water seam.
4. Check crossings and scenery from normal route-driving height.
5. Save the route only after the complete result is acceptable.
6. Reopen the route and repeat the visual check.

If the Water Ruler itself should not remain in the saved route, close Water
Tools after processing and then save. Closing the panel deletes only the ruler;
the processed water and adjusted terrain remain.

## Troubleshooting

| Symptom | Check |
|---|---|
| No ruler to process | Select New Water Ruler and place at least two points. |
| Processing finds no connected water patches | Move the ruler points into the actual low bed, check Above bed, and confirm detailed terrain is available. |
| Water stops too close to the ruler | Increase Scan distance by one tile and retry; do not jump immediately to a very large radius. |
| Water reaches an unintended area | Undo Last, reduce Scan distance, and move or add ruler points so the trace stays inside the intended connected bed. |
| Water elevation looks wrong | Confirm no ruler point sits on a bank, island, bridge, or other raised surface; edit the point and reprocess. |
| Shallow terrain shows through | Run Adjust Terrain with the same Above bed value used to process the water. |
| Adjust Terrain changes nothing | The bed may already meet clearance, or no processed water exists in the current ruler corridor; read the Status message. |
| Undo Last says Nothing to undo | The operation was already undone, the ruler was edited or removed afterward, or no Water Tools operation completed. |
| Ruler disappears | F7 was toggled off, another exclusive tool panel opened, or New Water Ruler was toggled off during placement. The water result remains. |
| Processing stops at the 4,096-tile safety limit | First reduce an unnecessarily large Scan distance. Divide the job only if the complete river genuinely exceeds the hard corridor guard. |
| A seam or dry gap remains after save/reload | Undo and reprocess if still available; otherwise restore the route backup, then use a corrected ruler and consistent settings. |

## Safe operating sequence

For production route work, use this order:

1. Back up the route.
2. If needed, use a short ruler on the backed-up copy to validate Above bed and
   Scan distance, then Undo the test result.
3. Select New Water Ruler and trace the complete connected river.
4. Review and edit every ruler point before processing.
5. Select Process Water Tiles once for the complete river.
6. Inspect the full shoreline, islands, crossings, and tile borders.
7. Undo and correct the ruler or values if the water result is wrong, then run
   one replacement pass for the complete river.
8. Select Adjust Terrain once for the complete river only if shallow terrain
   needs correction.
9. Inspect banks, islands, crossings, and scenery again.
10. Close Water Tools to remove the temporary ruler.
11. Save, reopen, and perform a final route-wide visual inspection.
