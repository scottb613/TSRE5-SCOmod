# TSRE GenX

TSRE GenX is an experimental improvement branch of the TSRE5 Trainsim.com fork, focused on practical route-editor improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

The goal is not to replace Eric's main TSRE work. This is a test branch for debugging, experimentation, and evaluation, with the hope that useful pieces can eventually be reviewed and folded upstream.

## Branches

- `tsre-scomod-stable` is the stable/rescue branch, capped and tagged at `v0.6`.
- `tsre-scomod-wip` is the development branch; its current work-in-progress release is `v0.8`.

## Highlights

- Updated `code` to build with current MSYS2/MinGW tooling.
- Reworked `F` terrain conforming for stronger, more practical track cuts and embankments, including improved low-end track-width behavior.
- Added `Ctrl+F` selected-object tile conforming, `Shift+F` non-destructive smoothing, and the `F2` Conform DB brush for grade-following spot cleanup.
- Added `Alt+A` selection of all 256 terrain texture patches on the selected tile.
- Added `autopaint` tile-level track and road painting for faster route-wide texture work.
- Added `Water Edge` detection and painting that compares terrain elevation against the tile water plane, follows the actual shoreline with a feathered texture transition, and handles shallow-water terrain breaks instead of painting coarse 16x16 patches.
- Expanded `Water Helper` with a persistent terrain-snapped watercourse ruler, bounded ruler-following shoreline scans, seam-matched water elevations, water-patch enabling, an `Undo Last` action, and a latched `Water Ruler` workflow that retains the completed ruler until it is replaced or the helper closes.
- Added confirmed TERRTEX reset tools for returning painted terrain to `terrain.ace`: route-wide reset in F2 Terrain Utilities and current-tile reset from the terrain right-click menu.
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
- Reworked `F1-F4 panels` property panels, Object Panel, Control Panel, Settings Editor, and startup screens into a consistent scalable interface.
- Rebuilt the `F2 Terrain Editor` around paired MAIN TERRTEX and SNOW TERRTEX previews, six Recent swatches plus a dedicated Brush swatch, and clearly separated texture, preset, brush, embankment, and advanced groups.
- Added `F2 texture loading and safety` with multi-file ACE/DDS/common-image Load support, generated-tile-texture filtering, decode validation, failed-file reporting, filename tooltips, and a complete Clear Recent reset.
- Hardened `Terrtex: Mirror Season` so painting requires a valid same-name Main/Snow source pair, marks the active output side, and automatically disables mirroring instead of retaining an invalid brush state.
- Moved `F2 terrain maintenance` into a pinnable Terrain Utilities helper containing TrackDB/RoadDB conform height biases and confirmed route TERRTEX reset.
- Added `global UI` scaling, consistent manual-action sound feedback, cleaner square F12 boolean selectors, and coordinated state colors without false responses during passive refreshes.
- Added `Place Guard` validation with automatic undo and distinct feedback for dangerous or invalid scenery, track, and interactive-object placements.
- Reorganized `F12 Settings` into practical categories with direct settings saves, timestamped backups, full-sentence tooltips, and an integrated key-assignment reference.
- Repaired `F3 maps` OSM Vector Map HTTPS support, hardened failed-response handling, and increased generated map imagery to 4096 resolution.
- Adapted Eric-from-Trainsim's 8.006m imagery-provider work into one `F3 maps` workflow: keyless OSM Vector plus separately saved Google, Mapbox, and Custom configurations, with resolution in F3 and no duplicate F12 Map tab.
- Adapted Eric-from-Trainsim's 8.006m `ORTS rolling-stock compatibility` work with safe normalized `.inc` path resolution that avoids duplicate inclusion and unsafe heuristic rewriting.
- Adapted Eric-from-Trainsim's `route reporting` idea into Route > Create Route Health Report, scanning every route world file, saving one consolidated report in the route folder, and opening it immediately for review and copying.
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
- Replaced legacy `settings.txt` with per-user `settings.json`, atomic saves, timestamped backups, defaults reset, and damaged-file recovery.
- Added crash-safe coordinated saves for route TDB/TIT/TSECTION, RDB/RIT, and TRK files with rolling AppData backups, rollback, and interrupted-save recovery.
- Added independent TrackDB and RoadDB terrain-conform height-bias settings without reloading the live editor.
- Hardened Dynamic Track procedural/fallback material handling without requiring Open Rails track profiles or seasonal matching.
- Reworked Dynamic Track selection, percent-grade presentation, object controls, and the continuous world-space yellow TrackDB overlay.
- Rebuilt NextGen Auto-Flex as a direction-neutral transactional solve that rejects U-turns, backtracking, one-ended joins, and bad seams.
- Aligned the Dynamic Track mesh and TrackDB on one endpoint plane with exact joint tangents and smooth internal curve/straight transitions, verified by Open Rails train tests.
- Added `Advanced Shunting WP` support for ORTS Extended AI timed-horn, uncouple, join/split, and pass-red operations.
- Expanded `F4 path editing` with explicit New/Edit/Clone sessions, selectable and deletable controls, undo/redo, live switch recalculation, reverse and wait points, passing sidings, and natural-end routing.
- Finished `F4 path metadata and saving` with cancellation rollback, stable saved filenames, player-path flags, required endpoint information, stronger PAT validation, and atomic output.
- Improved `F4 path presentation` with named endpoints, reverse and wait/advanced markers, service-point identification, selected-path emphasis, overlap and passing-route visualization, and focused editing controls.
- Standardized compact Pin icons, movement-state colors, equal Select/Place controls, and reliable shutdown with helper windows open.
- Standardized destructive confirmations with one centered GenX warning dialog, consistent red warning styling, yellow warning triangle, safe default, and shared sizing; removed the obsolete OverwriteDialog.
- Refined Object Properties grouping and labels while preserving established operations, and removed the unused selected-terrain Shader launcher without hiding Shader ID information.
- Adapted Eric-from-Trainsim's launcher idea so Main Load retains the selected root, returns after Route Editor or Load-launched CE closes, and launches CE in an isolated child process; standalone CE is unchanged and CE view colors persist.

## Downloads

Executable test builds are intended to be published on the GitHub Releases page, not committed directly into the source tree.

Download the current test ZIP from Releases when available. Keep this copy separate from any production TSRE install.

The ZIP includes `AddShortcutDesktop.cmd`, which can create a desktop shortcut for the packaged `TSRE5.exe`.

The ZIP also includes `LICENSE.md` and `THIRD-PARTY-NOTICES.txt`. TSRE GenX follows the original TSRE5 GPLv3 license; bundled runtime DLLs remain under their respective upstream licenses.

## Documentation

- `scoWorkList.txt` contains the detailed change summary.
- `scoFileEdit.txt` lists the code/project files touched during the work.
- `THIRD-PARTY-NOTICES.txt` records upstream authorship, acknowledgements, and bundled runtime-library notices.
- `docs/` contains the release-facing documentation copies.

## Status

This is not an official TSRE release. It is a working route-editor improvement branch for testing and discussion.

