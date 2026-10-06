# TSRE GenX v0.18 — Forest Replacement, glTF Models and Wire Workflow

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes. It combines the established TSRE editor with Qt 6/CMake,
terrain and map tools, Dynamic Track, PolyVeg, Activity Builder and Consist Builder.

> **Current version: v0.18.** Both operator-run clean Release builds passed all 28 registered tests on 6 October 2026. The operator accepted the remaining manual checks. This full Windows release follows the immutable v0.17 baseline.

## New in v0.18

### Forest regions and PolyVeg

- **Replace All Forest Regions** in the shared HACKS popup converts the route's
  legacy Forest regions using the selected F6 PolyVeg schema. Overlapping regions
  share one planting footprint; rotation, holes and cross-tile spacing are preserved.
- Preflight checks terrain coverage, schema inputs and conflicting generated assets.
  Planting uses density/rows, seed, per-tile cap, slope, spacing, feathering,
  TrackDB/RoadDB clearances and water exclusion. Ground outside terrain coverage
  receives no plants; unreadable existing terrain stops the operation.
- Progress and cancellation cover generation, placement and baking. Originals
  remain until the entire batch succeeds; failure or cancellation rolls back new
  objects and owned assets. Entirely uncovered or all-empty batches cannot commit.
  A successful batch removes **all surveyed original Forest regions**, including
  those outside planting coverage. Review the result and use normal Save afterward.
- **View > PolyVeg** independently hides or shows raw plants, baked vegetation,
  schema-listed vegetation shapes and bake markers, including selection and shadows.
  Visibility starts enabled each session and does not change saved route content.

### Open Rails glTF/GLB models

- Shape Viewer and route shape selection accept `.gltf` and `.glb` alongside legacy
  `.s` models. Static scenery and Pickup objects retain their native model references
  through normal placement and world-file saves.
- The lightweight editor preview supports base colour, PNG/JPEG textures, opaque
  and cutout surfaces, transforms, bounds and selection. Rigid animated models
  display their authored rest pose. Original model resources remain unchanged.
- Open Rails supplies final material shading and animation. TSRE does not claim
  full PBR parity or animation playback. Skinning, morphs, compressed geometry,
  blended materials and unsupported required features are rejected explicitly.
  DDS-only preview, vertex colours, seasonal/night visibility and LOD remain outside
  current support. A warning identifies the tested Open Rails embedded-DDS parser
  limitation even when TSRE can show a PNG fallback.
- Shared model resources, bounded loading and context-aware GPU cleanup protect
  repeated placement and coexistence with legacy shapes. Basic placement,
  save/reopen and simulator display have operator evidence; remaining acceptance
  is listed in the test matrix.

### Wire saving and route-wide deletion

- Exit offers **Save & Quit** when generated objects are already baked and only
  route saving remains. **Bake & Save** remains available when raw wires or PolyVeg
  still need baking; Discard and Cancel preserve their existing safeguards.
- Route-wide wire deletion shows discovery, targeted tile loading and cleanup
  progress. It loads saved tiles identified as wire candidates and includes loaded
  unsaved wires, avoiding unconditional loading of every scenery tile.
- Saved world references protect generated shapes until Save; surviving shapes
  protect shared textures. Malformed saved worlds stop discovery before removal.
  Recorded saved wire pairs continue to prevent duplicate Commit/Bake placements.

## All GenX Features at a Glance

### Terrain, textures, seasons and water

- Track/road terrain conforming, selected-object conforming, smoothing and separate
  database height biases support track beds, cuts and embankments across tile borders.
- Whole-tile patch selection and Track, Road and shoreline painting complement
  route-local brush presets, full texture rotation and validated multi-file loading.
- Native 4 m/8 m terrain tools and resolution-aware texture matrices keep saved
  terrain mapping consistent with Open Rails; ambiguous custom scales are preserved.
- Main/Snow previews, Recent textures, seasonal/Night sets, paired Mirror Season
  painting, ACE/DDS alternatives and coordinated cache refresh improve texturing.
- Water rulers, shoreline processing, fitted seam-matched surfaces, terrain
  adjustment, Waterbed Offset and Undo support river and shoreline work.
- Confirmed terrain/TERRTEX resets, Water Tiles Off and generated-map cleanup retain
  protected source assets. Terrain saves coordinate heights, flags, metadata and textures.

### Vegetation and automated scenery

- F6 PolyVeg plants repeatable vegetation from SCO LIDEX geodata, corridors or Area
  rulers using route-local schemas, density/rows, seed, caps and clearance controls.
- The full-screen Schema Editor manages recipes and assets. Tile/LOD baking,
  raw/baked status, jump controls, bake-group properties and ownership manifests
  support review, safe saves, rebaking and cleanup.
- Forest-to-PolyVeg replacement and independent vegetation visibility extend that
  workflow to existing routes without altering unrelated scenery.
- F5 Auto Place supports track/road runs, finite range, spacing, offsets, selected
  scenery as the source and repeatable pole wobble.
- Numbered SNAP attachments support native wire Commit, blue previews, sag/width/
  span controls, Bake All Wires and selected-run/route-wide deletion. Recorded pairs
  prevent duplicates; saved references and shared textures govern cleanup.

