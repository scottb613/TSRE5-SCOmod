# TSRE GenX Route Editor

TSRE GenX is a Windows route editor for Microsoft Train Simulator and Open
Rails routes. It carries the established TSRE editor forward with Qt 6 and
CMake builds, expanded terrain and map workflows, Dynamic Track, PolyVeg,
route diagnostics, Activity Builder work, and a consistent GenX interface.

GenX is an actively developed editor, not a demonstration port. Back up a
route before major terrain, water, track, or vegetation work and evaluate new
workflows on a copy first.

> **Current release:** v0.16. This full Windows release is led by the critical
> 4 m terrain-texture persistence hotfix. The v0.14 public tag was withdrawn;
> v0.15 remains the immutable prior-release baseline.

## v0.16 Release Status

The operator accepted the complete applicable v0.16 functional matrix. Final
clean MinGW and MSVC Release builds both passed all 15 registered CTest tests;
the exact executable hashes and timings are recorded in the v0.16 test matrix.
The published v0.15 tag, package, and frozen documents remain unchanged.

## Release Highlights

### v0.16 — 4 m Terrain Texture Hotfix and Full Release

- Corrects saved terrain-texture matrices for native 512-sample/4 m terrain so
  GenX and Open Rails consume the same resolution-aware coordinate domain.
- Covers default/reset, rotation, mirror, scale, route-wide TERRTEX reset,
  generated maps, and conservative normalization of clear legacy fixed-16
  signatures while preserving established 8 m constants.
- Adds a read-only PolyVeg bake-group panel, safe attached-interactive track
  removal, consistent grade precision, idempotent editor mode keys, terrain
  mesh-resolution labeling, improved F1 ordering, exact parallel Path Editor
  serialization, and Shift+M terrain/map-state control.
- Adds LIDEX-style ownership and GPL notices only to first-party source files
  created from scratch; inherited and third-party headers remain untouched.

v0.16 is a complete binary release, not a source-only checkpoint. Its final
MinGW/MSVC evidence, executable hashes, package checksum, source archive, and
source tag are frozen as part of the release record.

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

v0.15 is the corrective full release on `tsre-scomod-wip`. Its existing
annotated `v0.15` tag stores the exact reviewed source used for the binary
package and remains immutable. The complete Windows distribution is prepared
from the matching reviewed MinGW build. No GitHub Actions were used.

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

### Source

1. Check out the `v0.16` tag for the exact release source or
   `tsre-scomod-wip` for continuing development.
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

### Packaged Binary

The current packaged binary release is
[v0.16](https://github.com/scottb613/TSRE5-SCOmod/releases/tag/v0.16).
Use the published SHA-256 companion to verify the downloaded ZIP before
extracting it.

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

The operator reports the complete applicable v0.16 functional matrix
satisfactory. Focused terrain-grid, terrain-track, and route-save transaction
probes passed against the latest existing MinGW tree. The final clean
dual-compiler build and full-suite results are still pending and will be
recorded in [`TEST-MATRIX-v0.16.md`](TEST-MATRIX-v0.16.md) before release.

## Documentation

This source folder contains the public editor source and synchronized release
documents:

- [`RELEASE-NOTES-v0.16.md`](RELEASE-NOTES-v0.16.md) — full grouped v0.16
  release-candidate notes led by the 4 m texture hotfix.
- [`TEST-MATRIX-v0.16.md`](TEST-MATRIX-v0.16.md) — automated and manual
  verification record.
- [`POLYVEG-USER-GUIDE.md`](POLYVEG-USER-GUIDE.md) — complete PolyVeg schema,
  planting, ruler, bake, cleanup, and troubleshooting cheat sheet.
- [`WATER-RULER-USER-GUIDE.md`](WATER-RULER-USER-GUIDE.md) — Water Ruler
  placement, water processing, terrain adjustment, Undo, save, and
  troubleshooting procedure.
- [`SEASONAL-MIRROR-PAINTING-USER-GUIDE.md`](SEASONAL-MIRROR-PAINTING-USER-GUIDE.md)
  — paired main/snow TERRTEX preparation, painting, preview, save, and
  troubleshooting procedure.
- [`TERRAIN-IMPROVEMENTS-USER-GUIDE.md`](TERRAIN-IMPROVEMENTS-USER-GUIDE.md)
  — improved F2 terrain brushes, conforming, waterbed work, terrain keys,
  native 4 m/8 m behavior, Auto Paint, cleanup, and save procedure.
- [`HD-4M-TERRAIN-MESH-USER-GUIDE.md`](HD-4M-TERRAIN-MESH-USER-GUIDE.md)
  — complete route conversion to the experimental 4 m terrain mesh with SCO
  LIDEX, TSRE GenX validation, Open Rails Unstable setup, rollback, and
  troubleshooting.
- [`DYNAMIC-TRACK-AUTO-FLEX-USER-GUIDE.md`](DYNAMIC-TRACK-AUTO-FLEX-USER-GUIDE.md)
  — beginner-to-advanced Dynamic Track and NextGen S-C-S-C-S Auto-Flex
  connection procedure, traditional S-C-S comparison, TrackDB safety, Open
  Rails validation, recovery, and troubleshooting.
- [`GRADE-HELPER-USER-GUIDE.md`](GRADE-HELPER-USER-GUIDE.md) — Grade Ruler,
  direct grade editing, Lock Grade, piece-by-piece Grade Helper transitions,
  orange/cyan/red Grade Symbols, save/reopen validation, Open Rails testing,
  and troubleshooting.
- [`PATH-EDITOR-AND-FULL-MAP-USER-GUIDE.md`](PATH-EDITOR-AND-FULL-MAP-USER-GUIDE.md)
  — F10 full-screen TrackDB map navigation and the complete standalone PAT
  workflow: flowing-water routing, live switches, endpoints, reverses, waits,
  advanced ORTS points, passing sidings, validation, repair, and safe save.
- [`TERRTEX-TEXTURES-USER-GUIDE.md`](TERRTEX-TEXTURES-USER-GUIDE.md) — F3
  Terrain Texture panel, validated source loading, previews and Recent/Brush
  controls, presets, rotation, manual paint, Track/Road/Water Auto Paint,
  tile selection, reset boundaries, save/reload validation, and troubleshooting.
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
