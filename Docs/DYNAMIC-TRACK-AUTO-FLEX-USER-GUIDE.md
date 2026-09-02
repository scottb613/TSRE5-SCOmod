# Dynamic Track and Auto-Flex User Guide

This guide explains what Dynamic Track and Auto-Flex are, when to use them, and
how to build and verify a smooth connection between two existing track ends in
TSRE GenX.

## Auto-Flex in Plain Language

Auto-Flex fills a gap between two open TrackDB ends with one calculated Dynamic
Track object.

You place a short Dynamic Track starter at the first open end, press
**Auto-Flex**, and click the second open end. GenX then calculates the required
straights and curves, aligns both joints, calculates the grade between their
elevations, validates the result, and rebuilds the Dynamic Track's TrackDB
entry.

```text
existing track A                 existing track B
===============> o             o <===============
                  \           /
                   \         /
                    Auto-Flex
                  calculated path
```

Auto-Flex is useful for:

- closing a gap between two independently laid track sections;
- joining offset or differently aligned endpoints;
- building a compound connection containing as many as two curves;
- creating an S-curve where a single ordinary curve cannot make both joints;
- joining endpoints at different elevations with one consistent grade plane.

Auto-Flex does **not**:

- draw arbitrary freehand track;
- connect to the middle of an ordinary TrackDB section;
- create a turnout or change a switch definition;
- repair a damaged TrackDB automatically;
- move the existing destination track to make a solution fit;
- conform the surrounding terrain;
- guarantee that every pair of endpoints has a physically valid solution.

## The Current GenX Solver

Current GenX releases expose one built-in Dynamic Track choice:

**Dynamic Track (Auto-Flex)** using **NextGen Flex S-C-S-C-S**.

The letters describe the five possible section slots:

| Letter | Section |
| --- | --- |
| S | Straight |
| C | Curve |
| S | Straight between the curves |
| C | Second curve |
| S | Final straight |

The solver does not have to use all five sections. A simple connection might
be one straight, one curve, or Straight-Curve-Straight. A compound alignment or
full S-curve can use both curve slots.

Older GenX development builds offered a Classic/NextGen selector. **Classic
Flex has been retired from the current user interface.** The current NextGen
solver handles simple connections as well as the more complex S-C-S-C-S cases.

## Why S-C-S-C-S Improves on Traditional S-C-S

Traditional Dynamic Track Flex uses an **S-C-S** pattern:

```text
Straight -> one circular Curve -> Straight
```

That pattern works well when one circular arc can leave the first tangent and
arrive on the second tangent. It is simple and predictable, but one curve has
limited geometric freedom. Many pairs of otherwise reasonable endpoints cannot
be joined by one arc without missing an endpoint, meeting it at the wrong
angle, turning backward, or requiring an impractical alignment.

NextGen can use **S-C-S-C-S**:

```text
Straight -> Curve 1 -> middle Straight -> Curve 2 -> Straight
```

The second curve and middle straight give the solver another way to change
heading and lateral position before it reaches the destination. This permits:

- full S-curves, where the two curves turn in opposite directions;
- offset connections between nearly parallel tracks;
- compound alignments, where two curve stages solve a connection that one
  circular arc cannot;
- better approach alignment when both endpoints have fixed tangents;
- simpler connections too—the unused sections are disabled when one straight
  or one curve is sufficient.

### A typical lateral-offset problem

Two parallel or nearly parallel track ends may face the same general direction
but sit on different centerlines:

```text
traditional one-curve attempt

source  ==================\
                           \____  misses the required destination tangent
destination     =================

NextGen two-curve solution

source  ==================\
                           \________
                                    \================ destination
                         Curve 1       Curve 2
```

The first curve moves the alignment away from the source centerline. The second
curve removes that heading change so the track arrives parallel and tangent to
the destination. A single curve normally cannot perform both jobs.

### What is actually improved

| Traditional S-C-S | NextGen S-C-S-C-S |
| --- | --- |
| At most one curve | As many as two curves |
| Best for a direct tangent-arc-tangent join | Also handles reverse and compound alignments |
| Limited ability to change lateral offset and then recover heading | First curve creates the offset; second can recover the destination heading |
| More endpoint pairs have no valid solution | A larger set of endpoint positions and tangents can be solved |
| Older workflow depended more heavily on endpoint order and fragile database assumptions | Current solver tests both endpoint directions and validates the complete connection |

S-C-S-C-S is therefore not simply “more curved track.” It gives Auto-Flex
enough degrees of freedom to satisfy both endpoint positions and both tangent
directions while still choosing the shortest fully validated candidate it can
find.

