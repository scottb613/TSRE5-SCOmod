# HACKS User Guide

Applies to TSRE GenX v0.20. HACKS contains advanced recovery, replacement and
route-wide operations for route builders and modders. Some change the current
selection; others alter the entire route or write directly to disk. Select an
action for a specific task rather than treating this window as a cleanup list.

Keep a complete route backup before using destructive actions. Inspect the
result before saving, and reopen the route in GenX and Open Rails afterward.
An available Undo operation is not a replacement for a backup: several actions
clear Undo, remove generated files or update terrain files immediately.
Some buttons execute immediately rather than opening a preview or confirmation.

## Open HACKS and choose the right selection

Select an eligible object or terrain tile and click **Hacks...** in its
properties. The shared popup changes its choices when the selection changes.
Closing it or toggling the button off closes the helper. Its position can be
pinned between sessions.

| Selection | Actions available in the shared popup |
| --- | --- |
| Supported standard track object | Repair Junction, Repair Elevation, Remove Vector, Remove Branch, Remove All Interactives, Delete All Wire Bakes |
| Signal | Fix Signal Flags, Remove All Interactives |
| Terrain | Reset All TERRTEX, Compress TERRTEX — DXT1, Water Tiles Off |
| Static scenery | Delete Instances |
| Forest region or recognized PolyVeg object/bake | Delete All PolyVeg Bakes, Replace All Forest Regions; qualifying static objects also offer Delete Instances |
| Recognized generated AP/legacy test wire object | Delete All Wire Bakes; qualifying static objects also offer Delete Instances |

The selection controls availability, not the extent of a route action. For
example, selecting one forest exposes a command that replaces **all** Forest
regions. Standard track is the shared popup's supported track context; dynamic
track does not currently expose the same action set.

Speedposts have a separate legacy **Advanced > Hacks** dialog. Level crossings
also have a **Silent crossing MSTS HAX** checkbox. Both are covered below.

## Selected track actions

### Repair Junction

Rebuilds the selected track object's stored **JNodePosn** junction information
from matching junction records in TrackDB. Use it when that object's junction
metadata needs to be brought back into agreement with the database.

It does not move track or reconstruct missing track geometry. The change is
recorded as an object Undo operation. Inspect the junction, save if correct,
then check the route again.

### Repair Elevation

Recalculates the matching TrackDB vector's stored section **sElev** values using
the legacy curve-direction rule. The vector endpoints are set to zero; interior
values use zero or plus/minus 0.05 according to consecutive curve directions.

This is a specialized stored-section repair. For physical track height, grade or
terrain adjustments, use the corresponding placement, grade or terrain tools.
It is not a general repair for every height mismatch reported by F11. TrackDB
Undo data is recorded; inspect the affected vector and save only if appropriate.

### Remove Vector

Removes the database vector containing the selected track object's reference.
A vector can contain several connected sections, so its extent can exceed the
single visible shape you selected. The command checks the matching TrackDB and
RoadDB records.

Use it only for deliberate database reconstruction or removal. It removes
logical database data; the visible world shape can remain. Reconcile the world
objects and database afterward. Remove attached interactive world objects in
the affected area through their normal object controls first, so they are not
left with broken references. Database item deletion alone does not ensure every
corresponding world object has been removed.

Database Undo states are recorded, but inspect linked objects and rerun the
diagnostics before committing a repair. **Remove All Interactives** is a
separate whole-route operation, not the routine prerequisite for a local fix.

### Remove Branch

Removes the connected database tree reached from the selected track reference,
checking both TrackDB and RoadDB. Connections can carry the operation through
junctions; "branch" does not mean only a short siding or the selected shape.

Each tree has a **1,000-node guard**; a tree exceeding that limit is not removed
by that database operation. It is not an unlimited route-clear command. The
same world-object and interactive-reference precautions as Remove Vector apply.
Database Undo states are recorded. Work on a copy and inspect the actual extent
before saving.

## Selected signal action

### Fix Signal Flags

Checks the selected signal's TrackDB **TrSignalType** flags against its enabled
signal heads and configured subobjects. This includes junction-link and
number/gradient/user plate flags.

