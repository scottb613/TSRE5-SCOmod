# TSRE GenX v0.22 - Route Autopaint and Final TERRTEX Processing

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes, with Qt 6/CMake, terrain and map tools, Dynamic Track,
PolyVeg, Activity Builder and Consist Builder.

> **Current version: v0.22.** Clean MinGW and MSVC Release builds and all 39
> registered tests passed on 9 November 2026, including the enabled reference corpus.
> The binary package contains the manually accepted MSVC v143 runtime.

## Major Updates Since v0.21

### Route painting and native texture dimensions

- **Auto Paint All TERRTEX:** run up to ten saved-preset Track, Roads and Water
  passes in order, with independent Main/Snow mirroring per row. Invalid checked
  pairs block Commit and are revalidated before painting.
- Completed tiles save transactionally, with patch disk staging, a bounded
  decoded-source cache and viewport suspension. Cancel stops between tiles and
  retains completed saves. Route autopaint clears Undo; keep an editable backup.
- Manual and route painting save native-size, uncompressed ACE without mipmaps.
  There are no format or size selectors in the paint panels or route dialog.
  Older recipe output fields are ignored while preserving the paint sequence.
- Normal whole-tile Track/Roads/Water painting coalesces GPU uploads while
  preserving Undo and manual Save.

### Final TERRTEX processing

- **Process All TERRTEX:** independently select **Keep Existing / ACE / DDS**,
  **Compress Uncompressed Textures**, and **Keep / Remove / Add Missing Mipmaps**.
  All seasonal folders are included and source dimensions are preserved.
- Compression applies opaque, lossy DXT1 only to uncompressed pixels. Existing
  compressed blocks are not recompressed. Mipmap-only operations preserve the
  base and retained authored levels; adding mips appends missing levels.
- ACE/DDS conversion works in both directions, updates detailed and distant
  terrain references, and removes superseded originals after verification.
  Container-specific small mip tails are translated; unsupported lossy block
  transfers, partial ACE mip chains and destination collisions are refused.
- Atomic file publication and durable checkpoints support resume with locked
  settings. Original and replacement hashes are checked before reference
  publication and again before cleanup. Externally changed originals stop resume
  before references change. This final-processing tool retains no original
  texture backups or Undo and does not provide whole-route rollback.

### Texture decoding and verification

- The maintained native ACE codec has focused format, alpha, mipmap and input
  validation coverage. Both compiler lanes match all 68 reference textures exactly.
- Standard byte-channel DDS decoding avoids repeated per-pixel mask searches.
  Optional startup diagnostics separate route setup and texture decoding; no
  universal route-loading speedup is claimed.
- Both clean Release builds pass all 39 registered tests with no skips,
  including the enabled local reference corpus.
  The refreshed MSVC runtime and manual conversion/save/reopen workflow were
  accepted by the operator on 9 November 2026.

## Complete GenX Feature Summary

### Terrain shaping and high-definition grids

- **Track and road conforming:** stronger cuts and embankments, useful narrow
  widths, separate database height biases and corrected tile-border distances.
  F conforming, Ctrl+F selected-object conforming, Shift+F smoothing and the
  Conform DB brush support broad shaping and local grade-following correction.
- **Native 4 m/8 m terrain:** editing, patch outlines, Undo, seams and coordinate
  calculations support detailed 512-sample grids alongside normal 256-sample
  grids. Mesh Res identifies the actual terrain spacing.
- **Persistent terrain mapping:** resolution-aware rotation, mirror, reset and
  whole-tile map transforms match the saved Open Rails coordinate domain.
  Clear legacy fixed-16 signatures are normalized conservatively; ambiguous
  custom scales remain untouched.
- **Coordinated terrain saves:** height, flag, descriptor and seasonal texture
  writes use staged publication and rollback on failure, retaining dirty state
  for retry. This does not replace complete route backups.

### Terrain painting, seasonal textures and output

- **Pointer-tile autopaint:** with F3 open, hover over visible loaded terrain:
  Alt+1 paints track, Alt+2 roads and Alt+3 water edges across that tile using
  the current brush. Terrain-height ray picking crosses tile boundaries without
  precise object aiming. Window shortcuts work across panel focus; held keys do
  not repeat. F12/menu hints list the keys; the status bar reports results or
  missing targets. The pointer must be inside the route viewport.