### What S-C-S-C-S does not improve automatically

- It does not create a turnout.
- It does not choose a railway-design speed or guarantee a desirable minimum
  radius for the route.
- It does not create true transition spirals or easement curves. Each `C` is a
  circular Dynamic Track curve, and curvature still changes at section
  boundaries.
- It does not make an impossible, backward, excessively long, or obstructed
  connection valid.
- It does not replace visual inspection and an Open Rails train test.

The additional geometric freedom is the S-C-S-C-S improvement. Direction-
neutral endpoint handling, transactional application, exact tangent/endpoint
checks, grade-plane consistency, and TrackDB rebuilding are separate GenX
reliability improvements wrapped around that richer solver.

## How GenX Protects the Route

Auto-Flex is transactional. GenX first builds and validates a candidate without
changing the selected Dynamic Track. A candidate is rejected if it:

- cannot find a valid source or destination TrackDB connection;
- is effectively zero length;
- exceeds the 2048 m Dynamic Track connection limit;
- runs backward or forms an unintended U-turn;
- misses either endpoint;
- fails to match the endpoint tangents;
- creates an invalid straight-to-curve or curve-to-straight seam;
- connects only one end;
- requires changing Dynamic Track that owns an interactive track item.

Only a fully validated candidate is applied. GenX then removes the starter's
old TrackDB entry, writes its new sections, and adds the corrected entry back to
the TrackDB before reporting success.

Rebuilding a Dynamic Track's TrackDB entry clears the ordinary Undo history.
Save and make a checkpoint before Auto-Flex; do not plan on Ctrl+Z as recovery
after a successful solve. A rejected solve makes no candidate change.

This protection is not a substitute for a route backup or an Open Rails test.

## Before You Begin

1. Back up the complete route folder.
2. Open the route and confirm the two intended track ends are visible and
   belong to the TrackDB.
3. Save any unrelated work before starting the connection.
4. Remove signals, platforms, sidings, pickups, sound regions, or other
   interactive track items from the Dynamic Track starter if it must be
   replaced. GenX blocks the edit when removing its old database entry would
   orphan an interactive.
5. Confirm **AutoTDB** is on in the Control Panel. The status should report
   `AutoTDB: ON`.
6. Load the terrain and neighboring tiles around both endpoints.

For a first attempt, choose two uncomplicated open ends on the same tile or on
neighboring tiles, with moderate separation and no switch immediately at the
joint.

## The Two Ends Must Be Real TrackDB Connections

Auto-Flex searches within 4 m of the Dynamic Track starter's origin for the
source connection and within 4 m of your final click for the destination
connection.

Use open TrackDB endpoints, not merely visible rail artwork. A static scenery
shape can look like track without being part of the TrackDB. Likewise, clicking
the middle of a continuous track vector does not create a new attachment point.

Good preparation looks like this:

- both ends are already in the TrackDB;
- each end is open and available for a new connection;
- the gap is less than 2048 m;
- the intended path generally leaves each endpoint in its forward direction;
- enough room exists for a practical curve radius;
- endpoint elevations and grades have been set before Auto-Flex.

If necessary, use the yellow TrackDB guide and raised endpoint markers to
distinguish database geometry from the rendered track mesh.

## Complete Auto-Flex Walkthrough

### 1. Inspect and prepare the alignment

1. Fly or move the camera above the complete gap.
2. Inspect both open ends from above and from a low side angle.
3. Verify that the intended connection will not double back through either
   endpoint.
4. If the ends have different elevations, verify that the resulting average
   grade is reasonable for the route.
5. If an end needs a final position, direction, or grade correction, make that
   correction before placing the Dynamic Track starter.

Auto-Flex treats endpoint directions as axes and tests both valid travel
directions. You do not have to guess which database end was stored first.

### 2. Select the built-in Dynamic Track

1. Press **F1** to open **Objects**.
2. In the object selector, open the built-in NextGen track group under the
   **Other** categories.
3. Select **Dynamic Track (Auto-Flex)**.
4. Press **Q** or choose **Place**.

Dynamic Track is an editor-owned tool. GenX intentionally ignores route or
addon REF-file `dyntrack` entries and supplies this one known placement choice.

### 3. Place the Dynamic Track starter

1. Zoom closely onto the first open track end.
2. Put the placement pointer on that endpoint.
3. Click once to place the Dynamic Track starter.
4. Leave the newly placed Dynamic Track selected.
5. Confirm that its origin is at or very near the open TrackDB end.

