# Proposal: Wire Bake

**Status:** Concept proposal  
**Scope:** Route Editor generation of repeated spans between placed supports  
**Primary output:** MSTS/Open Rails-compatible generated shape objects

## Summary

TSRE should automatically generate wire, cable, fence, pipe, rail, chain, and
similar span geometry between repeated route-side objects.

Source models identify their connection locations through named attachment
points in the exported shape hierarchy. After the route builder places and
selects an ordered run of compatible poles, posts, towers, or other supports,
TSRE matches corresponding attachment names and generates the connecting
geometry.

This extends the existing auto-placement and auto-gantry workflow without first
requiring a complete general-purpose loft-object editor.

## Problem

Repeated supports are relatively easy to place, but connecting them convincingly
is laborious. Typical examples include:

- telephone and utility wires;
- catenary and support cables;
- fence boards, rails, and wire;
- pipes between supports;
- chains and suspended barriers;
- wall, hedge, or roadside strips;
- other repeated linear route details.

The supports may vary in spacing, elevation, heading, and terrain position.
Prebuilt fixed-length spans cannot follow every run, while manually modeling a
custom shape for each arrangement is slow and discourages later editing.

TSRE already knows where the placed supports are. If their models also identify
the exact connection points, TSRE has enough information to construct the
intervening geometry.

## Guiding principle

```text
The support shape defines where a span attaches.
The selected route objects define the run.
TSRE generates the connection between them.
```

## Proposed concept

Extend TSRE's existing auto-placement and auto-gantry tools with an
**Automatic Span Generator**.

The first implementation operates on an explicit user selection or group. It
does not scan the complete route attempting to infer unrelated runs.

Each compatible source shape contains named helper nodes or subobjects at its
attachment positions. Conceptual names include:

```text
wire_01
wire_02
wire_03
wire_04

board_01
board_02
pipe_01
rail_01
```

The helpers are positioned in Blender or another modeling tool at the exact
local coordinates where spans should begin and end. They may be visible authoring
markers or invisible exported nodes, provided the MSTS/Open Rails shape retains
their names and transforms.

TSRE transforms each attachment into route-world coordinates, matches it to the
same attachment name on the next support, and generates the required span.

## Support-bound span profiles

Wire Bake is not limited to electrical or telegraph wire. Each supported route
object type has its own defined **span profile** describing what should be
generated between matching instances of that object.

Conceptual examples:

```text
TELEPHONE POLE
    Twelve flexible wires with individual named attachment points

THREE-WIRE FENCE POST
    Three tensioned fence wires with little or no sag

BOARD FENCE POST
    Two or three rigid wooden rails

MESH FENCE POST
    One repeated or stretched fence panel plus an optional top wire

PIPE SUPPORT
    One or more rigid tubes

CHAIN POST
    One sagging chain between posts

CATENARY SUPPORT
    Messenger, contact, feeder, and auxiliary wire families
```

The source shape continues to own the exact attachment locations and names. The
support-bound profile supplies the connection behavior:

```text
Support shape or compatible object family
Attachment-name family
Span geometry type
Material and texture
Width, diameter, or cross-section
Default and permitted sag
Curve quality limits
Maximum span length
Distance levels
Compatible neighboring support types
```

When the user selects a run, TSRE detects the support type and automatically
loads its default span profile. The helper should not assume that every repeated
object requires a generic wire.

One support type may expose more than one named family. For example, a catenary
mast could offer contact wires, feeder wires, and a return conductor, while a
fence post could offer rigid boards and a flexible top wire. The helper may let
the builder enable or disable those predefined families, but it must retain the
profile's compatibility and safety constraints.

Profiles may be distributed with TSRE, supplied by a route, or supplied beside
an asset pack. Their final storage format requires a focused schema decision.
Embedding attachment transforms in the shape remains important: a profile
identifies and configures attachment names but does not duplicate their local
coordinates.

## Why attachment points belong in the shape

Keeping the attachment data in the support model provides several advantages:

- the model author controls the exact connection locations;
- attachment points move and rotate with the placed support;
- the route builder does not maintain a separate offset file;
- compatible models can expose consistent naming conventions;
- the same support remains usable in different routes and runs;
- editing the source model and re-exporting it keeps geometry and attachments
  together.

This approach depends on the exporter and shape loader preserving named helper
nodes and their local transforms. That behavior must be verified before it is
adopted as the permanent attachment contract.

## Primary workflow

