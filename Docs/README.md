# TSRE GenX

TSRE GenX is an actively developed Windows route editor for Microsoft Train
Simulator and Open Rails routes. It combines the established TSRE foundation
with Qt 6/CMake builds, expanded terrain and map workflows, Dynamic Track,
PolyVeg, route diagnostics, and a consistent GenX interface.

GenX is experimental and under active development, but it is a full route
editor rather than a demonstration port. Back up production routes and test
major operations on a copy.

Current source checkpoint: **v0.14**

## Release Highlights

### v0.14 - Terrain, Water, PolyVeg, and production safety

- Fixed TrackDB/RoadDB terrain conforming at world-tile borders and made F2
  Size/Intensity text, sliders, and brush state remain synchronized.
- Completed F7 Water Tools with explicit special-ruler modes, bounded 4 m/8 m
  water processing, repeatable terrain clearance, long-ruler corrections, and
  operation-specific Undo.
- Added the full-screen PolyVeg Schema Editor and transactional ownership for
  generated bake files, manifests, discard, and route Save.
- Hardened PolyVeg geometry validation and legacy-orphan cleanup and registered
  a self-contained baker probe.
- Added transactional terrain saves plus stronger malformed-input, ACE
  allocation, TrackDB/RoadDB, and route-save guards.
- Restored authored static-track shapes and made missing/loading shape materials
  render magenta instead of inheriting stale terrain texture state.
- Consolidated Route Editor shortcuts, global map-overlay control, responsive
  sizing, and GenX checkbox/spin-control styling.

## Verification

Clean Qt 6.11.1 MinGW and VS2026/MSVC Release builds completed on 2026-08-26.
The expanded registered suite passed 15/15 in both lanes.

- MinGW/distribution SHA-256: `327612590FF056EB82E6ACBE82A3E64411249A777565FF0839393C35284FBB78`
- MSVC SHA-256: `4789690A99D4845DF3645C7CB24E4B19EE8E8EADE57C8BEDECE05CBD93B99C4B`

The operator accepted v0.14 with no outstanding release testing. See
`TEST-MATRIX-v0.14.md` for exact evidence and accepted future hardening.

## Earlier GenX Milestones

### v0.13

Unified the Route Editor interface, completed editable experimental 4 m terrain,
bounded native overlays, strengthened route-session cleanup, refreshed Activity
Builder, and improved diagnostics and long-consist selection.

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

v0.14 is a reviewed **source checkpoint** on `tsre-scomod-wip` and immutable
tag `v0.14`. Complete local binary and source packages are retained, but no
v0.14 binary is uploaded and no GitHub Release is created. The latest uploaded
binary remains v0.12.

Supported systems are 64-bit Windows 10 version 1809 or later and Windows 11.

## Build

```powershell
.\AAA_Compile.ps1 -Configuration Release -Clean
```

Machine paths belong in `CMakeUserPresets.json`; checked-in presets remain
portable.

## Documentation

- `RELEASE-NOTES-v0.14.md` - grouped v0.14 notes
- `TEST-MATRIX-v0.14.md` - verification status
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
