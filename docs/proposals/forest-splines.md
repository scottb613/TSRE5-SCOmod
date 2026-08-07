# Proposal: Forest Splines and Forest Helper

**Status:** Concept proposal  
**Scope:** Route Editor vegetation-placement workflow and compatible output  
**Primary output:** Ordinary terrain-planted Static world objects

## Summary

TSRE should provide a **Forest Helper** for planting natural, mixed vegetation
along a temporary, editable **Forest Ruler**.

The route builder plots a green multi-point ruler, drags its points to shape the
planting path, sets the width of its planting box, and chooses a JSON-defined
forest type. TSRE previews a deterministic mixture of trees, shrubs, brush,
rocks, or other vegetation inside that box. When the builder chooses **Done**,
every generated object is planted separately on the terrain, the complete
operation becomes one undo step, and the temporary ruler disappears.

The committed route contains ordinary Static objects referencing their normal
shape files. It does not require a new Open Rails route object, a baked
tile-sized shape, or a persistent spline record.

## Problem

The existing MSTS Forest region is useful for broad, inexpensive background
woodland, but it is a poor authoring interface for varied foreground vegetation:

- one region is based on one tree texture;
- the result is visually repetitive;
- the rectangular region does not naturally follow railway boundaries, roads,
  rivers, field edges, embankments, or irregular terrain;
- a mixed woodland requires overlapping regions rather than one coherent
  planting definition;
- individual species proportions and object-specific planting behavior are not
  available;
- the builder cannot shape the forest naturally by dragging a few path points;
- shrubs, dead trees, rocks, and ground cover do not fit naturally into the
  single-texture Forest model.

Route builders need to describe the kind and shape of vegetation they want,
not manually place hundreds of scenery objects or stack multiple single-ACE
Forest regions.

The existing Forest and PolyForest objects should remain available for legacy
routes and broad background woodland. Forest Splines are an additional,
object-based placement method rather than a file-format replacement.

## Guiding principle

```text
The ruler describes where vegetation may grow.
The JSON forest type describes what may grow there.
TSRE plants every resulting object on the terrain.
```

## Core concepts

### Forest Helper

The Forest Helper is a Route Editor popup that controls the current planting
operation. It follows the shared GenX popup style and uses an orange title.

Its initial controls should be deliberately small:

```text
Forest type
Width
Population density
Terrain depth
Planting seed / New Layout
Variation scale
Track and road clearance
Done
Cancel
```

Advanced recipe details belong in JSON rather than crowding the normal
planting workflow.

**Width** and **Population density** are primary operation settings in the
Forest Helper, immediately beside the forest-type selection rather than hidden
as advanced recipe fields.

```text
WIDTH
    Controls the width of the green planting box around the ruler path.

POPULATION DENSITY
    Controls the target number of vegetation objects per unit of usable area.
```

Changing either value regenerates the live preview immediately. Width changes
the available planting area; population density changes how many objects TSRE
attempts to plant within that area. Neither control changes the forest type's
species proportions.

### Forest Ruler

The Forest Ruler is a temporary green construction guide modeled on the Water
Ruler interaction.

The user can:

- click to add control points;
- drag any control point to reshape the ruler;
- insert or delete a point;
- adjust the planting-box width;
- inspect the generated preview while editing;
- commit with **Done** or discard with **Cancel**.

The ruler and its planting boundary remain green so their purpose is distinct
from the blue Water Ruler and ordinary route selections.

The first implementation may use straight segments between control points if
that produces the fastest reliable proof of concept. A smooth curve through the
same draggable points can follow without changing the JSON format or committed
route objects.

### Population Box

The ruler centerline and its width define a **Population Box** in which
candidates may be planted. The box must be drawn directly on the terrain around
the green ruler so the complete affected area is visible before **Done**.

The Population Box display consists of:

