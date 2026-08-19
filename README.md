# TSRE GenX

TSRE GenX is an actively developed Windows route editor for Microsoft Train
Simulator and Open Rails routes. It combines the established TSRE foundation
with Qt 6/CMake builds, expanded terrain and map workflows, Dynamic Track,
PolyVeg, route diagnostics, and a consistent GenX interface.

GenX is experimental and under active development, but it is a full route
editor rather than a demonstration port. Back up production routes and test
major operations on a copy.

Current source checkpoint: **v0.13**

## Release Highlights

### v0.13 - Route Editor workflow and production pass

- Unified remaining panels, properties, helpers, and popups around the compact
  PolyVeg-derived charcoal/orange interface.
- Standardized pinned, snapped, mutually exclusive, latching `...` windows.
  Contextual helpers close when their calling property panel is no longer active.
- Made editable fields select their value on entry for immediate overwrite.
- Reworked Dynamic Track, Signal, Terrain, Static, Forest, and other properties
  for aligned labels and fields, compact controls, and useful tooltips.
- Added paired Control Panel coordinate copying, orange copied confirmation,
  compact Location labeling, and a low-contrast route title over the compass.
- Promoted Errors & Messages to styled F11 with direct deletion of identified
  invalid TrackDB/RoadDB items. F12 now toggles Settings normally.

### Terrain, maps, and route sessions

- Completed editable experimental 512-sample, 4 m terrain support across
  conforming, Undo, patch outlines, gaps, and renderer data lifetime.
- Added native 2048/4096 overlays with indexed drawing, bounded camera-area
  residency, explicit Map Tiles controls, and map-off purge.
- Hardened detailed-terrain and overlay cleanup during travel and route changes.
- Changed Exit to Main Load into a guarded restart so reopened routes begin with
  clean route-owned state and memory.

### PolyVeg and Activity Builder

- Shortened the F7 PolyVeg stack, retained deterministic planting and baking,
  added Map Tiles control, and reserved Edit Schema for the planned editor.
- Refreshed F5 as a map-first workspace with compact fixed-width side panels,
  improved path controls, readable legends, and unclipped short labels.
- Kept map overlays user-controlled when entering F5.

### Reliability and cleanup

- Improved Consist and Activity Consist selection across long consists and
  unresolved stock cards.
- Removed the unfinished in-editor activity simulator and unused OpenAL path.
- Consolidated future interface rules in `scoUiStyle.txt`.

## Verification

Clean Qt 6.11.1 Release builds completed on 2026-08-19. Both registered suites
passed 7/7.

- MinGW/TST SHA-256: `C6DDD2BA7C5E68FE7412C1BB98A4457A915ECC409254F412A1FADCE36B121D9F`
- MSVC SHA-256: `062B82FFFBF945DCB52C617B9010E88C082E97FA180C1BEBF7F0CB96D6FDEE15`

See `TEST-MATRIX-v0.13.md` for coverage and focused manual checks.

## Earlier GenX Milestones

### v0.12

Added deterministic PolyVeg planting and baking, the MSVC build lane, Static
matrix-scale preservation, bounded saved overlays, optimized terrain-byte
correction, expanded 4 m display, and hardened NextGen Auto-Flex.

### v0.11

Made NextGen S-C-S-C-S the single Dynamic Track solver; added water, vegetation,
and grade rulers; strengthened parsing, textures, DDS, markers, and shaders; and
completed the CMake-only transition.

### v0.9 and the original GenX line

Migrated the full application from Qt 5/qmake to Qt 6.11.1/CMake while
preserving MSTS/Open Rails encoding and the legacy renderer. Added repeatable
builds, regression probes, route health reporting, safer saves, modern settings,
terrain tools, and the first full Activity Builder.

## Release Status

v0.13 is a reviewed **source checkpoint** on `tsre-scomod-wip` and immutable
tag `v0.13`. A local distributable is produced for verification, but no v0.13
binary is uploaded and no GitHub Release is created. The latest uploaded binary
remains v0.12.

Supported systems are 64-bit Windows 10 version 1809 or later and Windows 11.

## Build

```powershell
.\AAA_Compile.ps1 -Configuration Release -Clean
```

Machine paths belong in `CMakeUserPresets.json`; checked-in presets remain
portable.

## Documentation

- `RELEASE-NOTES-v0.13.md` - grouped v0.13 notes
- `TEST-MATRIX-v0.13.md` - verification status
- `scoKeyList.txt` - Route Editor shortcuts
- `scoUiStyle.txt` - canonical interface rules
- `scoWorkList.txt` - historical engineering record
- `THIRD-PARTY-NOTICES.txt` - licenses and acknowledgments

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file, and rendering systems. His later TSRE5vc work also
provided independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment, and DDS work supplied important reviewed references.

TSRE GenX continues because these complementary bodies of work were shared with
the community.

## Repository

https://github.com/scottb613/TSRE5-SCOmod