The **Signal Flags** dialog previews old and new values; mismatches are marked
`[E]`. Apply only after reviewing and confirming the proposed repair. Cancelling
the dialog leaves it unapplied. Save and test the signal afterward. This repairs
flags, not a missing model, a signal script or the entire route's signalling.
Do not assume every signal repair has the same Undo behavior as track repairs.

## Route object removal

### Remove All Interactives

Loads route world tiles and removes TrackDB/RoadDB-linked interactive world
objects across the route, including signals, speedposts/mileposts, platform and
siding markers, pickups, level crossings, car spawners, hazards and linked sound
regions. Track and road geometry are retained.

This is useful for a deliberate strip-and-rebuild of route interactives. It is
not ordinary distribution cleanup: these objects may be essential to operation.
The command asks for confirmation and records grouped Undo data. If accidental,
Undo before saving or closing. After an intentional removal, follow the displayed
recommendation to save, close TSRE and restart before further route work.

### Delete Instances

Loads route world tiles and deletes every matching instance of the selected
static object. Both the **object type and filename** must match; filenames are
compared without case sensitivity. Supported static types include ordinary
Static, Gantry and CollideObject scenery.

Use it to remove a repeated scenery model throughout a route. It is not limited
to nearby tiles or objects with the same REF description. The confirmation names
the selected file. Placements are deleted in a grouped Undo operation; the model
and texture source files are not the target of this command. Inspect the count
and Undo before saving if the match was broader than intended.

## Route terrain and texture actions

### Reset All TERRTEX

Starts a full terrain-paint reset after requiring saved route changes and
confirmation. It writes detailed terrain patches back to `terrain.ace` and
distant-mountain patches to `terrainDM.dds`, then `terrainDM.ace`, then
`terrain.ace`, according to which default texture is available.

The operation resets patch texture mappings, collapses obsolete material-table
entries, removes matching generated per-tile TERRTEX files across seasonal
folders, and **permanently deletes saved files in `terrain_maps`**. It also clears
the saved map-overlay display state and reloads loaded terrain.

It processes both `TILES` and `LO_TILES`. It writes files directly, reports
invalid tiles/save/delete failures, and **cannot be undone in TSRE**. Check all
reported counts; a partial failure is not a completed reset.

Use this only when intentionally discarding route terrain painting. To remove
saved maps while retaining painted terrain, use **Route > Pack for Distro...**.
For a local painting experiment, use the narrower tile-paint controls.

### Compress TERRTEX — DXT1

Final production conversion for the entire route's TERRTEX, including seasonal
ACE and DDS files. Save all route changes first, select terrain, open HACKS,
read the confirmation and allow the complete operation to finish.

- Converts supported textures to **opaque DXT1 DDS** and discards alpha.
- Preserves base dimensions and whether the source has mipmaps.
- Updates detailed/distant terrain references and deletes converted ACE originals.
- **Introduces permanent image-quality loss. Run it only once, after editing.**
- **Retains no TERRTEX originals or backups and clears Undo history.**

Run it on the final production copy while retaining an editable original
elsewhere. Review the completion/error message and inspect distant terrain and
seasons in Open Rails. An interrupted conversion keeps completed work and can
be resumed with the same action; it is not rolled back to the original images.
A completed-conversion record blocks repeated conversion for that route in the
local setup. Do not treat a fresh local setup as evidence that copied textures
have never been compressed.

### Water Tiles Off

Requires saved route changes, then turns off water-patch flags in the route's
detailed terrain `TILES/*.t` files. Terrain heights, textures, gaps and non-water
patch flags are retained. In v0.20 this action scans `TILES`, not `LO_TILES`.

It affects all eligible detailed tiles, not just the selection. It writes directly
to disk and **cannot be undone in TSRE**. Review scanned/changed/invalid/save-failure
counts. Separate scenery objects that depict water are not removed by this
terrain-flag operation.

## Route vegetation and wire actions

### Delete All PolyVeg Bakes

Loads route world tiles and deletes recognized **PolyVeg - Bake** objects across
the route. It is not restricted to the selected bake or visible tiles, and does
not target raw PolyVeg or ordinary Forest regions.

Placement deletion remains one Undo operation until Save commits it. On Save,
only generated bake assets that are now unreferenced are eligible for cleanup.
If no bake objects remain, the action may immediately prune unreferenced orphan
generated files/manifest entries; such file cleanup is not placement Undo.

