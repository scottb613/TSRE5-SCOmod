# TSRE GenX Route Editor

TSRE GenX is a Windows route editor for Microsoft Train Simulator and Open
Rails routes. It carries the established TSRE editor forward with Qt 6 and
CMake builds, expanded terrain and map workflows, Dynamic Track, PolyVeg,
route diagnostics, Activity Builder work, and a consistent GenX interface.

GenX is an actively developed editor, not a demonstration port. Back up a
route before major terrain, water, track, or vegetation work and evaluate new
workflows on a copy first.

> **Current development line:** v0.15. The v0.14 public tag was withdrawn.

## Release Highlights

### v0.15 — Corrective PolyVeg Safety and Water Clearance

- Corrects the generated-asset cleanup defect exposed after v0.14. Valid
  compressed-binary MSTS world files are now recognized and malformed or
  unsupported input stops cleanup before any generated file can be deleted.
- Commits PolyVeg bake ownership before route Save can write references into
  world files. A later Discard can no longer remove bake geometry that a saved
  route may already reference.
- Adds required schema-controlled water clearance. Planting excludes terrain
  below the current water surface, applies a configurable setback, and follows
  the live terrain and water geometry across tile borders.
- Applies water exclusion consistently to OSM/flood and ruler/area planting.
- Blanks and freezes the viewport during single-tile and loaded-LOD bakes while
  retaining responsive progress reporting.
- Keeps the earlier tile-border conforming, terrain-water, interface, parsing,
  save-transaction, texture, and diagnostic work that passed the v0.14 gate.

v0.15 is an active development line receiving a **corrective source safety
push** on `tsre-scomod-wip` with an annotated `v0.15` source tag. The tag stores
this exact reviewed source snapshot; it is not a frozen binary release and does
not create a GitHub Release, upload a binary, or invoke GitHub Actions. The
latest downloadable binary release remains v0.12.

### v0.14 — Withdrawn

- Corrected TrackDB/RoadDB terrain conforming at world-tile borders and kept F2
  Size/Intensity text, sliders, and brush state synchronized.
- Completed F7 Water Tools with explicit special-ruler modes, bounded 4 m/8 m
  processing, repeatable terrain clearance, long-ruler corrections, and
  operation-specific Undo.
- Introduced the full-screen PolyVeg Schema Editor and transactional ownership
  for generated bake files, manifests, Discard, and route Save.
- Hardened PolyVeg geometry validation, legacy-orphan cleanup, route-save
  transactions, malformed-input handling, ACE allocation, and TrackDB/RoadDB
  validation.
- Restored authored static-track shapes and made missing or loading shape
  materials render magenta rather than inherit stale terrain texture state.

Post-checkpoint route testing exposed a catastrophic PolyVeg cleanup/save
ownership defect with valid compressed-binary world files. The remote v0.14
tag was removed; its exact annotated tag and frozen evidence are retained
locally only for forensic comparison. Use v0.15 instead.

### v0.13 — Route Editor, 4 m Terrain, and Diagnostics

- Unified the Route Editor interface and completed editable experimental 4 m
  terrain with bounded native overlays.
- Strengthened route-session cleanup, refreshed Activity Builder, and improved
  diagnostics and long-consist selection.

### v0.12 — PolyVeg and Dual-Compiler Verification

- Added deterministic PolyVeg planting and baking.
- Added the MSVC verification lane alongside the MinGW parity/release build.
- Preserved Static matrix scale, bounded saved overlays, optimized terrain-byte
  correction, expanded 4 m display, and hardened NextGen Auto-Flex.

### v0.11 — Dynamic Track and CMake-Only Transition

- Made NextGen S-C-S-C-S the single Dynamic Track solver.
- Added water, vegetation, and grade rulers and strengthened parsing, textures,
  DDS handling, markers, and shaders.
- Completed the CMake-only transition.

### v0.9 and the Original GenX Line

- Migrated the complete application from Qt 5/qmake to Qt 6.11.1/CMake while
  preserving MSTS/Open Rails encoding and the legacy renderer as the parity
  reference.
- Added repeatable builds, regression probes, route health reporting, safer
  saves, modern settings, terrain tools, and the first full Activity Builder.

## Installation

### Current Source

1. Check out the `tsre-scomod-wip` branch for continuing development or the
   `v0.15` tag for this exact reviewed source safety snapshot.
2. Install the Qt 6.11.1 and vcpkg prerequisites described by the checked-in
   CMake presets.
3. Keep machine-specific paths in `CMakeUserPresets.json`.
4. Configure and build the MinGW Release baseline with:

   ```powershell
   .\scripts\Invoke-CMake.ps1 --preset windows-release-local
   .\scripts\Invoke-CMake.ps1 --build --preset windows-release-local --clean-first
   ctest --test-dir .\build\windows-release-local --output-on-failure
   ```

Do not copy only `TSRE5.exe` into an older runtime. The executable, Qt runtime,
plugins, and packaged assets must remain compiler-matched.

### Latest Packaged Binary

The latest public binary is [v0.12](https://github.com/scottb613/TSRE5-SCOmod/releases/tag/v0.12).
v0.15 intentionally has no GitHub Release or binary download.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11.
- Qt 6.11.1 for source builds. The MinGW 13.1 build is the parity/release
  baseline; MSVC v143 is the independent verification lane.
- Existing Microsoft Train Simulator or Open Rails route content for editing.
- PolyVeg consumes the route-local geodata derivatives produced by SCO LIDEX.
- GenX remains experimental. Maintain route backups, especially before terrain
  conforming, water processing, route-wide saves, or PolyVeg bakes.
- Windows 7 and Qt 5/qmake builds are not supported.

## Verification

The operator has confirmed the corrected v0.15 editor behavior, PolyVeg water
setback, bake viewport blanking, Save, and reload. Final MinGW and MSVC Release
builds, full CTest results, and executable hashes are recorded in
[`TEST-MATRIX-v0.15.md`](TEST-MATRIX-v0.15.md) before the source checkpoint is
pushed.

## Documentation

This source folder contains the public editor source and synchronized release
documents:

- [`RELEASE-NOTES-v0.15.md`](RELEASE-NOTES-v0.15.md) — complete grouped v0.15
  release notes.
- [`TEST-MATRIX-v0.15.md`](TEST-MATRIX-v0.15.md) — automated and manual
  verification record.
- [`scoKeyList.txt`](scoKeyList.txt) — Route Editor shortcuts.
- [`scoUiStyle.txt`](scoUiStyle.txt) — canonical interface rules.
- [`scoWorkList.txt`](scoWorkList.txt) — historical engineering record.
- [`THIRD-PARTY-NOTICES.txt`](THIRD-PARTY-NOTICES.txt) — third-party licenses
  and acknowledgments.

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file, and rendering systems. His later TSRE5vc work also
provided independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API
reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment, and DDS work supplied important reviewed references.

TSRE GenX continues because these complementary bodies of work were shared
with the community.

## License

TSRE GenX is distributed under the [GNU General Public License version 3 or
later](LICENSE.md). Third-party components remain under their respective
licenses.
