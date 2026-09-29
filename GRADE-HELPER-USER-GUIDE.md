# Grade Helper User Guide

This guide covers the complete TSRE GenX grade workflow: measuring a proposed
grade, reading and editing grade units, repeating a constant grade, building a
controlled piece-by-piece transition with Grade Helper, interpreting Grade
Symbols, validating the TrackDB, and testing the finished track in Open Rails.

## What Grade Helper Does

Grade Helper changes the grade of each newly placed, connected track or road
piece by a chosen step until an exact target grade is reached.

Example:

```text
Current Grade:    0.00%
Target Grade:     1.00%
Step Per Piece:   0.20%

placed pieces:  0.20% -> 0.40% -> 0.60% -> 0.80% -> 1.00%
```

The helper verifies every new piece is connected to the previously selected
TrackDB/RoadDB endpoint. It rejects a disconnected placement or a sequence
whose current grade no longer matches the expected grade.

Grade Helper:

- works with newly placed regular track and road pieces;
- starts from the physical grade of the selected existing piece;
- can transition uphill, downhill, toward level, or through level;
- treats Step Per Piece as a positive change magnitude;
- uses a smaller final step when necessary so it lands exactly on the target;
- automatically hands the finished target to Lock Grade;
- does not change previously placed pieces;
- does not operate Dynamic Track Auto-Flex;
- does not shape terrain;
- does not create mathematical vertical curves or easements.

Each track piece still has one physical grade. Grade Helper creates a controlled
series of discrete grade changes across connected pieces.

## The Complete Grade Tool Set

| Tool | Purpose |
| --- | --- |
| Ruler (grade) | Measure the straight-line grade between two terrain points without changing track |
| Track Grade field | Read or directly edit one selected track/road piece |
| Lock Grade | Apply one captured grade to every newly placed track/road piece |
| Grade Helper | Step newly placed connected pieces toward an exact target grade |
| Grade Symbols | Show steady grades, transitions, and direct crest/gully warnings |

These tools complement one another. A typical workflow is:

1. Measure the corridor with Ruler (grade).
2. Set the first track piece to the intended starting grade.
3. Use Grade Helper to create the transition.
4. Continue at the achieved grade with Lock Grade.
5. Inspect the result with Grade Symbols.
6. Save, reopen, and drive it in Open Rails.

## Grade Units

GenX can display the same physical grade in four forms:

| Unit | Meaning | 1% example |
| --- | --- | ---: |
| Percent | Vertical rise per 100 horizontal units | `1.00000%` |
| Permille | Vertical rise per 1000 horizontal units | `10.00000‰` |
| 1 in X | One vertical unit per X horizontal units | `1 in 100` |
| Angle | Track pitch in degrees | approximately `0.57294°` |

The conversions are:

```text
percent = permille / 10
permille = percent x 10
1-in-X = 100 / percent
angle = arctangent(percent / 100)
```

Positive and negative signs represent the physical uphill/downhill direction
relative to the track object's stored direction. Flipping an object can change
that stored direction, so use the Grade Symbol arrow and physical track view
when the sign alone is ambiguous.

The **Units** selector changes display and entry units. It does not change the
physical grade merely because another unit is selected.

In Grade Helper, **Current Grade** and **Next Grade** are always shown in
percent. **Target Grade** and **Step Per Piece** follow the Units selection in
the parent Track Properties panel.

Percent or permille is usually the clearest choice for designing transitions.
When 1-in-X is selected, a Step Per Piece entry is interpreted as the grade
magnitude represented by that ratio. For example, a step of `1 in 500` means a
`0.20%` grade change per piece; it does not mean “add 500 to the current X
value.” Angle steps are likewise converted to their equivalent physical grade
change.

## Before Editing Track Grade

1. Back up the complete route folder.
2. Save unrelated work.
3. Confirm the relevant track or road objects are present in TrackDB/RoadDB.
4. Turn **AutoTDB** on. The Control Panel should report `AutoTDB: ON`.
5. Keep **Place Guard** on.
6. Open **View > Grade Symbols**.
7. Load the complete transition area and neighboring tiles.
8. Identify the exact connected endpoint where new track will continue.

For a first practice session, work on a disposable route copy with simple
single-path track pieces and no nearby junction or crossover.

## Part 1 — Measure with Ruler (grade)

The magenta Grade Ruler measures the average straight-line grade between two
terrain-snapped points. It changes no terrain, track, or database geometry.

### Place the Grade Ruler

