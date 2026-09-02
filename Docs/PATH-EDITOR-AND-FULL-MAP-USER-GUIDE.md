# Path Editor and Full Map User Guide

TSRE GenX includes a full-screen, high-resolution TrackDB map and a standalone
Path Editor for creating and maintaining MSTS/Open Rails `.pat` path files.
This guide covers those completed map and path workflows.

> **Scope:** The Activity Builder screen also contains Activity Editor controls.
> Activity authoring is not covered or certified by this guide. A saved path is
> an independent route resource; it does not become part of an activity until
> an activity assigns it.

Always back up the route before creating, repairing, cloning, or deleting path
files. Path files are stored in the route's `PATHS` folder.

## What the Full Map Provides

The full map is a scalable two-dimensional view built directly from the loaded
route's TrackDB. It is intended for route overview, TrackDB inspection, and
path work over distances that are difficult to manage in the 3D Route Editor.

The workspace contains:

- **Left panel:** the `PATH EDITOR` path list and New, Edit, Clone, and Del
  controls. Activity-related controls appear below it but are outside this
  guide.
- **Center:** the full TrackDB map.
- **Right panel:** `SELECTION & LEGEND` during normal viewing, or `PATH
  CONTROLS` while creating, editing, or cloning a path.
- **Top map toolbar:** route/path fitting, rotation, and display-layer toggles.

## Open and Close the Workspace

1. Load the route in the Route Editor.
2. Press **F10** to open the full-screen Activity Builder and Path Editor.
3. Press **F10** again to close it and return to the Route Editor.

The workspace opens maximized so the complete map and both side panels remain
usable on large routes.

## Navigate the Map

| Action | Control |
| --- | --- |
| Zoom | Roll the mouse wheel. Zoom stays centered under the pointer. |
| Pan | Drag with the left or middle mouse button. |
| Fit the entire TrackDB | Click `Fit Route`, or double-click the map. |
| Fit the selected path | Select a saved path and click `Fit Path`. |
| Rotate the map | Click `Rotate 90°`. Each click rotates clockwise. |
| Inspect a switch | With Junctions visible, click its orange symbol. |

The compass continues to show geographic north after the map is rotated.
If track is difficult to select while editing, zoom closer and click directly
on its line.

### Map display layers

The toolbar provides these independent display toggles:

- `Junctions` — TrackDB switches and their current/default alignment.
- `Interactives` — signals, stations/platforms, sidings, service points, and
  level crossings.
- `Markers` — route markers from the `.mkr` file matching the active route.
- `Labels` — map labels appropriate to the current scale.
- `Tile Grid` — route tile boundaries.

These layers are enabled by default. Turn off unneeded layers to declutter a
dense yard or large route.

### Map and path colors

| Color/symbol | Meaning |
| --- | --- |
| Grey | TrackDB track |
| Orange switch | Switch at its default route |
| Cyan switch | Switch changed for the draft path |
| Blue | Track endpoint |
| Yellow path | Selected saved path |
| Green point | Path start |
| Red point | Path end |
| Magenta path | Unsaved main-path draft |
| Cyan path | Track used more than once by the draft |
| Orange path | Passing siding |

Interactive symbols use paired green/red dots for a signal, a light-blue bar
for a station or platform, a blue diamond for a siding, amber `[P]` for a
service point, an orange X for a level crossing, and a purple flag for a route
marker.

## Paths and Activities Are Different

A path defines a route through the TrackDB and is saved as a `.pat` file. It
contains the start, endpoint, switch routing, reverse points, waits or advanced
points, and optional passing branches.

An activity may later refer to that path, but creating or editing the path does
not require creating or selecting an activity. Deleting or renaming a path
already used by activities can break those references.

## Quick Start: Create a Complete Simple Path

This is the shortest complete workflow.

1. Back up the route, then press **F10**.
2. In the left `PATH EDITOR`, click `New Path`.
3. Click the track where the train should begin. A green start point appears,
   and the magenta preview flows through the current switch alignment.