```text
1. Model a pole, post, fence support, tower, or similar object.
2. Add consistently named attachment points at the required locations.
3. Export the model to MSTS/Open Rails shape format with those names retained.
4. Place a sequence of supports with TSRE's normal or automatic placement tools.
5. Select or group the supports that form one run.
6. Open Automatic Span Generator and choose the span settings.
7. Confirm the detected run order and attachment matches.
8. Inspect the generated preview.
9. Generate the compatible span shapes and world objects.
10. Save and test the result in Open Rails.
```

The operation should be committed as one Undo state. Canceling from the preview
must leave the route unchanged.

## Run definition and ordering

The initial implementation should use an explicit user selection or group to
define the run. Generation ends where that selection ends.

TSRE must then determine a stable support order. Possible initial rules include:

- preserve the order of an automatic-placement sequence when available;
- allow the user to nominate the first support and direction;
- otherwise derive the shortest unambiguous chain through the selected objects;
- show the proposed order before generation;
- reject ambiguous branches rather than connecting them arbitrarily.

Automatic whole-route discovery should remain out of scope until explicit runs
are proven safe and predictable.

Future versions may support named start, end, junction, and branch markers.

## Attachment matching

Two consecutive supports may be connected only when their attachment points are
compatible.

Their support-bound span profiles must also identify a compatible connection
family. Matching node text alone is insufficient if two unrelated models happen
to use the same helper name.

Required initial rules:

```text
Matching names          wire_01 connects only to wire_01
Compatible profile      Both supports permit the same span family
Compatible support set  Selected shapes belong to the permitted template/run
Maximum distance        Span length does not exceed its configured limit
Valid transform         Both attachment nodes have usable world coordinates
Valid direction         The ordered run does not reverse unexpectedly
```

If one attachment is missing, TSRE should identify the support, shape, and name
instead of silently generating a partial or crossed span.

When multiple names match, TSRE generates one corresponding span for each
matched pair. Four named wire attachments therefore create four parallel wires
between each consecutive pair of poles.

## Span settings

Generated span types should support:

```text
Span type               Wire, cable, board, pipe, rail, chain, or strip
Width/diameter          Cross-section width or thickness
Sag                     Vertical drop between attachments
Curve segments          Tessellation of a flexible span
Material/texture        Compatible shape material definition
Maximum span length     Safety and quality limit
Attachment prefix       wire_, board_, pipe_, rail_, and similar families
Output grouping         Per span, section, or tile
```

Settings may be supplied by a reusable span preset, with the helper exposing the
values most useful during placement. The selected support type supplies the
default profile and allowable ranges; the route builder adjusts only values that
the profile marks as editable.

### Flexible spans

Telephone and utility wires require sag. A first proof of concept may use a
simple symmetric curve controlled by midpoint drop.

The preferred production curve is a catenary or a close parabolic
approximation. It should:

- begin and end exactly at the transformed attachment points;
- handle supports at different elevations;
- preserve visual continuity at tile boundaries;
- use enough segments for a smooth result without excessive geometry;
- reject impossible or excessive sag values.

### Rigid spans

Fence boards, rails, pipes, and similar rigid connections use zero sag. TSRE
generates a strip, tube, board, or configured cross-section between the matched
points.

Sharp changes in the run may require a joint, miter, corner support, or a break.
The first implementation should warn or stop rather than invent an unsafe
corner treatment.

## Preview and validation

No generated shape or world object should be committed until the complete run
has been previewed.

TSRE can draw the complete wire preview directly from the confirmed attachment
points and current span parameters. Preview generation is in memory only; it
does not create `.s`, `.sd`, texture, or world files.

For each matched attachment pair, the shared curve generator receives:

```text
World-space start attachment
World-space end attachment
Span type
Sag
Width or diameter
Curve-segment count or quality setting
Material/preset identity
```

A simple initial wire curve may interpolate between unequal endpoint elevations
and apply a symmetric downward midpoint displacement. The production generator
may replace this with a catenary or validated parabolic approximation without
changing the preview workflow.

The same calculated curve points must feed both:

```text
PREVIEW    Temporary OpenGL line, ribbon, or low-sided tube geometry
BAKE       Final MSTS/Open Rails shape geometry
```

Using one curve calculation prevents the baked wire from changing position,
sag, or tile-boundary intersections after the user accepts the preview.

The preview updates immediately when the builder changes sag, thickness,
quality, maximum length, run order, or selected supports. Thin wires may use a
high-visibility editor line in addition to their approximate physical width so
they remain visible while editing.

Suggested preview states:

```text
GREEN    Attachment matched and span valid
YELLOW   Long span, sharp direction change, or quality warning
RED      Missing attachment, incompatible support, invalid order, or hard limit
```

The preview should show:

- ordered support numbers;
- detected attachment names;
- attachment-point markers on every support;
- every proposed span curve;
- tile-boundary split locations;
- warnings at the affected support or span;
- generated shape and object counts.

Generation should be blocked while any required span remains invalid.

## Safety and termination rules

The generator must:

- connect only selected supports in the confirmed run;
- connect only compatible shapes or templates;
- connect only matching attachment names;
- stop at the final selected support;
- reject a span beyond the configured maximum distance;
- warn or stop at excessive heading or grade changes;
- reject duplicate supports and zero-length spans;
- avoid connecting across an ambiguous branch;
- roll back the complete operation if output generation fails.

These rules favor predictable author intent over aggressive automatic discovery.

## Generated output

Possible output organizations are:

### One generated shape per span

This is simple and local but can create many shape and world-object records.

### One generated shape for the complete run

This minimizes object count but produces poor locality for long runs, complicates
tile-boundary behavior, and may reduce effective culling.

### One generated shape per tile or bounded section

This is the recommended initial production approach. It balances object count,
editing, regeneration, culling, and world-tile ownership.

Short runs contained in one tile may naturally produce one generated shape.
Long runs should be divided into tile-local or otherwise bounded sections.

## Multi-tile runs

Runs may cross any number of 2048-metre world tiles. The generated span must
remain visually continuous across every boundary.

Recommended process:

```text
1. Resolve every attachment and generate the complete curve in world coordinates.
2. Intersect the curve precisely with world-tile boundaries.
3. Split geometry at those calculated intersection points.
4. Use the identical boundary vertex on both neighboring sections.
5. Convert each section into the owning tile's local coordinates.
6. Save one generated shape/object per tile or bounded section.
```

Both sides are derived from one world-space curve, preventing gaps, mismatched
sag, or independently rounded endpoints at tile borders.

The generated object for each section belongs to the appropriate world tile.
This supports normal Open Rails tile loading and culling.

## Editing and regeneration

Generated spans should remain reproducible after a support moves or the run
settings change. The route itself must continue to use compatible generated
shape and world objects, while TSRE may retain authoring metadata that identifies:

- the ordered support UIDs;
- matched attachment names;
- span preset and settings;
- generated output files and objects;
- generation version.

The exact storage location for this metadata requires a separate compatibility
decision. It must not introduce an unknown object into MSTS/Open Rails world
files.

Until regeneration metadata is implemented, the first proof of concept may
generate a disposable test output and require explicit regeneration of the
selected run.

## Shape generation

The available Blender MSTS/Open Rails exporter is the primary practical
reference for the required `.s` structure, hierarchy, materials, primitives,
distance levels, and companion `.sd` information.

Two implementation paths are possible:

```text
EXTERNAL PROTOTYPE
    Generate geometry through Blender and use the established exporter.

NATIVE TSRE GENERATOR
    Adapt the verified output requirements into a focused TSRE shape writer.
```

The external path is suitable for proving geometry and compatibility. A native
writer would remove Blender as a runtime dependency and provide the intended
integrated workflow.

The generator should not attempt to become a full modeling application. It
needs only the constrained geometry and materials required by supported span
presets.

## File compatibility and asset management

Generated content must:

- use valid MSTS/Open Rails shape and shape-definition syntax;
- use stable route-local filenames;
- reference available route textures and materials;
- preserve route world-file encoding and serialization behavior;
- avoid overwriting hand-authored assets;
- update or replace only output owned by the selected generated run;
- be validated by reloading in TSRE and Open Rails.

Generated filenames should be deterministic and collision-resistant. Repeating
the same generation operation should update its owned output rather than create
an unlimited series of abandoned shapes.

## Performance

Per-tile or bounded-section output provides the most practical runtime balance:

- fewer objects than one shape per individual span;
- better loading and culling locality than one shape for a complete long run;
- predictable regeneration scope;
- natural ownership by route world tile;
- bounded geometry size and memory use.

Curve-segment counts should be based on span length and curvature within safe
limits. Increasing tessellation beyond visible benefit only enlarges shape files
and rendering work.

Performance should be measured in Open Rails rather than inferred solely from
world-object or draw-call counts.

### Very long runs

The design must support route-scale telegraph runs. As an upper-bound example,
100 miles at approximately 40-metre pole spacing produces about 4,000 pole
spans. Twelve wires produce approximately 48,000 individual wire spans before
curve tessellation.

This is achievable only as bounded local work:

- retain one logical run definition but generate output per tile or bounded
  section;
- preview full geometry only around the editor camera and use coarse lines or
  summary bounds for distant portions;
- bake incrementally by tile with progress and cancellation;
- keep poles as ordinary independent objects and combine the wires within each
  generated tile section;