- a green terrain-conforming centerline;
- green left and right boundary lines at the selected width;
- green end caps closing the first and last ruler segments;
- a subtle translucent green interior showing the plantable area;
- control-point handles that remain visible above the shaded area.

The box is tessellated along the ruler and across the corridor as needed, with
every display vertex sampled against the terrain beneath it. A small visual
offset above the sampled height prevents z-fighting without changing any
planting elevation. The display must remain continuous over slopes, depressions,
embankments, and world-tile boundaries.

Changing **Width** in Forest Helper immediately moves both terrain-draped
boundaries and regenerates the vegetation preview. Dragging a ruler point
reshapes the centerline, Population Box, and candidate population together.

The translucent fill is an editor preview only. It must not paint terrain,
change terrain samples, create a route object, or survive after **Done** or
**Cancel**.

The first version may use one symmetric width. Later versions may allow:

- separate left and right widths;
- width handles at individual ruler points;
- a soft or feathered edge;
- exclusion gaps along selected portions of the ruler.

Although it is named a box, it is a terrain-draped plan-view planting boundary,
not a shared three-dimensional elevation volume. Every accepted object still
receives its own independently sampled terrain height.

### Multi-tile rulers

A Forest Ruler may cross any number of 2048-metre world-tile boundaries. Tile
edges must not split the visible ruler, reset its random pattern, change its
density, or create a seam in the planted forest.

Control points should retain stable route-wide positions. The generator
evaluates the complete ruler and planting box in continuous route coordinates,
then converts each final candidate into its owning tile and tile-local position.

For a multi-tile operation, TSRE must:

- draw one continuous green ruler and planting box;
- generate one continuous seeded distribution across the complete area;
- avoid duplicate or missing candidates at tile boundaries;
- sample every object against the terrain tile beneath that object;
- place every object into the correct world tile and `.w` file;
- preserve the requested density and forest proportions across the complete
  operation rather than restarting them per tile;
- commit and undo the complete multi-tile planting as one operation.

Required terrain tiles must be loaded and valid before **Done** is enabled. A
missing or unavailable tile should be identified in Forest Helper rather than
silently leaving a rectangular hole or planting at an invented height.

## Primary workflow

```text
1. Open Forest Helper.
2. Choose a JSON-defined forest type.
3. Start a new green Forest Ruler.
4. Click to plot its path.
5. Drag points until the path and planting box cover the intended area.
6. Adjust width, density, depth, clearance, or random layout as needed.
7. Inspect the live preview and object count.
8. Choose Done.
9. TSRE plants ordinary Static objects as one undoable operation.
10. The ruler vanishes and Forest Helper is ready for the next operation.
```

**Cancel** removes the ruler and preview without creating or changing route
objects.

## Forest definitions in JSON

Forest types should be data-driven. The helper discovers valid definitions
from designated application, personal, and route folders and presents their
friendly names in its forest-type list.

A definition can contain any number of vegetation entries. Proportions are
relative weights and do not need to add to exactly 100; TSRE normalizes them.
They describe the identity of the forest and are not changed when the user
raises or lowers its population.

Conceptual example:

```json
{
  "schemaVersion": 1,
  "name": "Mixed Northeastern Woodland",
  "description": "Mixed deciduous woodland with sparse pine and brush.",
  "defaultDensityPerSquareMetre": 0.025,
  "densityRangePerSquareMetre": [0.005, 0.060],
  "edgeFeather": 4.0,
  "defaultTerrainDepth": 0.15,
  "maximumSlopeDegrees": 38.0,
  "preventFootprintOverlap": true,
  "vegetation": [
    {
      "shape": "oak_medium_a.s",
      "proportion": 35,
      "rotationDegrees": [0, 360],
      "uniformScale": [0.85, 1.20],
      "plantingDepth": 0.15,
      "footprintRadius": 1.25
    },
    {
      "shape": "maple_medium_a.s",
      "proportion": 30,
      "rotationDegrees": [0, 360],
      "uniformScale": [0.80, 1.18],
      "plantingDepth": 0.15,
      "footprintRadius": 1.20
    },
    {
      "shape": "pine_medium_a.s",
      "proportion": 15,
      "rotationDegrees": [0, 360],
      "uniformScale": [0.85, 1.20],
      "plantingDepth": 0.15,
      "footprintRadius": 1.10
    },
    {
      "shape": "woodland_brush_a.s",
      "proportion": 20,
      "rotationDegrees": [0, 360],
      "uniformScale": [0.70, 1.30],
      "plantingDepth": 0.25,
      "footprintRadius": 0.65
    }
  ]
}
```

