# TSRE GenX v0.19 — Terrain Textures, Auto Place and Production Workflows

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes. It combines the established TSRE editor with Qt 6/CMake,
terrain and map tools, Dynamic Track, PolyVeg, Activity Builder and Consist Builder.

> **Current version: v0.19.** Both operator-run clean Release builds passed all 32 registered tests on 26 October 2026. The operator accepted the final manual checks. This full Windows release follows the immutable v0.18 baseline.

## Major Updates Since v0.18

### Terrain textures, presets and output

- Selecting a saved texture preset immediately loads its source and brush settings;
  the separate Apply button is removed. Invalid/missing sources report an error.
  Brush Scale, full rotation, previews and Recent controls support repeated painting.
- Independent **Mipmaps** and **DDS** output switches apply to subsequently painted
  patches. DDS uses native opaque RGB24 output; ACE remains available. Presets do
  not change output switches. Existing base dimensions are preserved, including
  2048-square TERRTEX, and matching seasonal format conversions are coordinated.
- Saved mipmaps use linear-light, alpha-aware filtering with repeating texture
  edges. Unsupported ACE mip dimensions stop Save instead of silently resizing.
- Corrected rectangular painting/cropping, RGB pixel-row alignment and canonical
  texture cache keys. Distant-terrain reset chooses terrainDM.dds, terrainDM.ace,
  then terrain.ace according to available assets.
- Shape textures prefer DDS over ACE within each ordered seasonal directory.
  Snow also recognizes WinterSnow-capable vegetation, retaining seasonal precedence
  and ordinary missing-texture fallback.

### Full-operation progress and final route production

- Route Save reports the combined operation, including pending PolyVeg and wire
  baking, terrain/TERRTEX encoding, file publication and generated-asset cleanup.
  The display reaches 100% only after the operation succeeds.
- Large save payloads use temporary disk staging rather than retaining an entire
  terrain batch in memory. Existing transaction backup, rollback and duplicate
  destination protections remain; complete route backups are still required.
- Selection-sensitive **HACKS** shows actions relevant to the current object or
  terrain. Terrain offers **Compress TERRTEX — DXT1** for the complete route,
  including seasonal ACE and DDS files and terrain-reference updates.
- **Production compression introduces loss, removes alpha, should be run once,
  and retains no TERRTEX originals or backups.** Use it on the final upload copy
  after editing is complete. Progress, per-file verification and interruption
  checkpoints support large routes; this is not an archival or reversible save.

### Ruler (ap), fence posts and wire choices

- F5 Objects can follow the editable cyan **Ruler (ap)** independently of tracks
  and roads. It is also available with the other rulers under F1 **TSRE Tools**;
  placement opens Auto Place on Objects with the appropriate Follow mode.
- Spacing, offsets, terrain following and ordinary point-edit controls apply.
  **Preserve Corners** forces posts at 45–90 degree turns. Ordinary post headings
  bisect their actual neighboring spans; preserved corner orientation is exempt.
- The half-height guide has visible edit handles. Its placement button latches;
  a second click clears it. Successful object placement clears/resets the guide;
  partial placement retains it for a duplicate-safe retry. Guides are session-only.
- Wires find compatible SNAP supports along ruler-placed runs without a nearby
  track/road. Existing pair reservations prevent duplicates; Max Span and bounded
  nearest-span discovery still require inspection near parallel or branching lines.
- **Plain** remains the default with its existing sagging geometry. **Barbed** is
  straight, with brown steel wire and blunt, constant-width barbs drawn on two
  crossed alpha strips. Barbs are shorter and thicker; Width scales the silhouette.
  Raw blue guides bake into textured scenery on Save. Existing bakes retain their
  material until explicitly deleted and replaced.

### PolyVeg planting and controls

- Compact collapsible controls, aligned Density/Cap values, a separate Seed column
  and adjacent ruler actions improve F6 layout. The old Jump controls are removed.
- **Use OSM** is optional for local ruler/area/full-tile planting. Turning it off
  avoids requiring map derivatives; terrain, slope, water and database clearances
  remain. Mapped polygon/flood workflows still require their geodata.
- **Plant Full Tile** arms the next terrain click for one tile, with cancellation
  and existing planting exclusions. F5 corridors offer side, setback, width and
  range controls with shared schema/settings and duplicate-area checks.
- Zero-result reports explain coverage, targets, terrain and exclusion counts;
  they scroll and can be copied. Report suppression leaves validation errors visible.

### glTF/GLB consists and international path names

- Consist Viewer and on-track previews accept glTF/GLB locomotive/wagon bodies
  and freight models alongside legacy shapes. Authored offsets/pivots and Flip
  are retained; corrected handedness/direction and depth clears improve previews.
- Standard 4096-square textures and bounded larger containers are supported.
  Simplified transparent surfaces draw after solids with graphics-state restoration.
  Open Rails still owns final PBR shading and animation; TSRE displays rest pose.
- Path metadata and standalone cloning preserve German umlauts, sharp s and other
  Unicode letters/combining accents without transliteration. Existing UTF-16
  route/activity/service/path serialization is retained.