The source search tolerance is 4 m, but careful placement makes the intended
connection unambiguous. On success, Auto-Flex moves the starter to the exact
database source point and applies the resolved source orientation.

If the starter is facing the visually unexpected direction, do not assume it
will fail. The current solver checks both tangent directions. The important
requirement is that its origin is beside the correct external TrackDB end.

### 4. Start Auto-Flex

1. With the Dynamic Track still selected, look at its **DYNAMIC TRACK**
   Properties panel.
2. Confirm the **FLEX MODE** card says **NextGen Flex S-C-S-C-S**.
3. Select **Auto-Flex**.
4. The button becomes the active tool while GenX waits for the destination.

Do not change the five section fields before the solve. They describe the
starter now and will be replaced with the calculated solution.

### 5. Choose the destination

1. Move the pointer to the second open TrackDB endpoint.
2. Zoom close enough to avoid a nearby parallel track or overlapping endpoint.
3. Click once.

GenX finds the closest valid destination connection within 4 m, excluding the
selected Dynamic Track itself. It rejects an attempt when source and
destination resolve to the same TrackDB vector.

### 6. Read the result

On success:

- the normal success feedback plays;
- the Dynamic Track remains selected;
- its rendered path changes to the solved alignment;
- the yellow TrackDB guide follows the same alignment;
- **Length** reports the calculated centerline length;
- **Curves** reports the number of active curve sections;
- the five section rows show which straights and curves were used;
- **Grade** reports the average grade derived from endpoint rise over solved
  path length.

On failure:

- GenX plays stopped/error feedback;
- the tool returns to Select;
- the existing Dynamic Track and TrackDB remain unchanged.

Do not keep clicking after a failure. Inspect the endpoints and correct the
cause before starting Auto-Flex again.

## Reading the Dynamic Track Panel

The upper fields identify the selected object:

| Field | Meaning |
| --- | --- |
| UiD | World-object identifier |
| Tile X / Tile Z | Tile containing the Dynamic Track origin |
| Index | Route TSECTION entry assigned to this Dynamic Track layout |
| Length | Total calculated track centerline length |
| Curves | Number of active curve sections |

The **SECTIONS** area contains:

- **First Straight** — length in metres;
- **First Curve** — angle and radius;
- **Second Straight** — length in metres;
- **Second Curve** — angle and radius;
- **Third Straight** — length in metres.

Unchecked or hidden sections are not used by the current alignment.

The section controls support advanced manual Dynamic Track editing, but manual
changes can make the visible object disagree with the intended endpoints or
its existing TrackDB geometry. For an Auto-Flex connection, rerun Auto-Flex
after changing the endpoint plan instead of hand-editing a solved alignment.

## Grade Behavior in Auto-Flex

Auto-Flex gives the complete connection one physical vertical plane between
the two endpoint elevations. The generated mesh, its internal straight/curve
boundaries, and the TrackDB vectors use that same plane.

The displayed average grade is:

```text
endpoint elevation difference / solved track length
```

This is better than using the straight-line gap because a curved connection is
longer than its direct chord.

Set endpoint heights and grades before solving. Changing the Dynamic Track
grade afterward can move the far end away from the fixed destination and
create a vertical joint even when the plan view still looks correct.

The Grade control can display and edit the same physical grade as:

- Permille (`‰`);
- Percent (`%`);
- 1 in X metres;
- Angle in degrees.

For example, `1.0%`, `10‰`, and `1 in 100` describe the same grade magnitude.
The sign indicates the uphill/downhill direction relative to the object's
stored direction.

For Grade Ruler, direct grade editing, Lock Grade, Grade Helper transitions,
and orange/cyan/red Grade Symbols, see the
[Grade Helper User Guide](GRADE-HELPER-USER-GUIDE.md).

## Validate the Connection in GenX

After a successful solve:

1. Orbit around both joints at a low camera angle.
2. Inspect the rail tops for a lateral kink or vertical step.
3. Inspect every internal straight/curve transition.
4. Follow the yellow TrackDB guide from source to destination.
5. Confirm there is one continuous guide, not a duplicate or displaced vector.
6. Check **Length**, **Curves**, the active section rows, and **Grade** for a
   plausible result.
7. If the result is not the intended alignment, do not attach interactives or
   continue building. Re-flex deliberately to the correct destination or
   restore the pre-Flex checkpoint.
8. Save with **Shift+Ctrl+S**.
9. Close and reopen the route.
10. Reinspect the Dynamic Track and TrackDB guide before continuing work.