This structure permits any number of forest definitions and predefined
vegetation objects. It is illustrative rather than a frozen schema; the proof
of concept should validate the exact names and units before implementation
makes version 1 permanent.

### Dynamic population density

Population density is dynamic and belongs to the current Forest Helper
operation, not to the vegetation proportions. JSON may supply an initial
recommendation and safe range, but the value shown in Forest Helper is the
authoritative density for the current planting operation.

The target population is recalculated whenever the user:

- drags, adds, or deletes a ruler point;
- changes the planting-box width;
- changes population density;
- changes track, road, slope, or edge-clearance settings;
- selects a different forest definition or planting seed.

Conceptually:

```text
usable planting area x selected density = target population
```

The accepted population may be lower when terrain, clearance, slope, or
non-overlap rules reject candidates. Forest Helper must display both the target
and accepted counts in the live preview.

Changing density changes the number and spacing of objects, but it does not
change the JSON proportions. A definition containing 35 parts oak, 30 parts
maple, 15 parts pine, and 20 parts brush retains that mixture at every density,
subject to small rounding and placement-rejection differences. The seeded
weighted selector should keep the accepted result as close to those target
proportions as the available planting area permits.

### Random scale variation and file compatibility

The planting seed controls a deterministic random uniform scale within each
entry's `uniformScale` limits. The Forest Helper's variation-scale control
narrows or widens those JSON-defined ranges without permitting values outside
the definition's safe constraints.

Weighted shape variants may be mixed with scale variation when genuinely
different models are available:

```text
oak_small_a.s
oak_medium_a.s
oak_large_a.s
```

The desired behavior includes numeric per-instance scale, but it has an
explicit compatibility gate. TSREvc currently reads the MSTS `Matrix3x3`
world-object representation, while its ordinary Static save path currently
writes `QDirection` and does not preserve an independent scale. Implementation
must establish a documented Static representation, render it correctly in
TSRE, preserve it through save/reload, and verify it in Open Rails.

A non-normalized quaternion or other undocumented scale trick is not an
acceptable compatibility foundation. If the compatibility gate is not met,
shape variants remain the safe fallback rather than silently losing the user's
random scale choices.

## Distribution

The generator must not use a square grid, staggered grid, rows, cells with one
candidate each, or a jittered lattice. Forest placement must not reveal an
underlying repeating pattern from the ground or from an elevated viewpoint.

Avoiding a grid is necessary but not sufficient. A perfectly even random mix
still looks like computer scatter rather than a forest. The selected JSON
definition must control the visible character of the planted habitat.

Candidate positions should use deterministic blue-noise/Poisson-disc sampling
inside a low-frequency seeded density field. The density field creates broad
natural patches, thinner areas, and small clearings without exposing cells or a
repeating pattern. Species selection may use overlapping cluster fields so a
stand contains recognizable groups of oak, maple, pine, brush, or other recipe
members instead of distributing every species as a uniform soup.

The generator should support vegetation strata within one definition. The
operation's dynamic population density scales their populations without
changing their species proportions:

```text
CANOPY       Dominant trees with the largest footprints and widest spacing
UNDERSTORY   Smaller trees and tall shrubs placed around canopy openings
GROUND       Brush, ferns, rocks, logs, and low vegetation
EDGE         Species favored near the planting-box boundary
```

