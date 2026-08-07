# Proposal: Track Assemblies and Pattern Library

**Status:** Concept proposal  
**Scope:** User experience and product behavior concepts only  
**Example route:** SCO_LHR track-development route

## Summary

TSRE should allow route builders to create, save, preview, place, mirror, and share complete connected track arrangements as reusable **track assemblies**.

A track assembly would be a tested collection of ordinary track pieces that remains logically attached while it is being positioned and edited. Examples include double-track branches, crossovers, scissors crossovers, yard ladders, passing loops, station throats, and turntable approach fans.

The route builder should choose the railway arrangement they want rather than remember the exact XTracks filenames, rotations, small adapter pieces, dynamic-track settings, and placement order needed to construct it.

The difficult work should be done once in a test route. A successful arrangement can then be captured, named, saved in a library, and recalled in any route.

## Problem

Complex track formations are difficult to build even when all required pieces are installed. The builder must remember or rediscover:

- which turnout and crossing families have compatible angles;
- which short straight pieces make the geometry close correctly;
- the correct order and orientation of every piece;
- how to preserve standard double-track spacing;
- where dynamic or FlexTrack closures are required;
- which switch orientation gives the desired main and diverging routes;
- how to finish every side with clean, reusable track endpoints;
- how to avoid invalid junction, crossover, or TrackDB topology.

The SCO_LHR right-hand double-track branch demonstrates the problem. It has two switch nodes, a diamond crossing, gentle closure geometry, a clean incoming double-track connection, and clean main and branch double-track exits. It works, but it took substantial effort to establish.

Requiring every user to reproduce that work is unnecessary.

## Proposed concept

Introduce two complementary track-building concepts:

```text
TRACK PIECE
    One ordinary shape selected from tsection.dat.

TRACK ASSEMBLY
    A tested, connected arrangement of ordinary track pieces that can be
    manipulated and placed as one logical unit.
```

An assembly is an editor convenience. Its members remain normal route objects and normal TrackDB content. It does not require a new Open Rails track format.

## Primary user workflow

### Build and capture

```text
1. Build and test an arrangement in a track-development route.
2. Select all pieces belonging to the arrangement.
3. Choose "Create Assembly from Selection."
4. Mark or confirm its external connection ports.
5. Choose the primary placement anchor.
6. Give the assembly a name, category, and description.
7. Save it to the Track Assembly Library.
```

### Recall and place

```text
1. Select an existing track endpoint.
2. Open the Track Assembly Library.
3. Choose an assembly from a visual catalog.
4. Select left/right, forward/reverse, and default-route options.
5. Match the incoming elevation and grade.
6. Inspect the complete ghost preview.
7. Place the assembly with one action.
8. Keep it grouped or break it into ordinary pieces.
```

## Assembly behavior

While an assembly remains grouped, it should behave like one editing object:

```text
Click any member       Select the complete assembly
Move                   Move the complete assembly
Rotate                 Rotate the complete assembly
Raise or lower         Raise or lower the complete assembly
Change grade           Tilt the complete assembly as a rigid unit
Delete                 Delete the complete assembly
Undo                   Restore the complete assembly
```

The grouping must not change the simulation meaning of its individual track objects.

### Breaking an assembly apart

The builder must always be able to override the grouping:

```text
Keep Grouped
Unlock One Piece
Remove Piece from Assembly
Force Breakup / Explode Assembly
```

Exploding an assembly removes only the editor relationship. It must not move any track, alter the established geometry, or disconnect a valid TrackDB.

## Ports and anchors

Assemblies should connect to routes through named **ports**, not through absolute coordinates.

For a double-track branch:

```text
PORT A  Incoming main      Two tracks
PORT B  Outgoing main      Two tracks
PORT C  Outgoing branch    Two tracks
```

Each port conceptually records:

- position relative to the assembly anchor;
- heading;
- elevation and grade;
- number of tracks;
- track spacing;
- connection order;
- minimum clear lead, when applicable.

The user normally selects an existing endpoint and chooses which assembly port to attach there. TSRE aligns the complete assembly from that port.

This removes the need to align four, six, or more individual rails manually.

## Ghost preview

No TrackDB or world changes should be committed until the complete assembly has been previewed.

Suggested preview states:

```text
GREEN    All connections valid and all required shapes available
YELLOW   FlexTrack or a small closure adjustment is required
ORANGE   Grade, radius, spacing, clearance, or operating warning
RED      Missing shape, invalid connection, overlap, or broken topology
```

The preview should show the full occupied footprint and every external port.

## Mirror, reverse, and orientation

### Flip X

Flip X must be a railway-aware mirror operation, not a graphical negative scale.

It should map:

```text
Right turnout       <-> Left turnout
Right-hand branch   <-> Left-hand branch
Right curve         <-> Left curve
Port ordering       <-> Mirrored port ordering
```