Do not add signals, platforms, pickups, or other track items to the new Dynamic
Track until its geometry has passed the save/reopen and simulator checks.

## Validate the Connection in Open Rails

The GenX mesh is an editor preview. Open Rails is the final test of physical
track geometry and train behavior.

1. Back up or checkpoint the route after the GenX save/reopen check.
2. Start a short Explore Route or test activity approaching the connection.
3. Drive through it slowly in the first direction.
4. Watch for:
   - a lateral jerk at either endpoint;
   - a vertical bump or wheel unloading;
   - a kink at an internal curve/straight transition;
   - a derailment or broken TrackDB path;
   - incorrect superelevation or route-specific track-profile behavior.
5. Repeat in the opposite direction.
6. Repeat at the intended operating speed.
7. Preserve `OpenRailsLog.txt` if any failure occurs.

For a graded connection, test both travel directions. A reversed grade frame
can be difficult to recognize in a still editor view but obvious under a moving
train.

## When Auto-Flex Rejects a Connection

A rejection means the candidate failed before GenX committed it. Common causes
are:

| Symptom or message | Meaning | Corrective action |
| --- | --- | --- |
| No external source connection | Starter origin is not near another TrackDB end | Reposition or recreate the starter within 4 m of the intended source endpoint. |
| Source connection is more than 4 m from the origin | The nearby database point is outside the allowed source tolerance | Place the starter closer to the exact endpoint. |
| No destination connection | Final click is not near a valid TrackDB end | Zoom in and click within 4 m of the intended endpoint. |
| Source and destination belong to the same vector | Both clicks resolved to one continuous TrackDB vector | Choose the actual opposite open endpoint or separate the intended database geometry correctly. |
| Connection collapses into the same point | The two resolved connections are too close | Correct the endpoint choice; do not use Auto-Flex for a zero-length join. |
| Connection exceeds 2048 m | Gap is beyond the Dynamic Track limit | Redesign the alignment with intermediate fixed endpoints and more than one connection. |
| No direction produced solid position, tangent, and grade connections | No valid S-C-S-C-S candidate fits the endpoint geometry | Increase available space, reduce the offset/angle, alter endpoint placement, or build the alignment in stages. |
| Track looks connected but Auto-Flex cannot find it | Visible object is absent from or inconsistent with TrackDB | Repair/add the object to TrackDB before retrying. |
| Solver chooses the wrong nearby end | Parallel or overlapping endpoint was closer to the click | Re-flex to the intended end or restore the checkpoint, then zoom closer and remove the ambiguity. |

Repeated rejection is usually an alignment problem, not a request to force the
same click. Change the geometry or divide the connection into sensible stages.

## Planning Better Auto-Flex Alignments

- Leave adequate tangent distance after turnouts and before platforms.
- Avoid starting or ending directly inside complicated junction geometry.
- Prefer moderate, route-appropriate curve radii.
- Keep the intended path in front of both endpoint tangents. A connection that
  must immediately reverse is intentionally rejected.
- Set the endpoint elevations first. Auto-Flex solves the path between known
  endpoints; it is not a vertical-alignment design tool.
- Use an intermediate fixed track end when one 2048 m connection would be too
  long or too geometrically ambitious.
- Inspect neighboring parallel track so the 4 m endpoint search cannot select
  the wrong line.
- Complete and test the Dynamic Track before attaching interactives.

## Terrain Around Dynamic Track

Auto-Flex creates track geometry only. Shape the terrain afterward:

1. Complete, save, reopen, and simulator-test the Dynamic Track first.
2. Select the Dynamic Track.
3. Press **F** to conform terrain to the complete selected track shape using the
   current F2 embankment settings.
4. Use **Shift+F** for surrounding smoothing when appropriate.
5. Use **Ctrl+F** for the selected TrackDB vector within the current tile.
6. Use the F2 **Conform TDB/RDB** brush for small freehand corrections.
7. Inspect both sides of every tile boundary.
8. Save and reopen after major terrain work.

See the
[Terrain Improvements User Guide](TERRAIN-IMPROVEMENTS-USER-GUIDE.md) for
embankment, cutting, bias, conforming, Undo, and save details.

## Essential Shortcuts

