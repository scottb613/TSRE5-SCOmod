# TSRE GenX

TSRE GenX is a maintained open development line of the TSRE5 route editor, focused on practical improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

GenX has grown beyond its initial experimental and migration role and now continues as an open community test branch. Its aim is simply to keep improving the editor into the best practical and reliable tool its contributors and testers can make it. It is not being prepared solely for later inclusion elsewhere, although useful ideas and focused fixes can still move between related projects when their maintainers find that helpful.

TSRE GenX combines Piotr Gadecki's original TSRE5 foundation, Eric Olesen's continuing TSRE 8.006 development as eric-from-trainsim, Peter Grønbæk Andersen's independent Qt6/CMake work, and the GenX route-building improvements described below.

## Branches

- The version tags through `v0.8` preserve the earlier stable checkpoints.
- `master` contains the source-only v0.9 Qt6/CMake line.
- `tsre-scomod-wip` carries continuing community development; immutable tag `v0.12` is the latest completed source checkpoint.

## v0.12 highlights

- Added a supported Visual Studio 2026/MSVC v143 CMake build lane alongside
  the Qt 6.11.1 MinGW parity and release-package baseline.
- Added the F6 PolyVeg Planter for deterministic, tile-clipped planting from SCO LIDEX contract-v2 polygons or a corridor/Area ruler.
- Added style-aware Flood Fill, exclusions and database clearances, repeatable seeds, route-global orchard rows, and high-count planting optimizations.
- Added loaded Raw/Baked/Tile status, cycling jump controls, Bake PolyVeg Tile, and bounded Bake PolyVeg LOD with safe generated-asset cleanup.
- Preserved and rendered Static-object Matrix3x3 instance scale.
- Hardened Reset All TERRTEX for canonical detailed/distant reset and safe generated-texture/saved-map cleanup.
- Made F3 saved PNGs lazy-resident inside camera Tile LOD. Route sessions start with overlays Off; extended SCO_LHR travel stayed at or below about 1.5 GB.
- Fixed optimized-MSVC terrain byte order and added display-only 512-sample/4 m terrain support.
- Hardened NextGen Auto-Flex, corrected scaled-display Shape/Consist rotation, standardized message sounds, improved F1 markers, retired obsolete route-image loading, and removed PolyForest without shifting neighboring IDs.

The final clean MinGW and MSVC Release builds linked with zero errors on
2026-08-16. The MinGW build matches TST at SHA-256
`026D55414585F11BD54AE25A794363A23FA08AC1C2D89E8D3785536C7E0800DF`; the
MSVC executable is `1448DA29E2F0D729BF5332573E78F713645585ECCDC2EF4185507292B15687E4`.
Both post-build automated suites passed 7/7. The 4 m gate remains display-only.

See `RELEASE-NOTES-v0.12.md` and `TEST-MATRIX-v0.12.md` for the complete community-release summary and evidence.

## v0.9 and earlier highlights