Inspect the result before saving. Use the normal scoped bake/delete controls
when only one area should change.

### Replace All Forest Regions

Replaces legacy Forest regions throughout the route with baked PolyVeg using
the **currently selected F6 schema and planting settings**. Select the schema
and check Density, Cap and Rows before opening this route action. The Cap is
applied **per tile**. Original Forest textures/populations do not choose the
replacement vegetation mix. The selected schema must contain bakeable assets;
the tool validates them before replacement.

The tool surveys regions, merges overlapping footprints, checks affected world
and terrain coverage, and previews the replacement scope for confirmation.
Mapped OSM forest polygons are not required. Track/road, water, slope, spacing
and feather exclusions apply; existing PolyVeg or conflicting bake assets on
affected tiles can block replacement.

Generation and baking proceed one tile at a time with progress and Cancel.
Original forests remain until the batch succeeds; cancellation/errors attempt
to remove this batch's additions and restore generated files. If all tiles
produce zero plants, the original forests remain. Inspect omitted/unplantable
coverage and the completion message. Baking clears Undo, so save normally after
success and inspect after reopening. Resolve any rollback I/O error before saving.

### Delete All Wire Bakes

Deletes tracked generated AP and recognized legacy test wire bakes throughout
the route, turns off previews and resets wire reservations. It preserves poles
and PolyVeg; it is not a search-and-delete of every arbitrary third-party wire
model with a similar appearance.

This clears Undo history. Generated-asset cleanup is part of the workflow; save
the route afterward. Use the normal wire controls for deleting one connected
run. Choosing this action from one selected wire or track object still has
route-wide scope.

## Separate legacy controls

### Speedpost Hacks and Remove broken Track Items and Expand

Select a speedpost and open **Advanced > Hacks**. Its separate legacy popup has
**Remove broken Track Items and Expand**.

It checks the selected speedpost's additional TrackDB item references for missing
items or mismatched speedpost types. If the primary reference is unusable it
reports **Speedpost broken, can't fix**; if no repair is required it reports
**No need for fix**. When repairable errors are found, it removes/rebuilds the
additional item links using the speedpost expansion routine.

Expansion can create links on nearby intersecting track sections, so inspect
every resulting link, direction and speed setting. This is a legacy recovery
tool, not a route-wide speed-limit editor. Use a backup rather than relying on
Undo for this popup, then save and test the affected area.

### Silent crossing MSTS HAX

This checkbox is in level-crossing properties, not the shared HACKS popup. It
toggles the legacy silent-crossing flag combination in the selected crossing's
`LevelCrData`. Unchecking clears that combination.

Use it only for the intended MSTS compatibility workaround and test the crossing
in the target simulator. It does not replace the crossing sound-file, timing or
sensitivity controls, and its label is not a guarantee of identical simulator
behavior for every crossing configuration. Save the edited crossing normally.

## Check the route after using HACKS

1. Inspect the changed area and review completion, omission or failure counts.
2. Undo an unintended supported placement/database change before saving. Restore
   a backup for direct disk edits or actions that cleared Undo.
3. Save intended object/database changes. For a deliberate Remove All Interactives
   run, follow its save-and-restart recommendation.
4. Press **F11**, choose **Load/Scan World Tiles**, and review fresh diagnostics.
5. Run **Route > Create Route Health Report**, then test the route in Open Rails.

For route distribution, use **Pack for Distro** after verification. Resetting
terrain, removing interactives, or deleting vegetation/wires is not a required
publication step. Choose those operations only when their changes are intended.

## Related guides and source

- [Pack for Distro User Guide](https://github.com/scottb613/TSRE5-SCOmod/discussions/13)
- [PolyVeg Planter User Guide](https://github.com/scottb613/TSRE5-SCOmod/discussions/2)
- [TERRTEX Textures User Guide](https://github.com/scottb613/TSRE5-SCOmod/discussions/10)
- [SNAP Pole and Wire User Guide](https://github.com/scottb613/TSRE5-SCOmod/discussions/11)
- [Published v0.20 source](https://github.com/scottb613/TSRE5-SCOmod/tree/v0.20)