Switch defaults and crossover relationships must remain logically correct after the mirror.

If a rigid piece has no compatible mirrored counterpart, TSRE should report that before placement and offer a compatible substitution only when one is known.

### Reverse

Reverse placement should reuse the same assembly definition with its anchor and port direction reversed. It should not require a separately authored template.

## Elevation and grade

An assembly should support two simple grade behaviors:

```text
SET ASSEMBLY GRADE
    Tilt the complete assembly as one rigid plane.

MATCH SELECTED PORT
    Inherit the position, heading, elevation, and grade of the existing
    track endpoint used as the placement anchor.
```

A rigid assembly should not bend internally merely because its overall grade changes. Vertical transitions belong in approach or exit tracks unless an assembly explicitly contains designated flexible regions.

## Building assemblies in a test route

A dedicated test route is a practical assembly-authoring environment. Builders can experiment, inspect TrackDB behavior, drive test paths, and keep known-good examples together.

Example catalog:

```text
SCO_TRACK_TEST
    Double-track branch - right - gentle
    Double-track branch - left - compact
    Single crossover - 5 degree
    Double crossover - 10 degree
    Scissors crossover
    Yard ladder - six track
    Station throat - four platform
```

After testing, the builder selects the arrangement and saves it to a library available from other routes.

## Library levels

Three complementary library sources are proposed:

### Factory library

A small set of curated and tested assemblies distributed with TSRE.

### Personal library

Assemblies captured by the user from their own test or production routes.

### Community library

Importable assembly packs created by route builders. Entries should identify their author, dependencies, intended track system, and compatibility requirements.

## Visual catalog

Users should not need to know filenames such as `A1tPnt10d150rRgtMnl.s` to choose a useful arrangement.

The assembly catalog should use friendly categories and diagrams:

```text
JUNCTIONS
    Single-track branch left/right
    Double-track branch left/right
    Single-to-double transition
    Double-to-single transition
    Double-track merge
    Wye

CROSSOVERS
    Single crossover left/right
    Facing crossover
    Trailing crossover
    Double crossover
    Scissors crossover
    Diamond with approaches
    Single slip
    Double slip

YARD AND TERMINAL
    Straight ladder
    Curved ladder
    Compound ladder
    Parallel ladder
    Opposing ladder
    Double-ended yard
    Classification-yard throat
    Station throat
    Engine-terminal fan
    Turntable approach fan

COMMON LAYOUTS
    Passing loop
    Runaround
    Platform loop
    Pocket track
    Siding with headshunt
```

## Friendly configuration choices

The user should make railway-level choices rather than shape-level choices:

```text
Direction       Left / Right
Track count     1 / 2 / 3 / 4
Spacing         Standard / Custom
Geometry        Gentle / Standard / Compact / Industrial
Default route   Main / Diverging
Lead length     Compact / Normal / Long
```

The catalog may show the actual turnout angle, minimum radius, footprint, and required packages as supporting information.

## Turnout and crossing families

The installed XTracks definitions include several possible matched turnout and diamond families. The filename suffix `10d` means ten **degrees**, not a railroad No. 10 frog.

Conceptual assembly families could include:

```text
ANGLE   APPROXIMATE FROG   CHARACTER             TYPICAL PURPOSE
-----   ----------------   --------------------  --------------------------
 2.5    No. 22.9           Very long and gentle  High-speed mainline
 5.0    No. 11.4           Long and gentle       Mainline junction
 6.0    No.  9.5           Balanced              General junction
 6.3    No.  9.1           Balanced/compact      Junction or station throat
 7.5    No.  7.6           Moderately compact    Branch or station throat
10.0    No.  5.7           Compact               Branch or yard
15.0    No.  3.7           Extremely tight       Industrial, tram, or yard
```

The factory library should not attempt to provide every possible combination. A practical initial selection might be:

```text
5 degree     Gentle mainline
6 degree     General purpose
10 degree    Compact
15 degree    Tight-yard specialty
```

Additional geometry can be supplied through personal or community assemblies.

## Avoiding the configuration explosion

Prebuilding every complete variation would be unmanageable. The combinations multiply across:

```text
Angle
x turnout radius
x left/right
x track spacing
x automatic/manual
x main/diverging default
x track profile
x entrance length
x exit length
x grade
```

Assemblies should therefore separate the fixed interlocking **core** from adaptable approaches:

```text
COMPLETE JUNCTION
|-- Fixed core
|   |-- Turnouts
|   |-- Diamond or crossing
|   `-- Essential matching adapter pieces
|
`-- Adaptable approaches
    |-- Main entrance closures
    |-- Main exit closures
    `-- Branch closures