1. Press **F1** to open **Objects**.
2. In **Other**, open the built-in **TSRE Tools** group.
3. Select **Ruler (grade)**.
4. Press **Q** or select **Place**.
5. Click the first terrain point.
6. Click the second terrain point.
7. Press **E** or select **Select**.

The completed ruler has:

- a magenta line and posts;
- two selectable endpoint handles;
- **Game Length** and **Geo Length** readouts;
- **Average Grade** in the selected unit.

Only two endpoints are accepted. In Select mode, drag either endpoint to revise
the measurement. When released, the moved endpoint snaps to terrain and the
grade readout updates. **Ctrl+Z** can undo supported ruler placement or movement
edits.

### Important Grade Ruler limitations

- It measures between terrain heights, not railhead heights.
- It measures the direct horizontal run, not the length of a future curved
  track alignment.
- It does not account for bridges, cuts, fills, or planned vertical clearance.
- It is a planning aid, not an automatic grade command.
- There can be only one route-wide special Water, PolyVeg, or Grade Ruler.
  Starting another special ruler removes the previous one. The ordinary
  measurement Ruler is separate.

Save and reopen if the Grade Ruler will remain as a route planning reference.

## Part 2 — Read or Edit One Track Piece

1. Press **E** or select **Select**.
2. Select the track or road piece.
3. In its Properties panel, find the **Grade** area.
4. Choose **Permille**, **Percent**, **1 in X**, or **Angle**.
5. Read the displayed value.
6. To change it, enter the new value in the active unit field.
7. Inspect the object and both connected ends immediately.

Direct grade editing rotates the selected piece to the requested physical
grade. It does not automatically redesign neighboring track, move the next
endpoint into place, or create a vertical transition. A changed piece can
therefore leave a joint misaligned until the adjoining track is deliberately
rebuilt.

The nearby general **Step** or adjustment-sensitivity field is for ordinary
object movement/rotation increments. It is **not** Grade Helper's Step Per
Piece.

Use direct editing to establish a known starting piece, correct a deliberately
isolated section, or set an endpoint before constructing the adjoining grade.

## Part 3 — Continue a Constant Grade with Lock Grade

Lock Grade captures the selected track/road piece's current physical grade and
applies it to each newly placed regular track or road piece.

### Lock Grade procedure

1. Select the final correct track or road piece in the existing alignment.
2. Confirm its displayed grade and physical uphill direction.
3. Select **Lock Grade** in the Track Properties panel.
4. Confirm the button changes to `Lock Grade: <value>%`.
5. Select the next track or road shape for placement.
6. Enter Place mode.
7. Place each new piece connected to the preceding endpoint.
8. Inspect each accepted joint and its Grade Symbol.
9. Turn **Lock Grade** off when the constant-grade run is complete.

Lock Grade always captures and internally applies the physical percent grade,
even if the panel displays another unit.

Lock Grade is appropriate for a long steady climb or descent. It is not a
transition tool. If the starting and desired final grades differ, use Grade
Helper.

## Part 4 — Build a Transition with Grade Helper

### Plan the transition

Before opening the helper, decide:

- **Current Grade** — supplied automatically by the selected starting piece;
- **Target Grade** — the steady grade you want to reach;
- **Step Per Piece** — how much the physical grade may change on each new
  piece;
- **Piece plan** — the track shapes and number of connected pieces available.

An approximate piece count is:

```text
ceiling(abs(Target Grade - Current Grade) / Step Per Piece)
```

Example: moving from `-0.40%` to `+0.80%` at `0.20%` per piece requires six
new pieces:

```text
-0.20%, 0.00%, +0.20%, +0.40%, +0.60%, +0.80%
```

The step controls grade change **per piece**, not per metre. Using different
piece lengths produces different rates of vertical change along the route.

### Start Grade Helper

1. Select the last correct regular track or road piece immediately before the
   new transition.
2. Prepare the track/road shape that will be placed next. To continue with the
   selected piece's shape and orientation, press **Ctrl+P** to pick that object
   for placement. For a different shape, choose it in F1 Objects and then
   reselect the transition's starting piece.
3. Verify the starting piece is in TrackDB/RoadDB and that the open end is the intended
   continuation point.
4. In Track Properties, select the grade **Units** you want to use.
5. Select **Grade Helper...**.
6. Review **Current Grade**.
7. Enter **Target Grade**.
8. Enter a positive **Step Per Piece**.
9. Review **Next Grade**.
10. Select **Start Grade Assist**.

