# TSRE GenX Route Editor

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes. It carries the established editor forward with Qt 6/CMake,
expanded terrain and map workflows, Dynamic Track, PolyVeg, Activity Builder,
Consist Builder and a consistent GenX interface.

> **Published version: v0.20.** Both clean Release compiler lanes passed 34/34 tests on 27 October 2026; the operator authorized the official cap after desktop testing. See [release notes](RELEASE-NOTES-v0.20.md), [test matrix](TEST-MATRIX-v0.20.md), [Pack for Distro](ROUTE-CLEAN-FOR-DISTRO-USER-GUIDE.md) and [Copy To](STATIC-OBJECT-COPY-TO-USER-GUIDE.md).

## Major Updates Since v0.19

### Pack for Distro

- **Route > Pack for Distro...** scans the full saved route and shows exact
  removal candidates, categories and sizes before deleting selected items.
  All three scan options default on; deletion candidates start unchecked.
- Protects placed shapes, SD files, textures, seasonal/night variants, ACE/DDS
  fallbacks, glTF resources and retained editor catalogs. Missing-content warnings
  allow unrelated cleanup; uncertain dependency data protects affected assets.
- Optional `_e.raw` and `_n.raw` buffer removal preserves elevation `_y.raw`,
  flags `_f.raw`, other RAW data and RAW backups. Separate categories identify
  unused models/textures, backups, artwork, temporary files and editing caches.
- The default-on editing-data option includes `osm_data` and saved `terrain_maps`
  tiles/overlays. **Editing maps / overlays** has its own selectable category.
  Runtime/catalog references, uncertain dependencies and RAW data remain protected.
- Optional consolidation compares complete byte-identical TERRTEX families,
  including every seasonal/format variant. Verified terrain-reference updates
  precede removal; images are not recompressed. Unsupported inputs retain assets.
- Category selection, numeric size sorting, grouped warnings and UTF-8 reports
  make the review easier. Reports retain actual partial outcomes and measured
  whole-route before/after sizes, including descriptor-size changes.
- Total cleanup progress includes revalidation, file checks, reference updates,
  deletion, measurement and loaded-terrain refresh. **Delete Selected is permanent:
  no ZIP, backup or Undo is created.** Use the intended saved production copy.

### Static Object Copy To

- **Route > Copy To...** selects a validated receiving route or an ordinary
  **Working Folder**. Both folder choices and the mode persist between sessions;
  revalidation is silent unless there is an error.
- Selected Static Objects offer **Copy To...** in their own right-click section,
  outside Edit. Route copies ask only **Name** and **Category**. Name starts with
  the selected shape filename without path/extension and remains editable.
- Copies original models, SD companions, referenced seasonal/night textures,
  ACE/DDS counterparts and external glTF/GLB images/buffers. REF alternatives,
  alignment/random placement settings, static flags, detail and collision settings
  are retained automatically for route catalog entries.
- Identical destination resources are reused; conflicting or missing resources
  stop copying. Staging and verification precede the final atomic REF update.
  Existing REF bytes/encoding are preserved; new catalogs use UTF-16LE.
- Working mode directly exports the selected model/resources into a new object
  folder for editing, with no REF form or REF update. Repeated exports receive
  numbered folders. Relative SHAPES/TEXTURES layouts remain intact.
- Copy To has no progress bars; completion/errors report the result. Failures
  roll back newly copied resources while retaining preexisting or externally
  changed files. Placement coordinates, orientation and scale are not cloned.

### Reliability and Interface

- Explicit Unicode validation rejects truncated text and isolated surrogate
  characters without rejecting valid supplementary characters. Descriptor/REF
  fields ignore lookalike text in comments or quoted descriptions.
- Bounded dependency reads and shared destination locks protect copying and
  cleanup. Report extensions are resolved before overwrite confirmation.
- Removes the redundant standalone Select Tool from right-click menus; **E**,
  control-panel selection and the main Edit command remain available.
- Both operator-run clean Release builds passed **34/34 registered tests**.
  The operator authorized the official cap after updating the desktop test copy.

## Previous v0.19 Highlights

- Immediate texture-preset loading, brush scale, preserved TERRTEX resolution,
  optional saved mipmaps, native RGB24 DDS output and seasonal DDS/Snow lookup.
- Overall Save progress including baking and texture work, disk-staged large
  save payloads and a selection-sensitive HACKS popup.
- Final full-route opaque DXT1 production compression for ACE/DDS TERRTEX.
  **Lossy, run once, no retained TERRTEX originals or backups.**
- Cyan editable Ruler (ap), F1/F5 access, spacing/corner preservation, smooth
  ordinary post headings, latched placement and automatic guide reset.
- Straight brown-steel Barbed wire alongside Plain, adjustable silhouette width,
  shorter/thicker blunt barbs and ruler-run SNAP discovery independent of tracks.
- Compact PolyVeg controls, optional OSM for local planting, click-to-plant full
  tiles, F5 corridors and detailed copyable zero-result reports.
- glTF/GLB consist/freight previews, standard 4K textures, simplified transparency,
  corrected direction/handedness and Unicode path-name/cloning support.

See [v0.19 release notes](RELEASE-NOTES-v0.19.md) for the full changes and grouped
feature summary, and [the test matrix](TEST-MATRIX-v0.19.md) for verification status.

## Published v0.18 Highlights