- **Compressed ACE compatibility:** accepts the file reader's expanded compressed
  wrapper with exact length checks, avoiding erroneous signature rejection
  before DDS production conversion while retaining image/allocation limits.

- **Painting coverage:** whole-tile patch selection, automatic track/road
  painting and elevation-following Water Edge painting reduce repetitive work.
  Shorelines use feathered transitions rather than coarse patch outlines.
- **Texture selection:** paired Main/Snow previews, Recent textures, a brush
  swatch, validated multi-file ACE/DDS/image loading and filename tooltips make
  sources visible and report unusable files before painting.
- **Route-local presets:** selecting a preset immediately loads its source and
  brush settings. Brush scale, intensity, shape and full 0-360 degree rotation
  support repeated work; scale measurement remains correct after rotation.
- **Seasonal display:** Summer, Spring, Autumn, Winter, Snow and Night choices
  refresh materials using ordered per-file fallbacks. ACE/DDS alternatives and
  WinterSnow vegetation flags are respected; shapes prefer DDS within a season.
- **Mirror Season:** paired default/snow painting validates matching sources,
  shows the active side and disables invalid mirroring. Original dimensions,
  including larger TERRTEX, are retained during painting.
- **Painting and final output:** manual painting saves uncompressed ACE without
  mips. Process All TERRTEX selects final Keep/ACE/DDS format, independent DXT1
  compression of uncompressed inputs and Keep/Remove/Add missing mipmaps.
- **Texture/cache corrections:** rectangular cropping, RGB row alignment,
  reload identity and shared-destination staging are corrected. Repeated
  ACE-to-DDS references reuse decoded data rather than multiplying cache entries.
- **Reset and production tools:** confirmed tile/route resets restore canonical
  detailed/distant materials and remove matching generated assets. Route-wide
  seasonal **Process All TERRTEX** converts ACE/DDS in either direction, updates
  terrain references and removes verified superseded inputs. Existing compressed
  blocks/authored levels are retained; atomic writes/checkpoints support resume.
  First-time DXT1 compression is lossy/opaque; no original backups or Undo remain.

### Water tools and special rulers

- **F7 Water Tools:** New, Add Points and Edit Points manage a terrain-snapped
  ruler; Process Water Tiles, Adjust Terrain and Undo provide explicit stages.
  Bounded shoreline scans fit seam-matched surfaces and handle shared corners.
- **Waterbed Offset:** consistent clearance beneath sloping water surfaces
  avoids repeated lowering. Supported 4 m/8 m grids use resolution-aware
  processing; mixed-grid operations are rejected instead of guessed.
- **Water flags and rulers:** Water Tiles Off clears water-patch flags without
  resetting other terrain data. Special rulers offer visible editable handles,
  terrain snapping and Undo while leaving the ordinary measurement Ruler separate.
- **Grade ruler:** a two-endpoint guide displays rise/run as percent, per mille,
  1-in-X and angle for planning vertical transitions.

### PolyVeg planting, schemas and forest conversion

- **F6 PolyVeg:** deterministic planting follows SCO LIDEX polygons, style-aware
  Flood Fill, corridor/Area rulers or a one-shot Plant Full Tile click.
  Local planting can disable Use OSM; mapped workflows require their geodata.
- **Recipes and clearances:** weighted shapes, density, cap, repeatable seed,
  yaw/scale variation, orchard rows and feathering combine with slope,
  footprint, spacing, overlap and permanent mapped exclusions.
  TrackDB/RoadDB setbacks and schema-controlled water clearance protect corridors
  and submerged ground.
- **Local corridors and feedback:** F5 side/setback/width/range controls share
  schema settings and check duplicate areas. Compact controls, high-count
  generation paths and copyable zero-result reports explain planting outcomes.
- **Shift+F6 Schema Editor:** full-screen route-local catalogs include asset
  search, static thumbnails, recipes, defaults and limits with validated atomic
  saves and compatibility with older catalog keys.
- **Tile/loaded-LOD baking:** raw vegetation becomes managed shape blocks.
  Raw/Baked/Tile status and read-only bake-group properties show ownership,
  counts and coordinates without ordinary object-transform controls.