Starting Grade Helper switches the editor to Place mode. Opening it also clears
any manually active Lock Grade so the two placement rules cannot compete.

### Place the transition pieces

1. Put the pointer at the open end of the selected starting piece.
2. Place one connected piece.
3. Confirm normal placement feedback.
4. Review the helper:
   - **Current Grade** becomes the grade just applied;
   - **Next Grade** advances by no more than Step Per Piece.
5. Continue from the open end of the most recently accepted piece.
6. Repeat one piece at a time until the target is reached.

For every placement, GenX verifies:

- the previously selected object was a valid regular track/road piece;
- its grade matches Grade Helper's expected Current Grade;
- the new piece is connected to that piece's expected database endpoint.

If either the connection or grade check fails, GenX rejects the new placement,
restores the previous placement state through its normal rejection path, and
plays stopped/error feedback. Return to the last accepted piece before
continuing.

### Reaching the target

If the remaining difference is smaller than Step Per Piece, GenX uses only the
remaining difference. It never intentionally overshoots the target.

At the target:

- the helper reports **Grade Achieved - Holding Target**;
- Current Grade and Next Grade both equal the target;
- Grade Assist stops stepping;
- normal Lock Grade turns on at the target value;
- the accepted placement receives normal feedback followed by the completion
  chirp.

You may then continue placing track at the constant target grade. Turn Lock
Grade off when that run is complete.

If Target Grade already equals Current Grade when Grade Assist starts, the
helper immediately enters the target-holding state and enables Lock Grade.

### Stop or restart Grade Helper

- While active, select **Grade Assist Active - Click to Stop** to cancel the
  stepping workflow.
- While holding the target, select **Grade Achieved - Holding Target** to clear
  the held workflow and its Lock Grade.
- Closing Grade Helper before the target stops Grade Assist.
- Closing Grade Helper after the target is reached leaves the automatically
  captured Lock Grade active until you turn it off.
- Changing away from Place mode stops an active transition.
- Opening Grade Helper again from a selected track piece resets Current Grade
  to that piece's physical grade.

If an accepted piece is manually changed or undone, stop Grade Helper and
restart it from the last known-correct connected piece. Ordinary Undo does not
rewind the helper's transient Current/Next Grade state reliably enough to
continue blindly.

The Grade Helper **Pin** remembers only the window position. It does not make
the helper open automatically in a later editor session.

## Part 5 — Read the Grade Symbols

Open **View > Grade Symbols** to show direction-aware square markers above
graded static track, roads, and Dynamic Track.

Grade Symbols are enabled by default, but the View menu is the authoritative
toggle.

| Color | Meaning | What to do |
| --- | --- | --- |
| Orange | Steady grade | Confirm it matches the intended constant-grade run. |
| Cyan | Transition: grade magnitude differs from a connected neighbor | Expected through a planned transition; inspect spacing and smoothness. |
| Red | Direct crest or gully warning | Inspect immediately and build a suitable transition or level separator. |

### Orange — steady grade

Orange means the piece is graded and no connected neighbor differs enough to
classify the piece as a transition or warning. It does not mean the grade is
appropriate for the route—only that it is locally steady.

### Cyan — transition

Cyan means the absolute grade magnitude differs from a connected neighbor by
more than the overlay tolerance. It deliberately does not claim the grade is
“increasing” or “decreasing”; stored object direction and TrackDB traversal can
reverse that wording.

A sequence of cyan symbols is normal across a Grade Helper transition.

### Red — crest or gully warning

Red is reserved for two genuinely connected, nontrivially graded pieces whose
physical uphill arrows point toward the common joint or away from it:

```text
crest:   uphill ->  joint  <- uphill
gully:   uphill <-  joint  -> uphill
```

This identifies a direct vertical-direction conflict. Nearby parallel track is
not treated as connected and does not trigger the warning.

### Symbol behavior and visibility

- The symbol arrow follows the physical uphill direction.
- Symbols are centered on the ordered TSECTION path and aligned to the local
  path tangent, including curved pieces.
- Multi-path shapes use their declared main route when available.
- Mirrored artwork keeps the symbol readable from either face.
- Truly level pieces at approximately `0.01%` or less do not display a Grade
  Symbol.
- Symbols are not drawn beyond their useful editor LOD. Move closer if they
  disappear at distance.
- TrackDB edits, grade rotation, and completed tile loads refresh the overlay.

