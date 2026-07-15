# TSRE5-SCOmod

TSRE5-SCOmod is an experimental improvement branch of the TSRE5 Trainsim.com fork, focused on practical route-editor improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

The goal is not to replace Eric's main TSRE work. This is a test branch for debugging, experimentation, and evaluation, with the hope that useful pieces can eventually be reviewed and folded upstream.

## Branches

- `tsre-scomod-stable` is the stable/rescue branch, currently tagged `v0.2`.
- `tsre-scomod-wip` is the current work-in-progress branch with the latest tested local changes.

## Highlights

- Improved F-key terrain conforming for track cuts and embankments.
- Adjusted F-tool track-width behavior so the 1/2/3 settings are more practical.
- Added a `Shift+F` terrain smoothing pass for selected track/ruler objects.
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

## Downloads

Executable test builds are intended to be published on the GitHub Releases page, not committed directly into the source tree.

Download the current test ZIP from Releases when available. Keep this copy separate from any production TSRE install.

The ZIP includes `AddShortcutDesktop.cmd`, which can create a desktop shortcut for the packaged `TSRE5.exe`.

## Seasonal TERRTEX Painting

TERRTEX painting has only two real output targets:

- default `TERRTEX`
- snow `TERRTEX/SNOW`

The F2 texture set selector still helps preview Summer, Spring, Autumn, Winter, and Night texture lookup for route objects, transfers, dynamic track, forest, and polyforest textures. For terrain painting, however, Mirror Season only works between default TERRTEX and snow TERRTEX. It does not write Spring, Autumn, Fall, or Night terrain folders.

When `Mirror Season` is on, TSRE applies the same brush/autopaint trace to the paired TERRTEX side, but it uses the paired side's matching source texture. Default painting uses the default texture. Snow painting uses the snow texture. If the matching paired texture does not exist, the mirror paint is skipped for that stroke.

When `Mirror Season` is off, TSRE still protects the opposite TERRTEX side from blank tiles. If painting creates a new per-tile terrain swatch, TSRE creates a matching placeholder swatch on the paired default/snow side if one does not already exist. That placeholder comes from the route's paired-side `terrain.ace` when available, or from the bundled SCOmod `terrain.ace` fallback.

## Documentation

- `worklist.txt` contains the longer forum-style summary of the work.
- `fileEdit.txt` lists the code/project files touched during the work.
- `Published/` contains the release-facing README and copied summaries used for package/release notes.

## Status

This is not an official TSRE release. It is a working route-editor improvement branch for testing and discussion.