Each stratum uses its own density, footprint rules, and species proportions.
Ground and understory candidates may coexist near a canopy object when their
solid footprints do not intersect. This permits layered woodland without
placing two trunks or solid models through one another.

JSON definitions may also describe:

- cluster strength and approximate cluster size;
- clearing frequency and approximate clearing size;
- edge preference or avoidance per entry;
- density variation across the habitat;
- local association, such as brush favored near large trees;
- exclusion, such as a species avoided on steep slopes;
- size/age distribution through constrained scale and shape variants.

These are authoring controls for the forest definition. The normal Forest
Helper continues to present a simple choice of habitat plus a few operation
controls.

For every accepted candidate, TSRE selects a vegetation entry using the
definition's normalized proportions. Entry-specific spacing prevents large
trees from being packed at shrub spacing.

Accepted objects must not occupy the same planting footprint. Each vegetation
entry supplies a base `footprintRadius`, which is multiplied by that object's
random scale. A candidate is rejected when its footprint intersects an already
accepted footprint. The footprint represents the trunk, stem cluster, rock
base, or other solid planting area; it should not normally use the complete
tree-canopy bounds because natural canopies may overlap.

Definitions may later provide an oriented rectangular footprint for long logs,
hedges, or asymmetrical ground cover. A spatial hash or uniform search grid may
be used internally so overlap checks compare only nearby accepted candidates
rather than every object in the operation. This grid is an invisible search
index only; it must never generate, align, quantize, or move planting positions.

Required distribution behavior:

```text
NATURAL SCATTER
    Blue-noise positions with broad, non-repeating density variation.

SPECIES PATCHES
    Seeded local groupings while preserving the recipe's overall proportions.

LAYERED HABITAT
    Canopy, understory, ground, and edge populations where defined.

CLEARINGS
    Irregular low-density or empty areas controlled by the habitat recipe.

EDGE FEATHER
    Gradually reduce acceptance near the planting-box boundary.

WEIGHTED MIX
    Select shapes according to their JSON proportions.

NO SOLID OVERLAP
    Reject candidates whose scaled planting footprints intersect.
```

Future schema versions may refine these natural-distribution behaviors, but
they must continue to produce ordinary Static output without visible grid
patterns.

## Deterministic random layout

Every operation has a planting seed. The same ruler geometry, forest
definition, settings, and seed must produce the same candidate positions,
shape choices, rotations, and scales.

Determinism provides:

- a stable preview;
- repeatable testing;
- a **New Layout** action that changes only the seed;
- predictable results while a point or width is not changing;
- easier diagnosis of missing, floating, or crowded objects.

Dragging a control point regenerates the preview for the changed planting box,
but no world objects are created until **Done**.

## Per-object terrain planting

Terrain conformance is mandatory and is performed separately for every object.
The ruler never supplies a shared elevation.

For each accepted candidate:

```text
1. Determine its final route-space X/Z position.
2. Normalize tile and local coordinates across 2048-metre tile boundaries.
3. Sample terrain height at that exact X/Z position.
4. Resolve the entry's planting depth, falling back to the forest default.
5. Set Y to terrain height minus planting depth.
6. Apply the seed-selected yaw and uniform scale within the entry constraints.
7. Confirm the transformed planting footprint does not overlap an accepted one.
8. Create the ordinary Static object.
```

The live preview and final commit must use the same height query and coordinate
normalization so objects do not jump vertically when **Done** is pressed.

Trees and shrubs remain vertically upright by default; only yaw is randomized.
A future `followTerrainSlope` option may be appropriate for rocks, logs, and
ground cover, but it should not tilt standing trees unless explicitly enabled
for that entry.

Candidates are rejected rather than guessed when required terrain is missing or
unloaded. The helper should report how many candidates were rejected and why.

## Clearances and exclusions

The generator should be capable of rejecting candidates that violate:

