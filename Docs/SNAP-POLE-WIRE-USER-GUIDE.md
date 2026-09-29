# SNAP Pole and Wire User Guide — TSRE GenX v0.17

Build telephone-wire runs from placed support models with numbered SNAP attachment
nodes. TSRE creates blue raw wires for inspection, then bakes route-local scenery
shapes. Poles remain separate objects. Blender is needed only to author a support,
not to install or use the supplied SCOsnapPole example.

Back up your route first. Start with two or three poles on a disposable route copy.
Save and reopen a small successful run before extending it.

## Install the optional SCOsnapPole example

1. Download `SCOsnapPole-TSRE-example-v0.17.zip` and its `.sha256` companion.
   This is a separate model download; install the main TSRE application separately.
2. Verify the ZIP checksum and extract it to a temporary folder.
3. Merge its `SHAPES`, `TEXTURES` and `ADDONS` folders into your chosen route's
   folder. Keep existing route folders and review any same-name file replacements.
4. Reopen the route. In **F1 Objects**, find class **SCO Examples** and choose
   **SCO snap telephone pole - 16 wire attachments**.
5. Place a short line of poles near the intended track or road.

The example supplies `SCOsnapPole.s`, its matching descriptor, three DDS textures,
an object-list `.ref`, installation notes, this guide and internal checksums.
The packaged shape references those DDS files directly. Its geometry and 16 SNAP
transforms are unchanged from the supplied model. No Blender project, route world
file or pre-baked wire run is included. Simulator acceptance of this particular
packaged pole remains a separate runtime check.

## First wire run

1. Place two or three compatible poles with spacing below **Max Span**. Start
   with a simple line away from adjacent parallel pole lines or junctions.
2. Press **E** and select one placed pole. Choosing only an F1 reference is not
   enough: Commit needs an actual support with valid attachment nodes.
3. Open **F5 Auto Place**. Confirm the selected shape and use the **Wires** controls.
4. Start with **Sag 1.5%**, **Width 12 mm**, and **Max Span 75 m**. Adjust Max Span
   to your actual spacing rather than using a large value to bridge missing poles.
5. Click **Commit**. Inspect the blue raw wires from both ends and from the side.
   Check insulator contact, crossarm rows, pole orientation, clearance and sag.
6. If necessary, change the wire settings and Commit again before baking. If you
   move or rotate a support, recommit so the captured attachment positions agree.
7. **Save** bakes active raw wire spans and saves their world placements.
   Alternatively choose **Bake All Wires**, then **Save**. Baking alone does not
   save the route's world placements.
8. Reopen the route and inspect the run. Check simulator rendering separately.

Commit does not duplicate already recorded baked pairs. It is not a command to
reshape an existing bake: delete the relevant wire run, Save, then recommit and
bake if its support positions or wire dimensions need to change.

## What the controls mean

| Control | Behavior |
|---|---|
| Sag | Midpoint drop as a percentage of horizontal span length. At 1.5%, a 50 m span drops 0.75 m at its midpoint. |
| Width | Diameter of the triangular wire geometry in millimetres. |
| Max Span | Maximum eligible distance between neighboring supports. |
| Commit | Discovers the selected support's connected run and records active raw spans. |
| Preview Off | Hides/deactivates the last previewed section but retains its raw definitions; inactive spans are excluded from baking. It is not Delete. |
| Bake All Wires | Bakes all active raw wire sections, including other committed sections. Save afterward. |
| Delete Wires | Removes the tracked connected run attached to the selected pole, after confirmation. Keeps the poles; clears Undo history. Save afterward. |

Discovery uses nearby track/road corridor loading and bounded candidates. Eligible
connections may cross track-node boundaries, but disconnected runs, spans beyond
Max Span, branches and closed cycles are not general-purpose wiring solutions.
The nearest-span logic cannot infer your intended route among adjacent identical
parallel pole lines. Inspect the result and the deletion confirmation scope.

## Place poles with Auto Place

Manual placement and F5 placement can both supply wire supports. Select your pole
reference, choose the appropriate track/road **Target**, and set offsets,
rotation, **Spacing**, **Range** and **Wobble** in the Objects controls.

- A finite Range starts at the pointer for a normal forward placement. **Ctrl**
  also starts at the pointer; **Shift** works backward. Infinity retains the
  whole-section normal-placement behavior.