4. If the preview flows the wrong way, click `Reverse Start`.
5. Follow the magenta route toward the destination. Click any orange switch
   that must change; the path recalculates immediately.
6. Click `Place Endpoint`, then click the desired location on the magenta path.
7. Click `Meta Data` and complete all fields.
8. Click `Check Path` and correct every reported problem.
9. Click `Save Path`.
10. Select the saved path in the left list and click `Fit Path` to inspect the
    final yellow route.
11. Close with **F10**, save the route as appropriate, and test the path in the
    intended Open Rails operation before distributing it.

`Save Path` validates the complete path again even if `Check Path` has already
passed.

## The Flowing-Water Method

The editor builds the route like flowing water:

1. The start point establishes the first track vector and travel direction.
2. Magenta follows the TrackDB through the active switch exits.
3. Clicking an orange switch throws it for the draft and recalculates the
   downstream route.
4. The endpoint stops the flow at the chosen point.

Orange switches remain live throughout a New, Edit, or Clone session; no
separate switch tool is required. A successful throw appears cyan. Throwing a
main-path switch clears the existing endpoint and resumes flow toward the new
natural end, so place or replace the endpoint after routing changes.

If a switch change cannot preserve a continuous main route or reconnect an
existing passing path, GenX rejects the change and restores the prior valid
alignment. A trailing-point switch that cannot accept the requested route is
also restored rather than being shown as a successful change.

## Path Controls

The right `PATH CONTROLS` panel appears only during a New, Edit, or Clone
session.

### Start and endpoint

- `Place Start` — arms start placement. Click a track line.
- `Reverse Start` — flips the initial travel direction. It clears the endpoint,
  waits, and passing sidings because the flow must be rebuilt.
- `Place Endpoint` — arms endpoint placement. Click the existing magenta path;
  an endpoint cannot be placed on unrelated track.

Only one placement button is active at a time. After a point is placed, the
editor returns to its normal selection mode.

### Reverse points

Use a reverse point where the train must change direction during the path.

1. Build the magenta route to the reversal location.
2. Click `Reverse Point`.
3. Click directly on the magenta path.
4. Route the new leg by throwing switches as required.
5. Place the endpoint on the final magenta leg.

Adding a reverse point clears the endpoint, waits, and passing sidings so they
cannot remain attached to an obsolete leg. Re-add them after the route is
correct. Multiple reverse points are supported.

### Wait points

A wait point must be placed on the current magenta main path.

1. Choose one wait mode:
   - `Wait for duration`: 0–1092 minutes plus 0–59 seconds. A zero duration is
     saved as the minimum valid one-second wait.
   - `Wait until clock time`: hour 0–23 and minute 0–59.
2. Click `Add Wait Point`.
3. Click the intended magenta segment.

Duration points display `W`; clock-time points display `T`.

### Advanced ORTS points

> **Compatibility:** These controls require the Open Rails Extended AI train
> shunting functionality. Do not assume they will work in MSTS or in an Open
> Rails configuration that does not support that extension.

Choose an operation, set its options, click the corresponding Place button,
then click the magenta path:

- `Blow Horn` — horn duration from 1 to 10 seconds; marker `H`.
- `Uncouple Cars` — keep the front or rear of the train, keep 1–99 cars
  including locomotives, and pause 0–99 seconds afterward; marker `U`.
- `Join / Split` — join the nearby train and continue the shunting move;
  marker `J`.
- `Request Pass Red` — request permission for the AI train to pass the next
  red signal; marker `R`.

Validate advanced operations in the exact Open Rails version and activity in
which they will be used.

## Add a Passing Siding

A passing siding is an alternate orange branch that leaves and rejoins the
ordered magenta main path at two different switches.

1. Complete the main magenta route first.
2. Click `Add Passing Siding`.
3. Click directly on the alternate siding track, not on the main path.
4. GenX searches the TrackDB for both connections back to the main path.
5. Confirm that the discovered branch is orange and reconnects at the intended
   two switches.
