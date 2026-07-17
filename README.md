# TSRE5-SCOmod

TSRE5-SCOmod is an experimental improvement branch of the TSRE5 Trainsim.com fork, focused on practical route-editor improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

The goal is not to replace Eric's main TSRE work. This is a test branch for debugging, experimentation, and evaluation, with the hope that useful pieces can eventually be reviewed and folded upstream.

## Branches

- `tsre-scomod-stable` is the stable/rescue branch, currently tagged `v0.4`.
- `tsre-scomod-wip` is the current work-in-progress branch with the latest tested local changes tagged `v0.5`.

## Highlights

- Improved F-key terrain conforming for track cuts and embankments.
- Adjusted F-tool track-width behavior so the 1/2/3 settings are more practical.
- Added a `Shift+F` terrain smoothing pass for selected track/ruler objects.
- Added selected-object `Ctrl+F` tile conforming for selected track/road objects.
- Added an F2 `Conform DB` height brush mode for grade-following spot terrain cleanup along track and road databases.
- Added tile-level autopaint tools for track, roads, and water.
- Added real shoreline/water-edge terrain painting based on terrain/water contour detection.
- Added route and tile terrtex reset tools for returning painted terrain tiles to `terrain.ace`.
- Added route-local F2 terrain paint presets for texture, brush size, intensity, brush shape, and rotation.
- Added a 0-360 degree terrain texture rotation control for seamless directional textures.
- Added an F2 seasonal selector for Summer, Spring, Autumn, Winter, and Night.
- Added seasonal fallback refresh for terrain, route objects, transfers, dynamic track, and forest/polyforest geometry.
- Fixed transfer-object reload behavior when switching from Winter back to Summer.
- Added Mirror Season for paired default/snow TERRTEX painting with matching paired textures required.
- Disabled the old settings-file `season` / `seasonalEditing` controls so the F2 selector is the active seasonal control.
- Protected editable 1024x1024 terrtex files from accidental downsampling while painting.
- Added forest/object stutter mitigation and a View menu toggle for Forest Regions.
- Fixed a texture-cache invalidation bug that caused severe lag and wrong texture reuse on large populated routes.
- Added single-instance startup protection.
- Fixed route-selection table refresh when switching MSTS root folders.
- Added a Windows executable icon and `AddShortcutDesktop.cmd` helper.
- Added build fixes for current MSYS2/MinGW tooling.
- Added Restore Last Session on the startup screen to reopen the last route, camera view, and editor window layout.
- Added a high-resolution branded startup splash and scaled loader/about display so the banner is not cropped.
- Converted the Status Window into a compact clickable control panel.
- Added gentle delayed snapping for the Status and Navi windows.
- Matched the Navi Window color/readability scheme to the darker Status Window style.
- Improved right-side object panel searching with `ALL` category defaults, mutually exclusive Tracks/Roads/Other filters, and a `Reset` search button.
- Added global `uiScale` support for larger editor fonts and proportionally wider panels.
- Added editor sound feedback with standardized `SCOclick.wav`, `SCObuzz.wav`, and `SCOchirp.wav` files.
- Added Place Guard validation with automatic undo, red status-panel error flash, and click/buzz WAV feedback.
- Updated startup and editor titlebars to identify the build as `TSRE SCOmod v0.5`.
- Reworked the F12 Settings Editor with active `settings.txt` saving, timestamp backups, organized tabs, dark striped rows, a Key Assignments tab, and full-sentence tooltips.
- Fixed F3 OSM Vector Map HTTPS loading by packaging the current OpenSSL 3 runtime DLLs.
- Hardened F3 OSM Vector Map loading so failed network replies do not crash the editor.
- Set downloaded map imagery resolution to 4096 for clearer per-tile map output.

## Place Guard

Place Guard validates object placement after TSRE performs its normal placement action. Rejected placements are automatically undone, the Status Window's Place Guard button flashes `ERROR` for three seconds, and `SCO_buzz.wav` plays. Accepted placements play `SCOclick.wav`.

Current validation rules:

- All guarded placements must finish on the camera tile or one immediately adjacent tile.
- Track-linked interactives, including signals and other track/road database items, must be started with the pointer within 3 meters of the target track or road database line.
- Track-linked interactives must finish within 10 meters of the sampled database elevation.
- Normal scenery/static objects must land on loaded terrain and within 1 meter above or below the terrain surface.
- Track objects and dynamic track are allowed a wider edit tolerance: 50 meters below terrain to 100 meters above terrain.
- Turning Place Guard off from the Status Window restores legacy placement behavior.

## UI Scaling

The editor now supports a global `uiScale` setting. The packaged setting uses `uiScale = 1.15`; `1.00` to `1.25` is the recommended range. This scales the main editor font, menus/dropdowns, startup screen, F2/F3 style panels, object list side panel, Status Window, and Navi Window.

## Object Search And Sounds

The right-side object panel now has `ALL` defaults for Tracks, Roads, and Other. Choosing a specific value in one of those three filters resets the others to `ALL`, and the search box only searches inside the active filter. The `Reset` button clears the search box, returns the filters to `ALL`, and repopulates the full object list.

Sound feedback is intentionally separated:

- `SCOclick.wav` plays after successful guarded object placement.
- `SCObuzz.wav` plays after Place Guard rejects and undoes a placement.
- `SCOchirp.wav` plays for deliberate user-commanded mode/status changes.

Passive status refreshes stay silent to avoid duplicate sounds after placement or automatic state changes.

## Settings Editor

The F12 Settings Editor now follows the same organization as the cleaned `settings.txt` file. Settings are grouped into practical tabs such as General, Logging, Startup, UI, Camera, Rendering, Overlays, Objects, Terrain, Map, Cleanup, Advanced, and Consist.

Saving from the Settings Editor now writes the active `settings.txt` file instead of `settings.txt.new`, and TSRE creates a timestamped backup before replacing the old file. Row comments were moved into full-sentence tooltips, and the new Key Assignments tab provides a striped two-column shortcut reference inside the editor.

## Downloads

Executable test builds are intended to be published on the GitHub Releases page, not committed directly into the source tree.

Download the current test ZIP from Releases when available. Keep this copy separate from any production TSRE install.

The ZIP includes `AddShortcutDesktop.cmd`, which can create a desktop shortcut for the packaged `TSRE5.exe`.

The ZIP also includes `LICENSE.md` and `THIRD-PARTY-NOTICES.txt`. TSRE5-SCOmod follows the original TSRE5 GPLv3 license; bundled runtime DLLs remain under their respective upstream licenses.

## Documentation

- `worklist.txt` contains the longer forum-style summary of the work.
- `fileEdit.txt` lists the code/project files touched during the work.
- `Published/` contains the release-facing README and copied summaries used for package/release notes.

## Status

This is not an official TSRE release. It is a working route-editor improvement branch for testing and discussion.