- Range limits distance; it does not force an extra pole at the range endpoint.
  A 225 m range with 50 m spacing places stations at 0, 50, 100, 150 and 200 m.
- F1 manual snapping and F5 target selection are separate. F5 uses its own
  track/road placement calculations.
- Wobble varies a deterministic subset of supports and keeps their bases fixed.
  At 100%, added lean and heading variation can reach 10 degrees. Repeating the
  setting does not compound the session adjustment. Zero restores the current
  session baseline; after reload, the saved orientation is the new baseline.
- Inspect placement and use Undo promptly where available. Recommit wires after
  support edits; Preview alone does not rotate poles.

## Compatible support names and SNAP nodes

Wire candidates match the selected support's filename as a case-insensitive
prefix, excluding `.s`. Selecting `SCOsnapPole` includes `SCOsnapPole_U` and other
suffix variants, but excludes `SCOsnapFence`. Selecting `SCOsnapPole_U` narrows the
family and excludes plain `SCOsnapPole`.

For model authors:

- Export attachment matrices named **SNAP_1**, **SNAP_2**, and so on in the first
  LOD hierarchy. Preserve those matrices in the exported `.s`, not only in Blender.
- Place each origin at its intended wire contact point. Use consistent numbering
  across compatible supports; inspect opposite headings for crossed rows.
- There is no fixed 16-wire limit. Sparse positive numbers are supported; only
  valid shared attachment numbers can connect. Duplicate numbers, invalid names
  and invalid hierarchy/transforms are rejected as attachment candidates.
- Older `SNAP_W1` or `MIRROR_W1` naming is not the current convention. Re-export
  those supports for new runs. Existing baked scenery does not require rebuilding
  solely because its old authoring convention changed.
- Confirm all intended nodes survived export. If Commit stays disabled, inspect
  the exported hierarchy before increasing Max Span or adding more poles.

## Save, discard and cleanup

Blue wires are pending generated work. Raw PolyVeg is also marked with a blue tint,
while ordinary manually placed vegetation remains ordinary scenery. Save bakes
active pending wires and owned raw PolyVeg on loaded tiles.

When pending generated work exists, closing offers **Bake & Save**, **Discard**,
or **Cancel**. Discard removes eligible unsaved generated work, including inactive
wire definitions; it preserves saved references and shared resources. If cleanup
fails, the editor stays open so you can resolve the reported problem and retry.
Individual files are published atomically; a whole route save is not one transaction.

To remove a run, select one of its poles with E, choose **Delete Wires**, inspect
the count, confirm, and Save. For deliberate route-wide cleanup, **HACKS > Route
Cleanup > Delete All Wire Bakes** is broader: use it only when all wire bakes
should be removed. Keep the route backup. Never remove generated S/SD/texture or
registry files manually while saved world objects may still reference them.

The registry accepts at most **8,192 span records**. An oversized Commit is
rejected with an explanation and preserves the accepted registry and preview.
Remove unused runs through the editor and finish cleanup before adding more.

## Troubleshooting

| Symptom | Check/action |
|---|---|
| Pole missing from F1 | Confirm the REF is in the chosen route's ADDONS folder, the shape is in SHAPES, and reopen the route. |
| Pole has missing textures | Copy all three supplied DDS textures to that route's TEXTURES folder. Keep the packaged shape's DDS references. |
| Commit disabled or no wires | E-select a placed support; verify current SNAP names, nearby track/road and eligible neighboring poles within Max Span. |
| Wrong poles connect | Use a narrower shape prefix or separate the competing lines; inspect a short run before committing a larger corridor. |
| Crossed or misplaced wires | Check attachment numbering and exported transforms, then pole headings. Recommit after support edits. |
| Existing bake does not change | Delete the tracked run, Save, then recommit using the intended settings. |
| Preview vanished | Preview Off retains inactive records. Select the support and Commit to make the intended run active again. |
| Bake succeeds but reopen loses placement | Bake alone is not Save. Save successfully before closing. |
| Cleanup reports unreadable world data | Resolve the reported world-file problem from a backup and retry. Do not bypass reference checks by deleting assets. |
| Save/Discard fails | Read the error, retain the open session, resolve access/data issues and retry. Some earlier world files may already have saved. |

The automated wire probe covers geometry, serialization, registry limits and
cleanup preservation. It does not replace visual checking of a model, its
attachments or the final simulator result.