6. Click switches on the orange branch if it has a valid long/short or crossover
   alternative. The entire branch is recalculated after each accepted throw.
7. Run `Check Path` before saving.

The editor rejects a clicked track that does not reconnect at two different
main-path switches. It also prevents duplicate assignment of the same siding.
Multiple passing paths are allowed, but two passing paths may not begin at the
same main-path switch. Save rejects an orange branch whose boundaries no longer
match the ordered main route.

## Select, Delete, Undo, and Redo

With no placement button active, click a draft start, endpoint, reverse diamond,
wait/advanced point, or orange passing siding to select it. Then use `Delete`,
the **Delete** key, or **Backspace**.

Deletion has structural consequences:

- Deleting the start clears the complete draft route.
- Deleting the endpoint returns the route to its natural flowing end.
- Deleting a reverse point rebuilds the remaining route and clears controls
  that depended on the removed leg.
- Deleting a wait/advanced point removes only that point.
- Deleting a passing siding removes only that orange branch.

Use `Undo` or **Ctrl+Z** to reverse the last path operation. Use `Redo` or
**Ctrl+Y** to reapply it. The editor retains up to 100 draft states. Undo/Redo
history belongs to the active path-edit session.

## Metadata and File Naming

Click `Meta Data` before saving and complete:

- **Path file / ID** — the `.pat` base name. Use letters, numbers, underscore,
  and hyphen. If `.pat` is typed, the editor removes the extension.
- **Path name** — the display name shown to users.
- **Start label** — descriptive starting location.
- **End label** — descriptive destination.
- **Available as a player path** — enable when the path is intended for player
  service selection.

All fields are required. A new filename must be unique. After the first save,
the file/ID is held stable to protect activity references; use Clone when a
separate filename is needed. Canceling the metadata dialog restores the
session's prior metadata.

## Check and Save the Path

`Check Path` verifies that:

- the start and endpoint exist;
- every main and reverse leg is continuous through its switch routes;
- wait and advanced points still belong to the correct active leg;
- passing paths reconnect to the ordered main path;
- passing-path boundaries do not conflict.

Cyan overlap is informational: it shows track traversed more than once, often
because of a reverse move. Inspect it to make sure the repeated movement is
intentional.

`Save Path` requires complete metadata and repeats the structural validation.
On success it writes `ROUTES\<route>\PATHS\<Path ID>.pat` as a UTF-16,
MSTS/Open Rails-compatible file. The write uses a temporary safe-save
transaction and is committed only when the complete file has been written.
The edit session then closes and the saved route is shown in yellow.

Do not exit an unfinished edit believing that the draft has been saved. The
file changes only after a successful `Save Path`.

## Edit an Existing Path

1. Select the path in the left `PATH EDITOR` list.
2. Click `Edit`.
3. Click a control to select it, or click switches to change the route.
4. Use Delete and the placement controls to reconstruct the required portion.
5. Run `Check Path`.
6. Click `Save Path`.

The selected path must contain readable path nodes and TrackDB control points.
If those basic records cannot be read, GenX refuses to open an unsafe edit.

### Repair an open or broken main path

If the saved route is no longer continuous on the current TrackDB, GenX can
still expose the surviving start and endpoint as repair controls.

1. Read the diagnostic shown in the right panel.
2. Select the obsolete start or endpoint and press Delete.
3. Place the replacement control.
4. Rebuild the route with the live switches.
5. Re-add any controls cleared during reconstruction.
6. Check and save only after the complete route is correct.

### Repair unreadable passing branches

When the main route is intact but one or more saved passing branches cannot be
reconstructed on the current TrackDB, GenX asks whether to discard only those
unreadable branches from the draft.

- Choose **No** to leave the original PAT unchanged and read-only.
- Choose **Yes** to open the intact main route and omit the unreadable branches
  from the editable draft. The PAT still remains unchanged until `Save Path`.