The current transition comparison uses approximately `0.05` percentage points
as its magnitude tolerance. Do not design a railway solely around the marker
threshold; use the symbols as inspection cues.

## Designing a Practical Transition

Grade Helper guarantees an exact sequence of per-piece grade values, but the
operator still designs the vertical alignment.

Consider:

- piece length;
- grade change per piece;
- train speed;
- locomotive and coupler behavior;
- clearance beneath bridges or overhead structures;
- turnout and platform locations;
- terrain cuts, fills, and drainage;
- the route's operating standards.

A `0.20%` step on every 10 m piece changes grade much faster in distance than a
`0.20%` step on every 100 m piece. Use consistent, sensible track lengths
through important transitions.

Grade Helper does not generate a continuous parabolic vertical curve. For
high-speed or visually sensitive locations, use smaller grade steps, suitable
piece lengths, and an Open Rails train test.

Avoid placing a direct positive-grade piece against a negative-grade piece.
Use intermediate grades—and often a level separator—to prevent a red
crest/gully joint.

## Interaction with Dynamic Track

Grade Helper applies to regular track and road placement, not Dynamic Track.

For a Dynamic Track Auto-Flex connection:

- prepare both endpoint elevations before solving;
- Auto-Flex calculates one physical grade plane between those endpoints;
- validate its reported average grade and both external joints;
- do not use Grade Helper to try to step through the internal S-C-S-C-S
  sections.

See the
[Dynamic Track and Auto-Flex User Guide](DYNAMIC-TRACK-AUTO-FLEX-USER-GUIDE.md)
for that workflow.

## Interaction with Terrain

None of the grade tools shape terrain automatically.

Finalize, save, reopen, and simulator-test the track alignment first. Then use
the F2 terrain tools or selected-object conforming:

- **F** — conform terrain to the selected track/object/ruler;
- **Ctrl+F** — conform along the selected TrackDB/RoadDB vector in the current
  tile;
- **Shift+F** — smooth terrain around the selected track/object/ruler;
- **F2 Conform TDB/RDB** — small freehand database-based correction.

See the
[Terrain Improvements User Guide](TERRAIN-IMPROVEMENTS-USER-GUIDE.md) before
major cuts or embankments.

## Save and Reopen Validation

After completing a grade or transition:

1. Turn off Grade Assist or confirm it is holding the intended target.
2. Turn off Lock Grade unless more constant-grade track will be placed.
3. Inspect every piece and database connection.
4. Inspect all Grade Symbols.
5. Save with **Shift+Ctrl+S**.
6. Close the Route Editor.
7. Reopen the route.
8. Return to the transition and confirm:
   - every piece is present;
   - physical grade direction is unchanged;
   - TrackDB/RoadDB remains continuous;
   - orange/cyan/red symbols still match the intended construction;
   - no endpoint has a visible lateral or vertical gap.

Only the placed track/road grades are route content. Grade Helper's active
Current/Target/Next workflow is transient editor state and should not be relied
upon after closing or changing routes.

## Open Rails Validation

1. Create a short Explore Route or test activity covering the transition.
2. Approach slowly in the first direction.
3. Watch the vehicle and couplers at every piece boundary.
4. Listen and feel for a bump, unloading, oscillation, or derailment.
5. Repeat in the opposite direction.
6. Repeat at the intended operating speed.
7. Test representative long-wheelbase locomotives and rolling stock.
8. Preserve `OpenRailsLog.txt` if a failure occurs.

A visually clean GenX overlay cannot prove that a vertical transition is
suitable for every vehicle or speed. Open Rails is the operational judge.

## Essential Shortcuts

| Key | Action |
| --- | --- |
| F1 | Open Objects and Track Properties |
| Q | Toggle Place mode |
| E | Toggle Select mode |
| Ctrl+Q | Toggle AutoTDB |
| Ctrl+P | Pick the selected object's type and rotation for placement |
| Ctrl+Z | Undo the last supported edit |
| Shift+Ctrl+S | Save the route |
| X | Flip the selected world object |
| Z | Toggle selected track object in or out of TrackDB |
| F | Conform terrain to selected track/object/ruler |
| Ctrl+F | Conform terrain along the selected database vector in the current tile |
| Shift+F | Smooth terrain around selected track/object/ruler |

Use `Z` only when you understand the TrackDB/RoadDB consequence. Grade Helper
requires the connected database path to remain valid.

## Troubleshooting