- **Safe ownership:** manifests distinguish generated work from manual scenery.
  Cleanup checks saved references, including compressed-binary MSTS worlds,
  and stops on uncertain input. Save-attempt ownership prevents Discard deleting
  an asset that a partially completed save has already referenced.
- **Replace All Forest Regions:** schema-driven replacement respects rotation,
  holes, overlapping footprints, spacing and terrain coverage. Original forests
  remain until batch success; failed/cancelled work rolls back new objects/assets.
  Successful conversion removes all surveyed originals, so review coverage first.
- **Independent visibility:** View > PolyVeg controls raw plants, bakes,
  catalog vegetation and markers without changing saved route content.

### Auto Place, SNAP poles and native wires

- **F5 Objects:** place selected scenery along track, roads or the independent
  cyan Ruler (ap), with finite range, spacing, offsets, terrain following and
  repeatable pole wobble. Normal E-selection can choose the source object.
- **Ruler placement:** available through F1 TSRE Tools, with editable points,
  Preserve Corners and neighbor-based post headings for fences. Successful
  placement resets the guide; partial work retains it for a duplicate-safe retry.
- **SNAP wire Commit:** numbered support attachments define native spans,
  including compatible shape-family variants, nearby track-node boundaries
  and ruler runs away from tracks. Recorded pairs prevent duplicates.
- **Wire appearance:** sag, width and maximum span govern blue raw previews.
  Plain retains sagging geometry; straight Barbed uses brown steel artwork,
  crossed alpha strips and adjustable blunt-barb width.
- **Wire lifecycle:** Save bakes raw runs; Bake All Wires, Preview Off,
  selected-run deletion and route-wide deletion provide explicit control.
  Saved references, shared textures and bounded registries protect cleanup.
  A separate example pack and guide document compatible SNAP pole models.

### Dynamic Track, grades and interactive objects

- **NextGen Auto-Flex:** the S-C-S-C-S solver handles compound connections and
  S-curves with geometry, length, tile and connection validation before mutation.
  It replaces Classic Flex and uses one editor-owned Dynamic Track category.
- **Geometry consistency:** database rebuilding, synchronized elevation,
  corrected graded-curve guides and authored static-track shapes keep editing,
  rendering and stored track data aligned.
- **Grade workflow:** Lock Grade carries selected track/road grade into new
  placement. Grade Helper builds connected per-piece transitions to an exact
  target; grade units use consistent precision across panels.
- **Grade Symbols:** connectivity-aware steady, transition and crest/gully
  markers cover static and Dynamic Track. Cached connectivity avoids unnecessary
  rebuilds during ordinary display.
- **Track/interactives:** eligible edits update surviving attached items and
  remove items on deleted geometry. Route-local section definitions survive
  saves; occupied-ID conflicts require explicit conversion, inspection or cancel.
- **Place Guard and properties:** invalid placements can trigger automatic Undo
  and distinct feedback; elevated scenery is supported and database-connected
  track has X-flip protection. Static Matrix3x3 scale survives edits, Pickup
  alignment handles grades, and eligible turntable/transfer models can activate.

### Maps, paths, Activity Builder and viewers

- **F4 Geo imagery:** keyless OSM vectors plus Google, Mapbox and Custom
  providers retain independent settings. High-resolution saved overlays load
  within camera tile range and release resources outside it.
- **Map controls:** explicit tile/route visibility avoids forced loading.
  M toggles saved maps; Shift+M hides terrain and temporarily hides its visible
  map, restoring the paired display together.
- **F10 Activity Builder:** a full-screen TrackDB map supports deep zoom,
  rotation, live switch changes, markers, labels, signals, stations and service
  points with coordinated path/activity controls.
- **Standalone paths:** New/Edit/Clone/Delete validates metadata, rolls back
  cancellation and writes PAT files atomically. Repair handles stale/disconnected
  paths; parallel-junction anchors retain the exact main/passing connection.
- **Routing and AI controls:** natural-end routing, reverse points, duration/
  clock waits, passing sidings, overlap display and Undo/Redo support authoring.
  ORTS Extended AI includes horn, uncouple, join/split and pass-red operations.
