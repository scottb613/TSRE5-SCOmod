# TSRE5-SCOmod

TSRE5-SCOmod is a small experimental branch of the TSRE5 Trainsim.com fork, focused on practical route-editor improvements for MSTS and Open Rails route building.

This branch is based on Eric's `TSRE8.006 baseline` from the `master` branch of `eric-from-trainsim/TSRE5-Trainsim.Com-Fork`.

## Current Focus

- Better terrain conforming around track cuts and embankments.
- A `Shift+F` terrain smoothing pass for selected track/ruler objects.
- Tile-level terrain autopaint tools for track, roads, and water.
- Water-edge/shoreline terrain painting based on the actual terrain/water contour.
- Protection against accidental 1024 terrtex downsampling while painting.
- A confirmed route terrtex reset tool for returning painted terrain tiles to `terrain.ace`.
- Route-local F2 terrain paint presets for texture, brush size, and intensity.
- Build fixes for current MSYS2/MinGW tooling.
- Initial editor stutter mitigation around forest/object loading bursts.

## Downloads

Executable test builds are intended to be published on the GitHub Releases page, not committed directly into the source tree.

The `Published` folder contains release notes and the worklist summary. The runnable `.zip` package is built locally from the dist folder and should be uploaded as a Release asset.

## Notes

This is not an official TSRE release. It is a working route-editor improvement branch for testing and discussion.

See `worklist.txt` for the longer forum-style summary of the current changes.