- Moved the complete application from Qt5/qmake to Qt 6.11.1, CMake, a current 64-bit MinGW compiler, and vcpkg-managed supporting libraries.
- Established one reproducible Windows build and deployment system for the Route Editor, Consist Editor, Shape Viewer, Activity Path Editor, runtime assets, and required libraries.
- Targeted native 64-bit Windows 10 version 1809 or later and Windows 11, retiring obsolete Windows 7 toolchain constraints.
- Preserved MSTS/Open Rails UTF-16 serialization with a byte-identical Qt5/Qt6 compatibility probe.
- Completed controlled startup, route save/reload, scenery editing, Place Guard, Camera Terrain Lock, and Activity Path Editor parity tests.
- Added a route regression evidence harness that hashes every route file and reports terrain, database, path, Undo, and save changes.
- Verified Activity Path Editor create, save, reload, and extension behavior while preserving all non-PAT route files, including TrackDB, RoadDB, and both item tables.
- Restored DDS rolling-stock textures in the Qt6 Consist Editor with tested DXT1, DXT3, DXT5, 24-bit, and 32-bit decoding.
- Stabilized native Windows captions, dropdown placement, Route Editor helper shutdown, rejected-Load sound feedback, Terrain Brush sizing, and Water Helper height steps.
- Retained the legacy renderer as the parity reference while keeping native compressed GPU texture work and renderer replacement as separate later work.
- Built v0.9 from the combined foundation of Piotr Gadecki (GokuMK), original author of TSRE5 who shared that invaluable work with the community; Eric Olesen (eric-from-trainsim), who continued the direct TSRE 8.006 source line; and Peter Grønbæk Andersen, whose independent Qt6/CMake and DDS-loading work provided proven modernization references.
- Updated `code` to build with current MSYS2/MinGW tooling.
- Added atomic terrain-file replacement and truthful failure reporting to route-wide cleanup tools.
- Added crash-safe coordinated saves for route databases and modified TRK files.
- Reworked `F` terrain conforming for stronger, more practical track cuts and embankments, including improved low-end track-width behavior.
- Added `Ctrl+F` selected-object tile conforming, `Shift+F` non-destructive smoothing, and the `F2` Conform DB brush for grade-following spot cleanup.
- Added `Alt+A` selection of all 256 terrain texture patches on the selected tile.
- Added `autopaint` tile-level track and road painting for faster route-wide texture work.
- Added `Water Edge` detection and painting that compares terrain elevation against the tile water plane, follows the actual shoreline with a feathered texture transition, and handles shallow-water terrain breaks instead of painting coarse 16x16 patches.
- Added the `Waterbed Offset` brush for consistent river depth beneath sloping water planes.
- Expanded `Water Helper` with one persistent terrain-snapped ruler, bounded shoreline scans, seamless fitted water surfaces, Undo, recovery, and memory limits.
- Added confirmed `TERRTEX reset` tools for route-wide and current-tile paint cleanup.
- Added `Water Tiles Off` to clear route-wide water-patch flags without changing other terrain data.
- Added `F2` route-local terrain paint presets covering texture, brush, intensity, shape, and rotation.
- Added `F2` full 0-360 degree terrain texture rotation for directional ground textures.
- Rebuilt `F2 seasonal textures` around a visible Summer, Spring, Autumn, Winter, and Night selector instead of hidden settings-file switches.
- Added `seasonal textures` active-season refresh and per-file ORTS-style fallback for terrain, route objects, transfers, Dynamic Track, forests, and polyforests, including ACE/DDS alternatives.
- Added `Terrtex: Mirror Season` for paired default/snow painting, with safe opposite-side placeholder creation when matching seasonal swatches are missing.
- Protected `seasonal TERRTEX` 1024x1024 files from accidental downsampling during painting and corrected transfer/material cache refresh behavior.
- Reduced `route performance` stutter with forest-generation controls, a Forest Regions view toggle, and texture-cache invalidation only when material paths change.
- Improved `load` reliability with single-instance protection, corrected route-table refresh, and validated editable recent MSTS/ORTS root selection.
- Added `Restore Last Session` for reopening the last route and camera context, plus compact `Pin` controls that remember the main window, Control Panel, and Grade Helper placement between sessions.
- Added `Windows` the GenX executable icon, desktop-shortcut helper, and Route > Open Route Folder command.
- Consolidated `Control Panel` Status and Navi functions into a scalable F7 panel with editing-state controls, navigation, movement locks, and delayed edge snapping.
- Revised `Category Search` with ALL defaults, mutually exclusive Tracks/Roads/Other filters, search-within-category behavior, cleaner Scale Rail family grouping, Reset, and a bounded Recent Items history.
- Added pasted objects to the same bounded `Recent Items` history as normal placements.
- Reworked `F1-F4 panels` property panels, Object Panel, Control Panel, Settings Editor, and startup screens into a consistent scalable interface.
- Rebuilt the `F2 Terrain Editor` around paired MAIN TERRTEX and SNOW TERRTEX previews, a six-item Recent Texture bank plus a dedicated Brush swatch, and clearly separated texture, preset, brush, embankment, and advanced groups.
- Added `F2 texture loading and safety` with multi-file ACE/DDS/common-image Load support, generated-tile-texture filtering, decode validation, failed-file reporting, source-filename tooltips, and a complete Clear Recent reset.
- Hardened `Terrtex: Mirror Season` so painting requires a valid same-name Main/Snow source pair, clearly marks the active output side, and automatically disables mirroring instead of leaving an invalid brush state active.
- Reduced `F2 Terrain Utilities` to a focused, pinnable Track Bias helper.
- Added a shared `Hacks` popup for Track, Static Object, and Terrain Patch properties.
- Added `Delete Instances` to remove every route instance of a selected static, gantry, or collision object.
- Added `global UI` scaling, consistent manual-action sound feedback, cleaner square F12 boolean selectors, and coordinated state colors without false responses during passive refreshes.
- Added `Place Guard` validation with automatic undo and distinct feedback for dangerous or invalid scenery, track, and interactive-object placements.
- Reorganized `F12 Settings` into practical categories with direct settings saves, timestamped backups, full-sentence tooltips, and an integrated key-assignment reference.
- Repaired `F3 maps` OSM Vector Map HTTPS support, hardened failed-response handling, and increased generated map imagery to 4096 resolution.
- Adapted Eric-from-Trainsim's `F3 imagery provider` work so keyless OSM Vector remains always available while Google, Mapbox, and Custom satellite imagery use an explicit provider selector with independently preserved settings.
- Adapted Eric-from-Trainsim's 8.006m `ORTS rolling-stock compatibility` work with safe normalized `.inc` path resolution that preserves legitimate parent-folder paths without heuristic path rewriting or duplicate inclusion.
- Adapted Eric-from-Trainsim's `route reporting` idea into Route > Create Route Health Report, which scans every route world file, atomically saves one consolidated report in the route folder, and opens it immediately in a read-only Copy All window.
- Preserved `Dynamic Track` Classic Flex while adding a Classic/NextGen selector and adapting GokuMK's work as the foundation for compound and full S-curve NextGen connections.
- Improved `Dynamic Track` editing with database rebuilding after Flex, synchronized elevation data, clearer section/length information, fallback textures, and consistent naming.
- Added `splash screens` runtime-discovered artwork with a persistent shuffled cycle shared by the Route Loader, Consist Loader, and About window during each session.
- Improved `track properties` grade-unit presentation and added Lock Grade so newly placed track or road pieces can inherit the selected piece's physical grade.
- Added `Grade Helper` for dock-friendly, validated, connected, per-piece vertical transitions to an exact target grade, with remembered placement and automatic Lock Grade handoff.
- Added `Grade Symbols` connectivity-aware markers for static and Dynamic Track, including steady-grade, transition, and direct crest/gully classifications.
- Added `View and Markers` reversible overlay toggling and improved marker placement, color, height, and label reliability.
- Added `Edit > Copy Info` selection diagnostics for clipboard-ready troubleshooting and reporting.
- Reduced `route display` overhead by rebuilding Grade Symbol connectivity only at safe tile/database boundaries, reusing unchanged pointer-depth results, and throttling unchanged Control Panel updates.
- Refined `route lighting` with warmer directional sunlight and cooler sky-facing ambient light while preserving MSTS texture artwork and neutral editor overlays.
- Added `Save, Restart and Restore` in F12 plus optional UI sounds for startup actions, toggles, checkboxes, dropdown selections, and menu commands.
- Added `property guidance` for Detail Level, MSTS StaticFlags, and MSTS Collision, clarified Alt+A in the key reference, and corrected read-only Control Panel hover behavior.
- Added the full-screen `F4 Activity Builder` and high-resolution TrackDB viewer.
- Added standalone MSTS/ORTS path creation, editing, validation, and PAT writing.
- Added natural-end `water flow` routing with live switch throwing and recalculation.
- Added reverse points, duration and clock-time wait points, overlap visualization, and passing sidings.
- Added selectable/deletable path controls, undo/redo, map rotation, route markers, labels, signals, stations, and service-point symbology.
- Standardized `F1-F4 and utility panels` with the charcoal/orange visual system, square selectors, consistent controls, active-tool highlighting, focus restoration, and pinnable helper windows.
- Added the pinnable `Auto Place` workflow and further Grade Helper, Control Panel, Object Selection, and Terrain Editor cleanup.
- Replaced the legacy `settings.txt` workflow with per-user `settings.json`, atomic saves, timestamped backups, reset-to-default support, and automatic preservation/recovery of damaged JSON.
- Added route-session cleanup so reopening or changing routes does not retain stale route-owned state.
- Added portable first-run defaults, bounded AppData housekeeping, and safe selection-color recovery.
- Added `terrain conform settings` for independent TrackDB and RoadDB height bias without re-invoking the full Game load path.
- Hardened `Dynamic Track textures` so procedural and fallback materials follow one dependable loading path without requiring an Open Rails track profile.
- Reworked `Dynamic Track display` with a track-following selection outline, normal percent-grade presentation, equal Select/Place controls, and a continuous world-space yellow TrackDB overlay.
- Rebuilt `NextGen Auto-Flex` around direction-neutral, transactional connection solving that rejects U-turns, backtracking paths, one-ended joins, and bad internal or external seams.
- Corrected `Dynamic Track geometry` so the object mesh and TrackDB share the same endpoint plane, tangent, grade frame, and curve-to-straight transitions; verified with smooth Open Rails train tests in both grade directions.
- Added `Advanced Shunting WP` support for ORTS Extended AI operations: timed horn, uncouple from either end, join/split, and permission to pass the next red signal.
- Expanded `F4 path editing` with explicit New/Edit/Clone sessions, selectable and deletable controls, undo/redo, live switch recalculation, reverse points, wait points, passing sidings, and natural-end path routing.
- Finished `F4 path metadata and saving` with cancellation rollback, stable saved filenames, player-path flags, required endpoint information, stronger PAT validation, and atomic output.
- Hardened `F4 path repair` for stale, disconnected, open-ended, and partly unreadable PAT files.
- Improved `F4 path presentation` with named endpoints, reverse and wait-point markers, service-point identification, selected-path emphasis, overlap and passing-route visualization, and focused controls shown only while editing.
- Standardized `utility window controls` with compact icon Pin buttons, clearer movement-state colors, and reliable application shutdown even when hide-on-close helper windows remain open.
- Added a compact native `Compass` heading tape with persistent View-menu state.
- Centered confirmations, warnings, completion notices, and unsaved-change dialogs on the active monitor.
- Standardized `destructive warnings` with one fixed-width GenX dialog, safe default, and consistent wording.
- Refined `Object Properties` grouping and labels while preserving established editor functions, and removed the unused selected-terrain Shader launcher without hiding Shader ID information.
- Adapted Eric-from-Trainsim's `Main Load` idea with adjacent Load/Restore actions, a more prominent Consist Editor action, retained root selection, return-to-loader behavior, and isolated CE process state; standalone CE remains unchanged and CE background colors persist independently.
- Hardened `Consist Editor` include handling, missing-stock replacement, popup styling, independent preview colors, and cleanup.