- the forest definition's minimum spacing;
- scaled object-footprint overlap;
- track clearance;
- road clearance;
- maximum terrain slope;
- unavailable terrain;
- the planting-box boundary;
- an optional existing-object clearance.

Track and road clearance should use the loaded TrackDB and RoadDB geometry, not
visual guesses based on the rendered rail or road shape.

The initial proof of concept may begin with planting-box and vegetation-spacing
checks, then add database and slope exclusions as explicit gates.

## Preview

Preview generation must not insert temporary objects into route world tiles.
It should render lightweight markers, simplified bounds, or ghost shapes from an
in-memory candidate list.

The helper should display at least:

```text
Candidate count
Accepted object count
Rejected count
Estimated breakdown by vegetation shape
```

Useful preview states:

```text
GREEN    Candidate accepted and terrain height resolved
YELLOW   Candidate rejected by spacing, slope, or clearance
RED      Missing shape, invalid definition, or unavailable terrain
```

For very dense splines, the visual preview may use points or simple vertical
stems until the user pauses editing. The committed result must still be based on
the exact candidate list shown by the final preview.

## Commit, selection, and undo

**Done** performs one bounded transaction:

```text
Begin one Undo state
Validate the final candidate list
Place every accepted Static object
End the Undo state
Remove the temporary ruler and preview
Prepare Forest Helper for another operation
```

One Undo action removes the complete planting operation. No partial forest
should remain after undo.

After a successful commit, the vegetation objects are ordinary independent
route objects. The temporary ruler is not saved and there is no persistent
spline-to-object ownership relationship. Builders may select, move, or delete
the committed objects using the normal editor tools.

If a required shape disappears or a fatal placement error occurs during commit,
the operation should roll back rather than leave a partially planted forest.

## Open Rails performance model

The first implementation should rely on normal repeated Static shapes. Open
Rails can instance identical shapes, so hundreds of references to the same tree
shape can be rendered in batches while retaining finer culling than one
tile-sized baked mesh.

A forest definition with ten vegetation shapes should normally create roughly
ten reusable shape/material populations rather than a unique mesh per planted
object.

This does not eliminate every cost. Large object populations still increase:

- world-file size;
- route loading and parsing work;
- CPU-side instance and culling data;
- TSRE selection and world-tile management work.

Crossing world-tile boundaries does not by itself make generation expensive.
The dominant measure is the accepted object count. Multi-tile placement is
beneficial at runtime because each Static belongs to its correct world tile, so
Open Rails can load, cull, and unload vegetation with the surrounding route
area instead of treating the complete forest as one tile-sized mesh.

Forest Helper should keep interactive editing responsive by:

- using lightweight points or bounds while control points are being dragged;
- debouncing regeneration during continuous mouse movement;
- using spatial indexing for blue-noise and overlap checks;
- sampling terrain once per final candidate per preview revision;
- grouping candidate work by owning terrain/world tile;
- avoiding route-object creation until **Done**;
- batching final placement inside one Undo transaction and one scene refresh.

The preview should show estimated and accepted object counts continuously. A
very long or dense ruler should warn before commit and may be split into
several operations by the builder. The safety threshold should be configurable
and based on object count, not an arbitrary single-tile restriction.

The helper should therefore show the estimated object count before commit and
support a configurable safety warning for unusually large operations.

Tile-sized baked vegetation shapes are not part of the initial design. They
should be reconsidered only if controlled Open Rails measurements demonstrate
that world-object overhead is a real limitation after instancing.

## Compatibility

The committed result must remain compatible with the existing route format:

```text
Forest Helper and ruler    Editor-only temporary state
Forest JSON                TSRE authoring data
Committed vegetation       Ordinary Static world records
Shape and texture assets   Existing route/global assets
```

No changes are required to TrackDB, RoadDB, terrain files, or Open Rails.

The generator must preserve TSRE's existing world-file encoding and save
behavior. It must use the normal route placement and undo paths rather than
writing world files directly.