## All GenX Features at a Glance

### Terrain, textures, seasons and water

- Track/road terrain conforming, selected-object conforming, smoothing and separate
  database height biases support track beds, cuts and embankments across tile borders.
- Whole-tile patch selection and Track, Road and shoreline painting complement
  route-local brush presets, full texture rotation and validated multi-file loading.
- Native 4 m/8 m terrain tools and resolution-aware texture matrices keep saved
  terrain mapping consistent with Open Rails; ambiguous custom scales are preserved.
- Main/Snow previews, Recent textures, seasonal/Night sets, paired Mirror Season
  painting, immediate preset loading, texture scale, native ACE/RGB24 DDS output, optional saved mipmaps and coordinated cache refresh improve texturing.
- Water rulers, shoreline processing, fitted seam-matched surfaces, terrain
  adjustment, Waterbed Offset and Undo support river and shoreline work.
- A final-production TERRTEX hack converts the full route and its seasonal ACE/DDS textures to opaque DXT1 with progress and interruption checkpoints. It is lossy, should run once, and retains no TERRTEX originals or backups.
- Confirmed terrain/TERRTEX resets, Water Tiles Off and generated-map cleanup retain
  protected source assets. Terrain saves coordinate heights, flags, metadata and textures.

### Vegetation and automated scenery

- F6 PolyVeg plants repeatable vegetation from SCO LIDEX geodata, corridors or Area
  rulers using route-local schemas, density/rows, seed, caps and clearance controls.
- The full-screen Schema Editor manages recipes and assets. Tile/LOD baking,
  raw/baked status, bake-group properties and ownership manifests
  support review, safe saves, rebaking and cleanup.
- Forest-to-PolyVeg replacement and independent vegetation visibility extend that
  workflow to existing routes without altering unrelated scenery. Optional OSM, full-tile planting, corridor setback/width and zero-result diagnostics support local planting.
- F5 Auto Place supports track/road runs and cyan Ruler (ap) fence lines, finite range, spacing, offsets, selected
  scenery as the source and repeatable pole wobble. Ruler placement supports editable points, corner preservation, smooth ordinary post headings and automatic guide reset.
- Numbered SNAP attachments support native wire Commit, blue previews, sag/width/
  span controls, Bake All Wires and selected-run/route-wide deletion. Recorded pairs
  prevent duplicates; saved references and shared textures govern cleanup. Plain wire retains sag; straight Barbed uses adjustable crossed alpha strips and brown steel artwork.

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
  and Static/Pickup placement workflows. Consist previews also support glTF/GLB bodies and freight models, standard 4K textures and simplified transparency.

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
  full Activity Builder and simulator compatibility remain subject to manual checks. Unicode path metadata and cloning retain German umlauts and other supported letters.

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
  remains more than one transaction; complete route backups are still required. Overall save progress includes generated baking, texture encoding and publication; large transaction payloads spill to temporary disk staging.
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

The binary asset is **`tsre-scomod-v0.19.zip`** with its `.sha256` companion.
This full release contains the reviewed, compiler-matched MinGW runtime.

1. Download the binary ZIP and matching checksum from the v0.19 release.
   GitHub's automatic source archives are not the runnable Windows package.
2. Verify the ZIP against its SHA-256 companion.
3. Extract the **complete archive** to a permanent writable location.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it, and keep the extracted application folder intact.
6. Keep the executable, matched runtime libraries, plugins, assets and documents
   together. Do not mix compiler runtimes or copy only a new executable into an old pack.

Back up routes before terrain conforming, water processing, route-wide saves,
Forest replacement or PolyVeg baking. Test substantial work on a copy first.
Production DXT1 conversion deliberately retains no TERRTEX backup: use a separate
final upload copy and retain your editable route separately.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11. Windows 7 and Qt5/qmake
  builds are unsupported.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for mapped PolyVeg.
- Source builds use Qt 6.11.1; MinGW 13.1 is the release baseline and MSVC v143
  is the independent verification lane.
- Wire discovery is bounded to 512 supports and 256 tiles per run, with an
  8,192-span registry. It cannot infer the intended path among nearby parallel lines.
- glTF preview is a bounded base-colour/rest-pose preview, not full simulator
  PBR, animation or intersecting-transparency parity.
- Both clean Release compiler suites passed 32/32 tests. The operator accepted
  the final manual checks. See the current test matrix for hashes and coverage.

## Source and Documentation

The immutable **v0.19** tag identifies the exact reviewed release source. `tsre-scomod-wip` remains the maintained public source branch.
`TEST-MATRIX-v0.19.md` records build hashes, tests and manual coverage. Verify the
binary ZIP with its matching SHA-256 companion before extraction.

The package includes terrain, TERRTEX, seasonal painting, PolyVeg, Water Tools,
Dynamic Track, Grade Helper, Path Editor/Full Map and SNAP Pole/Wire guides.
`scoWorkList.txt` contains the same grouped feature summary without release history.
The optional SCOsnapPole model example remains a separate route-content download.
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