- Route-wide Forest replacement with coverage checks, exclusions and rollback.
- Shape Viewer and Static/Pickup glTF/GLB base-colour previews and placement.
- Independent View > PolyVeg visibility for vegetation and bake markers.
- Clear Save & Quit versus Bake & Save prompts and responsive wire deletion.

See [v0.18 release notes](RELEASE-NOTES-v0.18.md) for the new changes followed by the complete grouped GenX feature summary, and [the test matrix](TEST-MATRIX-v0.18.md) for verified results and coverage limits.

## Previous Release Highlights

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

Download **`tsre-scomod-v0.20.zip`** and its `.sha256` companion from the v0.20
release. GitHub's automatic source archives are not runnable
Windows packages. The package uses the verified MinGW Release build.

1. Verify the ZIP against its SHA-256 companion.
2. Extract the **complete archive** to a permanent writable folder.
3. Run **`AddShortcutDesktop.cmd`** in the extracted folder.
4. Launch through the generated **TSRE GenX** shortcut. Move the shortcut only
   after creating it and leave the extracted application folder intact.
5. Keep the executable, compiler-matched runtime libraries, plugins, assets and
   documents together. Do not copy only a new executable into an older runtime.

Back up routes before terrain conforming, water processing, route-wide saves,
Forest replacement or PolyVeg baking. Test substantial work on a copy first.
Production DXT1 compression retains no TERRTEX originals/backups: run it on a
separate final upload copy and keep the editable route separately.

The optional `SCOsnapPole-TSRE-example-v0.17.zip` installs into a route separately
from the application. See the [SNAP Pole and Wire guide](SNAP-POLE-WIRE-USER-GUIDE.md).

### Source builds

Use the immutable v0.20 release tag, or `tsre-scomod-wip` for development.
Keep machine-specific paths in `CMakeUserPresets.json`. With matched Qt/vcpkg
prerequisites configured, the MinGW Release baseline uses:

```powershell
.\scripts\Invoke-CMake.ps1 --preset windows-release-local
.\scripts\Invoke-CMake.ps1 --build --preset windows-release-local --clean-first
ctest --test-dir .\build\windows-release-local --output-on-failure
```

The independent MSVC Release lane uses `windows-msvc-release-local` and CTest
with `-C Release`. Never mix MinGW/MSVC executables, libraries or plugins.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11; Windows 7 unsupported.
- Qt 6.11.1/MinGW 13.1 is the source-build parity/release baseline. MSVC v143
  provides independent verification. Qt5/qmake builds are unsupported.
- Existing MSTS/Open Rails route content; SCO LIDEX derivatives for mapped PolyVeg.
- glTF rendering is a bounded base-colour/rest-pose preview. Open Rails supplies
  final PBR shading and animation; intersecting transparency is not triangle-sorted.
- Wire discovery uses bounded nearest-span connections and requires review near
  adjacent identical fences, branches and junctions.
- The editor remains experimental. Complete route backups remain necessary.

## Verification

Both operator-run clean v0.19 Release builds passed 32/32 registered tests on
26 October 2026: MinGW in 5.90 s and MSVC in 6.08 s. The operator accepted the
final manual checks. The package uses the matched MinGW runtime.
[TEST-MATRIX-v0.19.md](TEST-MATRIX-v0.19.md) distinguishes automated, manual and
pending evidence. Final executable hashes are recorded in the matrix; the ZIP has a SHA-256 companion.

## Source and Documentation

The maintained public branch is `tsre-scomod-wip`; the reviewed immutable v0.19
tag identifies the final reviewed source. Verify the binary ZIP using its matching
SHA-256 companion. Public documents are frozen with a per-file manifest at release.

- [Release notes](RELEASE-NOTES-v0.19.md), [test matrix](TEST-MATRIX-v0.19.md) and
  [grouped feature summary](scoWorkList.txt).
- [TERRTEX textures](TERRTEX-TEXTURES-USER-GUIDE.md),
  [seasonal painting](SEASONAL-MIRROR-PAINTING-USER-GUIDE.md),
  [terrain tools](TERRAIN-IMPROVEMENTS-USER-GUIDE.md) and
  [4 m terrain](HD-4M-TERRAIN-MESH-USER-GUIDE.md).
- [PolyVeg](POLYVEG-USER-GUIDE.md), [Water Tools](WATER-RULER-USER-GUIDE.md) and
  [SNAP poles/wires and Ruler (ap)](SNAP-POLE-WIRE-USER-GUIDE.md).
- [Dynamic Track](DYNAMIC-TRACK-AUTO-FLEX-USER-GUIDE.md),
  [Grade Helper](GRADE-HELPER-USER-GUIDE.md) and
  [Path Editor/Full Map](PATH-EDITOR-AND-FULL-MAP-USER-GUIDE.md).
- `scoKeyList.txt`, `scoUiStyle.txt`, `scoFileEdit.txt` and
  `THIRD-PARTY-NOTICES.txt` supply shortcuts, UI rules, code ledger and notices.

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file, and rendering systems. His later TSRE5vc work also
provided independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API
reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment, and DDS work supplied important reviewed references.

Wayne Campbell and Pete Willard are thanked for their MSTS/ORST exporter code, which was used as a reference.

TSRE GenX continues because these complementary bodies of work were shared
with the community.

## License

TSRE GenX is distributed under the [GNU General Public License version 3 or
later](LICENSE.md). Third-party components remain under their respective
licenses.