Missing shape files, incompatible object types, and malformed JSON must be
reported before placement. JSON paths should use stable shape filenames rather
than route-local numeric identifiers.

## Forest-definition library

Three definition sources are useful:

### Factory definitions

A small collection distributed with TSRE and limited to assets TSRE may legally
and reliably reference.

### Personal definitions

User-created mixtures available across the user's routes.

### Route definitions

Definitions shipped with one route and allowed to reference that route's own
vegetation assets.

When names collide, the helper should clearly indicate the source and apply a
documented precedence order. A route definition should not silently replace a
factory or personal definition with different content.

## Validation

Before **Done** becomes available, verify:

```text
The JSON schema version is supported
The forest contains at least one positive-weight entry
Every referenced shape can be resolved
The ruler has enough valid control points
The planting box has positive width and area
Density and spacing values are within safe bounds
All final accepted candidates have terrain heights
The estimated object count is within the approved operation limit
```

After commit and save/reload, verify:

```text
Every object retains its shape filename
Every object retains its X/Z position and yaw
Every object retains its seed-selected scale
Every object remains planted at its sampled terrain height and depth
Multi-tile rulers produce no duplicate, missing, or visibly aligned boundary objects
No unintended terrain, TrackDB, or RoadDB file changes occur
Undo removes the complete planting before save
Open Rails loads and renders the repeated shapes correctly
```

## Suggested progression

### Phase 1: JSON loader and candidate generator

Load one forest definition, normalize proportions, generate deterministic
minimum-distance candidates, and report the planned object breakdown without
changing the route.

### Phase 2: Green Forest Ruler and planting box

Reuse the proven Water Ruler interaction concepts for point creation, dragging,
editing, and multi-tile coordinates. Render a green centerline, handles, and
planting boundary.

### Phase 3: Terrain-planted preview

Resolve each candidate's terrain height and planting depth and show lightweight
preview markers. Regenerate the preview after ruler or helper changes.

### Phase 4: Static-object commit and single undo

Place ordinary Static objects through the normal route API, commit the operation
as one Undo state, and remove the ruler after **Done**.

### Phase 5: Clearance and natural-distribution controls

Add TrackDB/RoadDB clearance, slope rejection, edge feathering, shape-specific
spacing, clustering, and count safeguards.

### Phase 6: Forest-definition library and authoring support

Add factory, personal, and route definition discovery, schema diagnostics, and
a friendly way to duplicate or edit definitions without hand-editing being the
only workflow.

### Phase 7: Controlled performance evaluation

Compare ordinary instanced Static populations with legacy Forest regions on a
representative Open Rails route. Measure route load time, memory, frame time,
culling behavior, world-file growth, and TSRE editing responsiveness before
proposing any baked-geometry optimization.

## Recommended first proof of concept

Use a short, uneven section of the SCO_LHR test route with terrain crossing at
least one world-tile boundary.

```text
1. Define one mixed forest with three tree shapes and one brush shape.
2. Plot a three- or four-point green ruler.
3. Set a modest planting width and density.
4. Drag the middle points and confirm the preview follows immediately.
5. Cross a world-tile boundary and confirm there is no distribution seam.
6. Confirm every preview candidate has its own terrain height and owning tile.
7. Press Done and confirm the ruler disappears.
8. Undo and confirm every planted object across every tile is removed together.
9. Repeat, retain the result, save, close, and reload.
10. Compare the route evidence manifest for unintended changes.
11. Run the route in Open Rails and inspect appearance and performance.
```

Success means the route builder can create a natural mixed woodland in one
clear operation without using overlapping single-texture Forest regions and
without sacrificing standard route compatibility.

## Final direction

Forest Splines should be a placement workflow, not a new simulator object.

The green ruler exists only while the builder is shaping the operation. JSON
forest definitions provide reusable mixtures and planting rules. **Done** turns
the preview into ordinary, individually terrain-planted Static objects, then
removes the ruler and prepares the helper for the next forest.