- use adaptive curve tessellation based on span length, sag, and viewing need;
- split geometry into safe subobjects before any shape-format index limit;
- disable collision and unnecessary shadows for wire geometry;
- provide distance levels that simplify or omit wires when they cannot
  contribute visible detail;
- regenerate only sections affected by moved supports or changed settings.

At typical spacing, one tile contains only a small portion of the complete run.
Open Rails therefore loads and renders nearby wire sections rather than one
100-mile object. The complete route may contain many generated shape files, but
only a bounded number of world objects and geometry sections are relevant near
the camera.

The generator must not create one world object per wire span. Twelve wires
between thousands of poles would otherwise produce tens of thousands of world
records and defeat the purpose of baking.

### Nine-tile working batches

TSRE may process long runs in a 3 x 3 working set of nine world tiles centered
on the active route area. This provides a bounded unit for detailed preview,
geometry generation, validation, and file output.

Each of the nine tiles is still processed as an individual output section:

```text
Load the run data needed by the 3 x 3 working set
Resolve a small neighboring support/curve halo
Generate and validate each tile's wire section independently
Write or update that tile's generated shape and world object
Release completed geometry buffers
Advance to the next 3 x 3 batch
```

The neighboring halo is important. A tile cannot calculate its boundary wire
from only the poles whose origins lie inside that tile. It must also see the
adjacent support or curve segment needed to derive the exact world-space
intersection. Both neighboring outputs then use the same calculated boundary
vertex.

The nine-tile batch is an editor working and memory limit, not a new route-file
format and not an assumption that Open Rails always loads exactly nine tiles.
Every generated section remains an ordinary tile-owned shape/object, allowing
Open Rails to apply its own viewing distance, world-tile loading, and shape
distance levels.

Within the active working set:

- nearby tiles may display the complete detailed wire preview;
- tiles approaching the preview limit may use simplified lines;
- geometry outside the working set may be represented by run bounds or omitted;
- each generated shape may contain distance levels appropriate to wire
  visibility;
- completed tiles may be discarded from editor memory after validation.

Long generation should maintain a run manifest recording pending, completed,
failed, and validated tile sections. Output for each tile should be staged and
replaced atomically so cancellation or failure cannot leave a truncated shape.
The builder may resume an interrupted bake without regenerating already
validated, unchanged tiles.

Although tiles are processed individually, the complete selected run remains
one logical generation job. Undo or rollback must use the manifest to remove or
restore every output section created or replaced by that job, including sections
from earlier nine-tile batches.

## Suggested progression

### Phase 1: Attachment-point proof

Export one pole shape containing two or more named wire attachments. Confirm
that TSRE can recover every name and transform accurately after placement and
rotation.

### Phase 2: Two-support preview

Select two compatible poles and preview straight and sagging spans between
matching attachment names without writing route content.

### Phase 3: Single-tile generated shape

Generate a compatible shape for one short run, place it as an ordinary route
object, and verify TSRE and Open Rails rendering.

### Phase 4: Ordered multi-support run

Add explicit run ordering, maximum-distance checks, sharp-angle warnings, and a
single-operation Undo transaction.

### Phase 5: Multi-tile splitting

Generate one world-space curve, split it at tile boundaries, and verify exact
visual continuity across the resulting generated shapes.

### Phase 6: Span presets

Add reusable wire, cable, fence-board, pipe, rail, and strip definitions with
validated materials and geometry limits.

### Phase 7: Regeneration metadata

Define a compatible editor-side ownership record so moved supports and changed
settings can regenerate their bounded output safely.

### Phase 8: Advanced runs

Consider branch markers, corners, specialized joints, complete-run discovery,
and broader loft-style features only after explicit linear runs are stable.

## Recommended first proof of concept

Use three or four telephone poles with two named wire attachments each.

```text
1. Export a pole containing wire_01 and wire_02 attachment nodes.
2. Place the poles at unequal spacing and elevation.
3. Select them in a confirmed run order.
4. Preview both wire spans with a visible sag.
5. Generate one compatible shape in a single tile.
6. Confirm exact endpoint attachment in TSRE.
7. Save, close, and reload the route.
8. Confirm the wires in Open Rails.
9. Repeat across a tile boundary and inspect the split for a visible seam.
10. Undo generation and confirm all generated world objects are removed.
```

## Final direction

Automatic Span Generation provides a focused bridge between repeated-object
placement and a future general loft system.

Route builders place ordinary supports. Model authors define named attachment
points. TSRE orders an explicit run, validates matching connections, and
generates bounded MSTS/Open Rails-compatible span shapes with correct multi-tile
continuity.