## Acknowledgments and source lineage

Piotr Gadecki, known as GokuMK, is the original author of TSRE5. He created the
core editor, MSTS/Open Rails file handling, texture, terrain, activity, consist,
and rendering systems on which GenX is built and shared that invaluable body of
work with the community. His later TSRE5vc Qt6 and native compressed-texture
work also supplied an essential independent reference for DXT color,
transparency, alpha, and future GPU texture work.

Eric Olesen, known as eric-from-trainsim, carried TSRE forward through the TSRE
8.006 line that directly precedes GenX. His continuing editor, compatibility,
imagery, rolling-stock, route-reporting, and dedicated 8.006n Qt6 work provided
the primary source/API reference for preserving the behavior of the immediate
GenX parent line.

Peter Grønbæk Andersen produced the independent Eric-line Qt6/CMake port used
as the primary build-structure and dependency-management reference. His CMake,
vcpkg, deployment, and DDS-loading work supplied proven solutions that were
reviewed and narrowly adapted to the GenX source rather than copied wholesale.

TSRE GenX continues because these three bodies of work complement one another:
Piotr's original application and technical foundation, Eric's sustained TSRE
8.x development, and Peter's independent modern build and Qt6 migration work.

## Source and executable distribution

v0.12 is prepared from the reviewed `tsre-scomod-wip` source line. Its
immutable source tag and executable ZIP must identify the same source. The
executable package is distributed through GitHub Releases rather than stored
on a source branch.

TSRE GenX follows the original TSRE5 GPLv3 license. Runtime libraries used by
locally built copies remain under their respective upstream licenses.

## Documentation

- `scoWorkList.txt` contains the detailed change summary.
- `scoFileEdit.txt` lists the code/project files touched during the work.
- `THIRD-PARTY-NOTICES.txt` records upstream authorship, acknowledgements, and bundled runtime-library notices.
- `RELEASE-NOTES-v0.12.md` summarizes the completed source checkpoint.
- `.github/RELEASE-WORKFLOW.md` documents the guarded manual tag, draft, asset-verification, and publication process.

## Status

TSRE GenX v0.9 remains on `master`. TSRE GenX v0.12 is the latest immutable
source checkpoint from `tsre-scomod-wip`; continuing development remains on
that WIP branch.
