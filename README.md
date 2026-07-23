# TSRE GenX

TSRE GenX is an experimental improvement branch of the TSRE5 Trainsim.com fork, focused on practical route-editor improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

The goal is not to replace Eric's main TSRE work. This is a test branch for debugging, experimentation, and evaluation, with the hope that useful pieces can eventually be reviewed and folded upstream.

## Branches

- `tsre-scomod-stable` is the stable/rescue branch, capped and tagged at `v0.6`.
- `tsre-scomod-wip` is the development branch; its current work-in-progress release is `v0.7`.

## Highlights

- Updated `code` to build with current MSYS2/MinGW tooling.
- Reworked `F` terrain conforming for stronger, more practical track cuts and embankments, including improved low-end track-width behavior.
- Added `Ctrl+F` selected-object tile conforming, `Shift+F` non-destructive smoothing, and the `F2` Conform DB brush for grade-following spot cleanup.
- Added `Alt+A` selection of all 256 terrain texture patches on the selected tile.
- Added `autopaint` tile-level track and road painting plus shoreline-aware water-edge painting based on the actual terrain/water contour.
- Added `F2` confirmed route-wide and current-tile TERRTEX reset tools for returning painted terrain to `terrain.ace`.
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
- Improved `Object Panel` with category-aware search and reset, cleaner Scale Rail grouping, and 30 Recent Items in a bounded scrollable history with Clear Recent.
- Reworked `F1-F4 panels` property panels, Object Panel, Control Panel, Settings Editor, and startup screens into a consistent scalable interface.
- Substantially reorganized `F2 textures` around a larger selected-texture preview, a compact two-column Recent Texture bank, clearer paint/preset/brush groups, and texture-name tooltips.
- Added `global UI` scaling, consistent manual-action sound feedback, cleaner square F12 boolean selectors, and coordinated state colors without false responses during passive refreshes.
- Added `Place Guard` validation with automatic undo and distinct feedback for dangerous or invalid scenery, track, and interactive-object placements.
- Reorganized `F12 Settings` into practical categories with direct settings saves, timestamped backups, full-sentence tooltips, and an integrated key-assignment reference.
- Repaired `F3 maps` OSM Vector Map HTTPS support, hardened failed-response handling, and increased generated map imagery to 4096 resolution.
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