After choosing Yes, inspect the entire route and save only if removing those
branches is the intended repair.

## Clone a Path

Clone is the safest way to make a new variation while retaining the original.

1. Select a path that has already been saved to disk.
2. Click `Clone`.
3. Enter a unique file name and display name.
4. Modify the cloned draft.
5. Check and save it.

The filename is sanitized to the allowed characters. The clone is written as a
separate UTF-16 PAT and immediately opened for editing. If the Clone edit
session is canceled, GenX attempts to remove the temporary cloned PAT and warns
if cleanup fails.

## Delete a Standalone Path

`Del` removes the selected `.pat` file after confirmation. This is destructive
and can leave activities that reference the path needing repair.

Before deleting:

1. Back up the route.
2. Record the path file/ID.
3. Confirm that no required activity uses it.
4. Prefer Clone if the intention is to create a replacement variation.

## Cancel an Edit Session

The active New, Edit, or Clone button remains checked during its session.
Toggle that button off to cancel. Unsaved changes to an existing path are not
written. A canceled New path has no saved PAT; a canceled Clone follows the
temporary-clone cleanup behavior described above.

## Recommended Validation Procedure

After every important path change:

1. Click `Check Path` and resolve every failure.
2. Inspect start, endpoint, reverse points, waits, and passing branches at a
   close zoom.
3. Click `Save Path` and verify the saved-path confirmation.
4. Select the path again and use `Fit Path`.
5. Close and reopen the full map, then reselect the path.
6. Confirm that the yellow saved route and all controls reconstruct correctly
   in Edit mode.
7. Test the path in the intended Open Rails activity or service environment.
8. For advanced points, test with Extended AI shunting enabled.

## Troubleshooting

### Clicking track does nothing

- Begin a New, Edit, or Clone session.
- Select `Place Start`, `Place Endpoint`, `Reverse Point`, `Add Wait Point`, or
  `Add Passing Siding` before placing that type of control.
- Zoom closer and click directly on the visible line.
- Endpoint, reverse, wait, and advanced points must be on the magenta main path.

### The path flows the wrong way from the start

Click `Reverse Start`. Rebuild the endpoint, waits, and passing sidings that are
cleared by the direction change.

### A switch will not stay thrown

The requested exit could not produce a valid continuous route, or an existing
passing path could no longer reconnect. GenX restores the prior valid state.
Inspect the surrounding TrackDB, route the main path in a different order, or
remove and rebuild the dependent passing siding.

### The endpoint disappeared

This is expected after a switch throw, start-direction reversal, or structural
route change. The preview returns to its natural end; use `Place Endpoint` again
after the route is correct.

### A passing siding is rejected

Confirm that the clicked alternate track reconnects to the current magenta path
at two different switches. Build the main path first, zoom closer, and avoid a
second passing path that begins at the same main-path switch.

### Check Path passes but Save Path does not

Save also requires complete metadata and a unique filename for a new path. Read
the right-panel diagnostic, correct the missing metadata or structural conflict,
then check and save again.

### The saved path is yellow, not magenta

Yellow is the normal selected saved-path color. Magenta is reserved for the
editable, unsaved main-path draft.

### An activity no longer finds its path

The path may have been deleted or an external tool may have renamed its PAT ID.
Restore the backed-up PAT or repair the activity reference. GenX deliberately
keeps a saved path's filename stable during normal Edit operations.

## Final Checklist

- Route backup exists.
- Start direction is correct.
- Every intended switch route is visible.
- Endpoint is on the final magenta leg.
- Reverse moves and cyan overlaps are intentional.
- Wait and advanced points are on the correct legs.
- Every orange passing siding reconnects at both ends.
- Metadata is complete and the player-path setting is correct.
- `Check Path` passes.
- `Save Path` succeeds.
- The path reloads and fits correctly as a yellow saved path.
- The path is tested in its intended Open Rails use.