- **International text and activity guards:** path metadata/cloning preserve
  umlauts, sharp s and combining accents. Safer path/consist initialization,
  traffic selection and timetable handling improve existing activity previews;
  full simulator behavior still requires workflow-specific checking.
- **Consist Builder:** normalized MSTS/ORTS include paths, Unicode-whitespace
  parsing, missing-stock replacement, accurate long-consist picking and
  independent preview colors improve rolling-stock editing.
- **glTF/GLB previews:** supported Static/Pickup, Shape Viewer and Consist/freight
  models retain native references and transforms. Standard 4K textures and
  simplified transparency complement legacy shapes; Open Rails owns final
  PBR shading and animation, while the editor displays rest pose.
- **Viewer reliability:** DDS covers DXT1/3/5 and uncompressed RGB/RGBA, including
  NVIDIA-style headers. Scaled-display rotation, isolated rendering state and
  resource cleanup improve repeated model viewing.

### Route diagnostics, distribution and object copying

- **Legacy world footers:** route/generated-asset cleanup recognizes complete
  Comment blocks after the world root, avoiding blanket retention of otherwise
  unused assets. Incomplete worlds, malformed comments, extra world/object roots
  and unknown trailing data remain protected.

- **F11 world scan:** progress, cancellation and incomplete-coverage reporting
  accompany orphan/isolated-track and joint/connectivity checks. Jump/Select,
  thresholds and remembered No Factor status support review.
- **Rejected worlds and route reports:** unreadable worlds remain visible while
  valid tiles scan; eligible malformed-name removal retains a recovery copy.
  Create Route Health Report inventories assets/database issues and atomically
  saves a consolidated Copy All report without automatic route repair.
- **Pack for Distro review:** full-route dependency scanning lists selectable
  categories, sizes and warnings. All three scan options default on; removal
  candidates start unchecked. Category selection and numeric sorting aid review.
- **Protected dependencies:** placed models, SD, seasonal/night ACE/DDS, glTF
  resources and retained catalogs remain protected. Missing content can allow
  unrelated cleanup; uncertain data protects affected resources.
- **Cleanup categories:** unused models/textures, backups, artwork, caches,
  osm_data and terrain_maps overlays are reviewable. Optional _e.raw/_n.raw
  removal preserves elevation _y.raw, flags _f.raw and other RAW data.
- **TERRTEX consolidation:** complete byte-identical seasonal/format families
  share verified terrain-reference updates before duplicates are removed.
  Images are not recompressed; unsupported inputs retain assets.
- **Cleanup accounting:** revalidation, verification, removal and measurement
  feed total progress and UTF-8 reports with actual savings/partial outcomes.
  Delete Selected is permanent, without backup or Undo; keep editable backups.
- **Static Object Copy To:** remembered receiving-route/Working Folder choices
  and an object context command support resource reuse. Route mode asks for
  Name/Category and retains catalog placement settings and static flags.
- **Complete copies:** native/compressed models, SD, seasonal/night ACE/DDS
  and external glTF resources follow the model. Identical destination files
  are reused; conflicts or missing dependencies stop copying.
- **Publication and export:** staging precedes atomic REF updates, preserving
  existing encoding; new catalogs use UTF-16LE. Failures roll back new resources.
  Working Folder mode creates numbered object folders without REF changes;
  copying transfers resources rather than instance coordinates.

### Interface, sessions and save reliability

- **GenX interface:** scalable charcoal/orange panels, active-tool feedback,
  tooltips, aligned values and contextual helpers cover properties/settings.
  Latching helpers support pinning/snapping; Transform/Height, signal subobjects
  and selection-sensitive HACKS provide focused editing controls.
- **Objects and navigation:** F1 search/category filters, numeric-aware track
  ordering and bounded Recent Items include pasted objects. Ctrl+F1 opens the
  Control Panel with Compass, coordinate-pair copying and movement locks.
- **Predictable controls:** E/Q/R/T/Y activate editing modes idempotently;
  focus returns after appropriate actions. Shaded 3D markers and optional 2D
  symbols preserve exact picking colors; passive refreshes do not play sounds.
- **Sessions and F12:** validated roots/New Route coordinates, Restore Last
  Session, remembered layouts and configurable Instance Protection aid startup.
  Categorized settings, key help, atomic per-user JSON, timestamped backups and
  Save, Restart and Restore support recovery.