### Track, grades and interactive objects

- Classic and NextGen Auto-Flex support compound/S-curves with transactional
  connection validation and consistent Dynamic Track mesh/database geometry.
- Grade units, Lock Grade, Grade Helper and connectivity-aware Grade Symbols
  support exact transitions and highlight direct crest/gully joins.
- Place Guard validates scenery, track and interactives, including elevated scenery
  and protection against flipping database-connected track.
- Pickup track alignment/quarter-turn rotation and eligible turntable/transfer
  activation improve interactive placement. Static instance scale is preserved.
- Legacy shapes and lightweight glTF/GLB previews coexist in supported viewers
  and Static/Pickup placement workflows.

### Maps, paths and Activity Builder

- F3 imagery supports OSM vectors, Google, Mapbox and custom providers; saved maps
  load within camera tile range, with independent overlay controls and Shift+M.
- F4 Activity Builder provides a scalable TrackDB map with live switches, deep zoom,
  rotation, route markers, labels, signals, stations and service-point overlays.
- Standalone path New/Edit/Clone/Delete sessions provide metadata validation,
  cancellation rollback, atomic PAT saves and repair of stale or disconnected paths.
- Natural-end routing, reverse points, duration/clock waits, passing sidings,
  overlap display and Undo/Redo support path construction. ORTS Extended AI controls
  include horn, uncouple, join/split and pass-red operations.
- Existing activity/traffic controls have guarded selection and preview paths;
  full Activity Builder and simulator compatibility remain subject to manual checks.

### Editor workflow, diagnostics and save safety

- Scalable charcoal/orange panels, F7 navigation/control tools, Compass, movement
  locks, pinning, search/category filters and bounded placement history simplify editing.
- Startup/root selection, Restore Last Session, route-session cleanup, per-user
  atomic JSON settings, backups, recovery, F12 controls and restart/restore support
  reliable sessions. Interface sounds follow deliberate actions.
- F11 route-wide diagnostics report orphan/isolated track, joint defects and
  unreadable worlds with progress, navigation and remembered review status.
- Create Route Health Report inventories route assets and database issues without
  automatic repair. Route-local track-section preservation and guarded conversion
  protect compatible existing definitions.
- Coordinated key-file save generations, atomic file publication, rollback and
  protected generated-asset cleanup reduce interrupted-save risk. A whole route
  remains more than one transaction; complete route backups are still required.
- Consist Builder includes normalized ORTS/MSTS includes, missing-stock replacement,
  independent preview colours, resource cleanup and isolated rendering state.

### Platform and compatibility

- Qt 6/CMake, matched 64-bit MinGW and an independent MSVC v143 lane provide
  repeatable builds while retaining legacy-renderer parity as a reference.
- MSTS/Open Rails UTF-16 serialization, bounded ACE/DDS/image handling and automated
  encoding, geometry, save and generated-asset probes protect file compatibility.
- The route-regression harness records file/terrain/database evidence. Automated
  checks complement the workflow-specific manual checks in the test matrix.

## Installation

The binary asset is **`tsre-scomod-v0.18.zip`** with its `.sha256`
companion, available from this release.

1. Download the binary ZIP and matching checksum when released. GitHub's automatic
   source archives are not the runnable editor package.
2. Verify the ZIP against its SHA-256 companion.
3. Extract the **complete archive** to a permanent writable location.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it.
6. Keep the application folder intact: executable, runtime libraries, plugins,
   assets and documents must remain together. The shortcut targets that folder.

Back up routes before terrain conforming, 4 m work, water processing, route-wide
saves, Forest replacement or PolyVeg baking. Test these workflows on a copy first.
The separately released SCOsnapPole example and `SNAP-POLE-WIRE-USER-GUIDE.md`
provide model conventions and installation guidance; the example is not an installer.

## Requirements, Verification and Documentation

- 64-bit Windows 10 version 1809 or later, or Windows 11. Windows 7 and Qt5/qmake
  builds are outside the supported platform.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for geodata planting.
- Source builds use Qt 6.11.1; MinGW 13.1 is the release baseline and MSVC v143
  the independent verification lane.
- Wire discovery targets connected support runs; ambiguous branching/parallel
  networks require review. The route wire registry supports up to 8,192 spans.
- See `TEST-MATRIX-v0.18.md` for verified results and coverage. Earlier release
  passes do not count as v0.18 acceptance. Final executable hashes are recorded in the test matrix; verify the ZIP against its matching checksum companion.
- Terrain, TERRTEX, seasonal painting, PolyVeg, Water Tools, Dynamic Track, Grade
  Helper, Path Editor/Full Map and SNAP Pole/Wire guides describe detailed workflows.
  The work list contains the same grouped feature summary without historical detail.

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file and rendering systems. His later TSRE5vc work provided
independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment and DDS work supplied important reviewed references.

Wayne Campbell and Pete Willard are thanked for their MSTS/ORST exporter code,
which was used as a reference.

## License

TSRE GenX is distributed under the GNU General Public License version 3 or later.
Third-party components retain their own licenses; see `LICENSE.md` and
`THIRD-PARTY-NOTICES.txt`.