```

Only the difficult core needs to be designed and tested. Approach and exit lengths can use ordinary straights, standard adapter assemblies, or automatic FlexTrack closures.

This turns many nominal variants into transformations of a few proven cores:

```text
One tested five-degree core
+ automatic port closures
+ mirror
+ reverse
+ rigid grade transformation
+ optional lead extensions
```

## Factory assemblies versus generation

Prebuilt assemblies are appropriate where the internal geometry is fixed:

- crossovers;
- double-track branches;
- scissors crossovers;
- single/double transitions;
- passing loops;
- common station throats;
- common turntable approaches.

Generation is more appropriate where one dimension genuinely repeats or varies:

- number of yard tracks;
- yard-track spacing;
- lead length;
- platform count;
- repeated turnout cells;
- minimum tail length.

The two approaches can be combined. A generator can assemble tested components rather than invent an entire formation from scratch.

## Yard ladders as repeated cells

A separate template should not be required for every possible yard size.

```text
YARD LADDER
|-- Tested ladder entrance
|-- Repeated turnout cell x N
`-- Parallel track closures
```

The user selects:

```text
Side
Number of yard tracks
Track spacing
Turnout family
Ladder style
Minimum tail length
Lead length
Grade
```

TSRE previews the resulting ladder, identifies required pieces and closures, and presents any limitations before placement.

## Inventory awareness

An assembly should declare its required track families and shapes. Before placement, TSRE should compare those requirements with the loaded global and route `tsection.dat` definitions and the available shape files.

Conceptual status display:

```text
DOUBLE-TRACK BRANCH - RIGHT

Required:
    Two compatible right-hand turnouts    Available
    One matching diamond                  Available
    Four matching short straights         Available
    Two dynamic closures                  Supported

Status: READY
```

If a required shape is unavailable, TSRE should identify the missing item and offer only known-safe alternatives.

Assembly dependencies should be identified primarily by shape filename and geometry signature rather than assuming that a numeric shape ID is universally correct.

## Conceptual saved information

An assembly definition should preserve reusable information while regenerating route-specific identity.

```text
SAVE IN ASSEMBLY                  REGENERATE WHEN PLACED
-------------------------------   -------------------------------
Shape filenames                   World-object UIDs
Relative positions                TrackDB node IDs
Relative rotations                Track-item IDs
Dynamic-track geometry            Route-local tsection IDs
Internal connectivity             Crossover-node IDs
Named external ports
Switch defaults
Dependency information
Author and descriptive metadata
```

## Validation

Before committing an assembly, TSRE should verify:

```text
Every internal endpoint is connected exactly once
Every external port contains the expected number of tracks
Paired-track spacing is within tolerance
Paired-track headings are parallel where required
Elevations and grades are continuous
Minimum curve-radius rules are satisfied
No duplicate track vectors exist
No unintended physical overlap exists
Junction nodes have valid pins
Crossings contain the required crossover-node pair
Required shapes are available
No illegal junction-to-junction connection exists
```

A saved assembly should also be capable of carrying a known-good validation status from its authoring route, while still being revalidated in its destination route.

## Parts list and guided construction

Even when a user does not place an assembly automatically, its library entry can provide an illustrated recipe:

- topology diagram;
- friendly description;
- required parts list;
- compatible alternatives;
- placement order;
- expected endpoints;
- default switch positions;
- warnings and operating notes.

This provides value before full automatic placement exists.

## Suggested progression

### Phase 1: Illustrated recipe catalog

Provide diagrams, parts lists, compatible families, and placement guidance.

### Phase 2: User-created reusable assemblies

Allow a connected selection to be captured, named, saved, recalled, and placed as a rigid group.

### Phase 3: Assembly transformations

Add mirror, reverse, anchor selection, grade matching, ghost preview, breakup, and single-operation undo.

### Phase 4: Small curated factory library

Ship a limited number of proven junction, crossover, transition, and ladder assemblies.

### Phase 5: Core plus automatic closures

Connect tested cores to existing route endpoints using standard adapters and FlexTrack.

### Phase 6: Repeating pattern generators

Generate yard ladders, station throats, and similar repeated arrangements from tested cells.

### Phase 7: Advanced solution search

Optionally allow TSRE to search installed track families and rank compatible solutions by radius, footprint, length, and piece count.

## Recommended first proof of concept

Use the completed SCO_LHR right-hand double-track branch as the first golden-master assembly.

Success criteria:

```text
1. Select the complete installed junction.
2. Detect and confirm its three double-track ports.
3. Save it as "Double-track Branch - Gentle Right."
4. Delete or move away from the original test location.
5. Recall the assembly at another double-track endpoint.
6. Preview and place it as one operation.
7. Confirm identical geometry and valid TrackDB topology.
8. Mirror it into a valid left-hand version.
9. Break the placed copy into ordinary editable pieces without changing it.
```

If this works, the same foundation supports crossovers, transitions, station throats, and yard-ladder cells.

## Guiding principle

```text
The route builder should choose the railway arrangement and its purpose.
TSRE should remember the pieces, geometry, and construction recipe.
```