| Key | Action |
| --- | --- |
| F1 | Open Objects and Dynamic Track Properties |
| Q | Toggle Place mode |
| E | Toggle Select mode |
| Ctrl+Q | Toggle AutoTDB |
| Ctrl+Z | Undo the last supported edit |
| Shift+Ctrl+S | Save the route |
| F | Conform terrain to the selected track/object/ruler |
| Ctrl+F | Conform along the selected TrackDB/RoadDB vector in the current tile |
| Shift+F | Smooth terrain around the selected track/object/ruler |
| Z | Toggle the selected track object in or out of TrackDB |

Use `Z` only when you understand the database consequence. Removing a visible
track object from TrackDB breaks the operational path until it is correctly
added again.

## Recovery and Undo

Auto-Flex database replacement is not an ordinary object movement. A successful
solve rebuilds the selected object's TrackDB entry and clears the ordinary Undo
history.

If the solved alignment is wrong but Auto-Flex succeeded:

1. Do not rely on **Ctrl+Z**.
2. Do not add interactives or place more track through the unwanted joint.
3. If the starter source is still correct, select the Dynamic Track, start
   Auto-Flex again, and click the intended destination.
4. If the source, database state, or route intent is uncertain, restore the
   complete pre-Flex checkpoint.

If the route has already been saved:

1. Do not attach more track items or continue building through the bad joint.
2. Close without further saves if the pre-save state is still available through
   the route backup.
3. Restore the complete matched route backup when database state is uncertain.

Avoid repairing a questionable Dynamic Track by editing only its world-file
object or only the TrackDB. The world object, route TSECTION entry, and TrackDB
geometry must agree.

## Troubleshooting

| Problem | Action |
| --- | --- |
| Dynamic Track is not in a route REF file | Use the built-in NextGen track group; GenX intentionally owns this item. |
| Auto-Flex button is not visible | Select an existing Dynamic Track object so its Properties panel is shown. |
| Auto-Flex immediately fails | Check that the starter origin is within 4 m of an external TrackDB endpoint and AutoTDB is on. |
| Destination click does nothing useful | Click an open TrackDB endpoint, not rail artwork or the middle of a vector. |
| Connection requires one large loop | Redesign it; backtracking and unintended U-turn candidates are rejected. |
| Connection is longer than 2048 m | Divide the alignment using intermediate fixed endpoints. |
| Wrong parallel track was selected | Re-flex to the intended endpoint or restore the checkpoint, then zoom closer and remove endpoint ambiguity before retrying. |
| Edit is blocked because of interactives | Remove or relocate attached signals/platforms/sidings/pickups/regions before replacing the Dynamic Track entry. |
| Visible rails and yellow guide disagree | Stop and restore the pre-Flex checkpoint; do not save an object/TrackDB mismatch. |
| Joint looks smooth but train bumps | Recheck endpoint elevations, save/reopen, then inspect both directions in Open Rails. |
| Dynamic Track is missing its normal texture | GenX uses procedural/fallback material for editing; Open Rails remains the final appearance check. |

## First-Time Practice Exercise

Practice on a disposable route copy before using Auto-Flex in finished track:

1. Lay two short straight tracks with open ends facing generally toward one
   another.
2. Offset one end sideways so a plain straight cannot join them.
3. Give both ends the same elevation for the first attempt.
4. Place **Dynamic Track (Auto-Flex)** at the first open end.
5. Select **Auto-Flex** and click the other open end.
6. Inspect Length, Curves, sections, and the yellow TrackDB guide.
7. Save, reopen, and drive through in both directions.
8. Repeat with the second endpoint at a modestly different elevation.
9. Repeat with endpoints requiring an S-curve.
10. Deliberately click away from the endpoint and confirm rejection leaves the
    existing object unchanged.

This exercise makes the solver's job clear before the route geometry becomes
complicated.

## Final Connection Checklist

- [ ] The complete route was backed up before track/database work.
- [ ] Both source and destination are real, open TrackDB endpoints.
- [ ] The gap is under 2048 m and has space for a practical alignment.
- [ ] Endpoint positions, directions, elevations, and grades were prepared
  before Auto-Flex.
- [ ] AutoTDB is on.
- [ ] Dynamic Track (Auto-Flex) was placed at the source endpoint.
- [ ] Auto-Flex completed with normal success feedback.
- [ ] Length, Curves, active sections, and Grade are plausible.
- [ ] The rendered rails and yellow TrackDB guide agree.
- [ ] Both external joints and every internal section transition look smooth.
- [ ] The route saved, closed, and reopened correctly.
- [ ] A train passed over the connection in both directions in Open Rails.
- [ ] Interactives were added only after geometry validation.

When all items pass, the Auto-Flex connection is ready for surrounding terrain,
signalling, and normal route construction.