| Problem | Likely cause | Action |
| --- | --- | --- |
| Grade Helper button is unavailable or does nothing | No regular track/road piece is selected | Select the last correct regular piece, then open Grade Helper. |
| Current Grade is not what you expected | Wrong starting piece, stored direction, or direct grade edit | Close the helper, select the correct piece, verify its physical arrow/value, and reopen. |
| Next Grade moves in the wrong direction | Target sign/value is wrong | Stop, correct Target Grade, and restart from the last accepted piece. |
| Step Per Piece cannot be negative | It is intentionally a positive magnitude | Set the target above or below Current Grade; the helper chooses the direction automatically. |
| New piece is rejected | It is disconnected from the expected endpoint or the previous grade changed | Return to the last accepted piece, repair the database/alignment, and restart Grade Helper. |
| A different tool closes Grade Helper | Active Grade Assist is tied to Place mode | Complete or stop the transition before switching tools. |
| Undo removed a piece but helper advanced | Helper state is transient and separate from ordinary object Undo | Stop and reopen Grade Helper from the last correct remaining piece. |
| Target was reached but later pieces keep the same grade | Grade Helper handed the target to Lock Grade | Turn Lock Grade off when the steady run is complete. |
| Lock Grade captures the wrong sign | Stored object direction differs from visual expectation | Inspect the physical uphill arrow; flip or select the correctly oriented starting piece before locking. |
| Grade Ruler value differs from finished track | Ruler uses direct terrain-to-terrain run, while track follows its own path and railhead elevations | Use the ruler for planning and verify final track grades from Track Properties. |
| Grade Ruler endpoint will not move | Select mode or endpoint selection is not active | Press E, select the endpoint handle, drag, and release over loaded terrain. |
| Grade Ruler disappeared | Another special Water/PolyVeg/Grade Ruler was started | Only one special ruler may exist; recreate the Grade Ruler if still needed. |
| No symbols are visible | Grade Symbols is off, pieces are effectively level, or camera is too far away | Enable View > Grade Symbols and move closer to graded track. |
| Cyan appears on every transition piece | Neighboring grade magnitudes differ | This is expected during a stepped transition; inspect whether the steps and piece lengths are appropriate. |
| Red symbol appears | Direct crest/gully conflict between connected graded pieces | Stop construction and insert a suitable grade transition or level separator. |
| Symbol seems to point backward | Object/TSECTION direction is reversed | Treat the arrow as physical uphill direction; verify with the numeric grade and route geometry. |
| Terrain intersects the new grade | Grade tools alter track, not terrain | Finalize the track, then conform terrain using the dedicated terrain procedure. |
| Open Rails train bumps despite clean-looking symbols | Per-piece transition is too abrupt or a joint/database issue remains | Use smaller steps and suitable piece lengths, inspect TrackDB, rebuild the transition, and retest. |

## First-Time Practice Exercise

Use a disposable route copy:

1. Place a short level track with an open end.
2. Select it and confirm `0.00%`.
3. Open Grade Helper.
4. Set Target Grade to `1.00%`.
5. Set Step Per Piece to `0.20%`.
6. Start Grade Assist.
7. Place five connected, equal-length straight pieces.
8. Confirm the sequence reaches `1.00%` without overshoot.
9. Confirm Lock Grade holds `1.00%` on one additional piece.
10. Turn Lock Grade off.
11. Inspect the cyan transition symbols and orange steady-grade symbols.
12. Build a separate deliberate direct crest/gully and confirm the red warning,
    then undo/remove that test construction.
13. Save, reopen, and drive the proper transition in both directions.

## Final Grade Checklist

- [ ] The route was backed up before track/database work.
- [ ] AutoTDB and Place Guard are on.
- [ ] The starting piece is the last correct connected track/road piece.
- [ ] Current Grade and its physical uphill direction were verified.
- [ ] Target Grade has the intended sign and unit.
- [ ] Step Per Piece is appropriate for the chosen piece lengths.
- [ ] Every new piece was accepted as connected to the previous endpoint.
- [ ] The target was reached without overshoot.
- [ ] Lock Grade was turned off when the steady run ended.
- [ ] Cyan transitions are expected and no red warning is unexplained.
- [ ] TrackDB/RoadDB is continuous through the complete alignment.
- [ ] The route saved, closed, and reopened correctly.
- [ ] The transition was driven in both directions in Open Rails.
- [ ] Terrain was conformed only after track geometry was finalized.

When every item passes, the grade transition is ready for normal route use and
surrounding terrain work.