- **Whole-operation Save:** progress covers generated baking, terrain/texture
  encoding, publication and cleanup. Large payloads stage to disk; coordinated
  generations, backup/rollback and duplicate-destination checks reduce partial
  failure risk without claiming one transaction for the entire route.
- **Generated-work decisions:** Save & Quit, Bake & Save, Discard and Cancel
  distinguish pending work. Failed saves/cleanup keep the editor open for retry;
  discarded objects invalidate affected selection and clipboard references.
- **Input/operation safeguards:** bounded reads, decompression/structure checks,
  ACE allocation safeguards and explicit Unicode validation reject malformed
  input. Shared locks and fresh dependency checks coordinate copying/cleanup;
  external editors still require a quiescent route.

### Platform, compatibility and delivery

- **Qt 6/CMake:** the complete application uses CMake/CTest and vcpkg-managed
  dependencies. Obsolete qmake projects, the unfinished activity simulator and
  its unused OpenAL dependency were removed.
- **Windows lanes:** matched MinGW remains the parity/package baseline;
  Visual Studio 2026/MSVC v143 provides an independent lane and the manually
  accepted v0.22 binary package. Supported systems
  are 64-bit Windows 10 1809 or later and Windows 11.
- **Compatibility:** MSTS/Open Rails UTF-16 and native route/terrain/object/path
  formats remain central constraints; the legacy renderer remains the parity
  reference. Regression probes and route-evidence hashing support fresh checks.
- **Delivery:** versioned compiler-matched working packs, shortcut helpers,
  frozen public documents and checksum manifests identify distributions.
  Current-version verification is recorded separately in its test matrix.

## Installation

The binary asset is **`tsre-scomod-v0.22.zip`** with its `.sha256` companion.
This full release contains the reviewed, compiler-matched MSVC v143 runtime.

1. Download the binary ZIP and matching checksum from the v0.22 release.
   GitHub's automatic source archives are not the runnable Windows package.
2. Verify the ZIP against its SHA-256 companion.
3. Extract the **complete archive** to a permanent writable location.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it, and keep the extracted application folder intact.
6. If the Microsoft Visual C++ runtime is unavailable, run the included
   `vc_redist.x64.exe` installer before starting the editor.
7. Keep the executable, matched runtime libraries, plugins, assets and documents
   together. Do not mix compiler runtimes or copy only a new executable into an old pack.

Back up routes before terrain conforming, water processing, route-wide saves,
Forest replacement or PolyVeg baking. Test substantial work on a copy first.
Production DXT1 conversion deliberately retains no TERRTEX backup: use a separate
final upload copy and retain your editable route separately.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11. Windows 7 and Qt5/qmake
  builds are unsupported.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for mapped PolyVeg.
- Source builds use Qt 6.11.1. Both MinGW 13.1 and MSVC v143 are verified;
  this download uses the operator-accepted MSVC runtime.
- Wire discovery is bounded to 512 supports and 256 tiles per run, with an
  8,192-span registry. It cannot infer the intended path among nearby parallel lines.
- glTF preview is a bounded base-colour/rest-pose preview, not full simulator
  PBR, animation or intersecting-transparency parity.
- Pack for Distro permanently deletes selected files without backups/Undo;
  its title does not mean ZIP creation. Copy To adds catalog resources, not world
  placements. External editors do not honor the internal operation locks. Abrupt
  process failure can leave unreferenced resources or safe partial terrain updates.
- Both clean Release compiler suites passed all 39 registered tests in
  the full runs with no skips (68/68 reference textures exact).
  Manual acceptance is recorded in the test matrix; exhaustive route coverage
  and automatic recovery from arbitrary external edits are not claimed.

## Source and Documentation

The immutable **v0.22** tag identifies the reviewed release source. `tsre-scomod-wip` remains the maintained public source branch.
`TEST-MATRIX-v0.22.md` records build hashes, tests and manual coverage. Verify the
binary ZIP with its matching SHA-256 companion before extraction.

The package includes Pack for Distro and Static Object Copy To guides, plus
terrain, TERRTEX, seasonal painting, PolyVeg, Water Tools,
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
