# TSRE GenX v0.16 — 4 m Terrain Texture Hotfix and Full Release

TSRE GenX is a 64-bit Windows route editor for Microsoft Train Simulator and
Open Rails routes. It combines the established TSRE editor with Qt 6/CMake,
expanded terrain and map workflows, Dynamic Track, PolyVeg, route diagnostics,
and a consistent GenX interface.

> **Current release: v0.16.** This is a full Windows release led by the
> critical 4 m terrain-texture persistence hotfix. The published v0.15 release
> remains immutable as the prior-release baseline.

## Major Updates Since v0.15

### Critical 4 m terrain-texture hotfix

- Corrected the coordinate domain stored in terrain texture matrices for
  native 512-sample/4 m terrain. GenX and Open Rails now consume the same
  resolution-aware transforms that GenX saves, eliminating the former
  renderer-only compensation.
- Applied the correct domain to default/reset transforms, arbitrary rotation,
  mirror operations, scale reporting, route-wide TERRTEX reset, and generated
  whole-tile map textures for R=8, R=16, and R=32 terrain patches.
- Preserved established 256-sample/8 m R=16 constants bit-for-bit.
- Added conservative normalization for clear non-R16 fixed-16 signatures from
  the former workaround. Affected terrain is marked modified so route Save can
  persist the correction; ambiguous custom-scaled records are not guessed.
- Corrected rotation-independent texture scale measurement so 45-degree
  matrices retain their true scale and cannot feed a zero divisor to a later
  scale edit.
- Expanded the registered terrain-grid probe across R=8/16/32 transforms,
  whole-tile map scale, legacy conversion, nonconversion safeguards, and
  rotation-independent scale measurement.

### Terrain and editor identification

- Added a read-only `Mesh Res` field to Terrain Object Properties. Supported
  detailed grids are identified as `8m Normal` or `4m High Def`; distant and
  nonstandard layouts report their actual spacing without being mislabeled.
- Added Shift+M as the keyboard equivalent of View > Hide Terrain Shape. When
  terrain is hidden, a visible saved-route map is temporarily hidden and then
  restored with the terrain; menu state and F12 key help remain synchronized.

### Track, grade, and object workflow

- Track changes and deletions can proceed while interactives remain attached.
  Surviving TrackDB/RoadDB item positions are adjusted and items on removed
  geometry are deleted with the section instead of blocking the edit.
- Standardized grade displays and inputs: two decimals for percent, one for per
  mille, two for 1-in-X, and three for degrees. Dynamic Track, Track
  Properties, Ruler, Selection Info, Grade Helper, and Lock Grade use the same
  presentation without changing stored physical-grade formulas.
- Plain E/Q/R/T/Y now activate Select, Place, Rotate, Translate, and Size
  idempotently. Repeating a mode key no longer turns that mode off; special
  Water and PolyVeg ruler ownership remains protected.
- Reordered the F1 Ref selector so FULL DATABASE, NEXGEN TRACK, and TSRE TOOLS
  appear first, and added numeric-aware track-category ordering so A10 follows
  A9.

### PolyVeg and Path Editor refinements

- Added a dedicated read-only properties panel for generated PolyVeg bake
  groups. It shows managed-object guidance, block and shape counts, tile
  coordinates, and the shared HACKS action without exposing movement,
  rotation, flags, detail, or random-transform controls.
- Corrected Path Editor serialization when consecutive junctions have multiple
  direct vector connections. Neutral vector anchors preserve the exact main or
  passing route selected in the editor while older PAT files retain their
  established ordered-pin fallback.

### Source stewardship

- Added consistent TSRE GenX ownership, lineage, and GPL notices only to
  first-party editor and regression-probe files created from scratch that
  previously lacked a complete header. Piotr's headers, inherited source, all
  third-party code, vendored OpenAL headers, and generated files remain
  unchanged.
- Preserved the complete v0.15 tag, release assets, frozen public documents,
  and source snapshot as an immutable prior release.

## Installation

1. Download **`tsre-scomod-v0.16.zip`** and its `.sha256` companion from this
   release. GitHub's automatic “Source code” archives are source only and are
   not the runnable editor package.
2. Verify the ZIP against the published SHA-256 companion.
3. Extract the **complete archive** to its permanent writable location. Do not
   run the application from inside the ZIP and do not copy only `TSRE5.exe`
   into an older release folder.
4. Run **`AddShortcutDesktop.cmd`** from the extracted folder.
5. Launch through the generated **TSRE GenX** shortcut. Move that shortcut
   wherever desired only after it has been created.
6. Keep the extracted application folder intact. The shortcut targets that
   folder, and the executable, Qt runtime, plugins, assets, and documents must
   remain together.

Back up a route before major terrain conforming, 4 m terrain work, water
processing, route-wide saves, or PolyVeg baking. Test high-risk workflows on a
copy before committing them to a production route.

## Requirements and Limitations

- 64-bit Windows 10 version 1809 or later, or Windows 11.
- Existing Microsoft Train Simulator or Open Rails route content for editing.
- Qt 6.11.1 for source builds. The MinGW 13.1 build is the parity/release
  baseline; MSVC v143 is the independent verification lane.
- PolyVeg consumes route-local geodata derivatives produced by SCO LIDEX.
- Windows 7 and Qt 5/qmake builds are not supported.
- GenX remains an actively developed editor. Maintain route backups and verify
  simulator behavior after significant terrain, track, water, path, or
  vegetation changes.

## Verification

The operator accepted the complete v0.16 functional test matrix. Final clean
MinGW and MSVC Release builds each passed all 15 registered CTest tests. The
MinGW release executable SHA-256 is
`3CDA231C4FCE074D26EAE8264BE53F28D21F2E5195D53F9F4820B4C1FAC62BBE`; the
independent MSVC verification executable SHA-256 is
`AE3BDA8ED28ACE6EAC7B9F207121B2CA61F6B9DA91D31642EF64A5748DB68204`.
Complete timings and gate evidence are frozen in `TEST-MATRIX-v0.16.md`.

## Source and Documentation

The `v0.16` tag will identify the exact reviewed source for this release. Use
the release ZIP for the runnable Windows editor and GitHub's source archives or
the tagged repository tree for source. Verify the binary ZIP with the published
SHA-256 companion; the package and frozen public-document manifests provide
file-level verification inside the release.

The distribution includes the versioned release notes and test matrix plus the
complete terrain, TERRTEX, PolyVeg, Water Tools, Dynamic Track, Grade Helper,
and Path Editor/Full Map user guides.

## Acknowledgments

Piotr Gadecki (GokuMK) created TSRE5 and its core route, terrain, texture,
activity, consist, file, and rendering systems. His later TSRE5vc work also
provided independent Qt 6 and compressed-texture references.

Eric Olesen (eric-from-trainsim) sustained the TSRE 8.006 line that directly
precedes GenX and provided its principal compatibility and source/API
reference.

Peter Gronbaek Andersen produced an independent Qt 6/CMake port whose build,
dependency, deployment, and DDS work supplied important reviewed references.

## License

TSRE GenX is distributed under the GNU General Public License version 3 or
later. Third-party components remain under their respective licenses; see
`LICENSE.md` and `THIRD-PARTY-NOTICES.txt`.
