# TSRE GenX v0.14 Release Notes

Released as a source checkpoint on 2026-08-26.

v0.14 concentrates the work since v0.13 into production-oriented terrain,
water, PolyVeg, file-safety, and release-engineering improvements. It remains
experimental route-editing software: keep route backups and make significant
changes on disposable copies first.

## Terrain and Water Tools

- Corrected Conform TDB/RDB distance calculations at world-tile borders by
  comparing samples and snapped track positions in one tiled coordinate frame.
- Hardened F2 Size and Intensity editing so blank, intermediate, or invalid text
  cannot silently leave a zero-effect brush while the sliders show another value.
- Completed Water Tools as the F7 special-ruler workflow with explicit New,
  Add Points, Edit Points, Process Water Tiles, Adjust Terrain, and Undo actions.
- Corrected long-ruler selection, arbitrary-distance tile normalization, drag
  and release terrain snapping, Undo ownership, and save/processing ordering.
- Reworked water-mask coverage and shoreline handling for native 4 m and 8 m
  terrain, including shared corners, bounded traversal, mixed-grid rejection,
  resolution-independent frontier sampling, and idempotent bed clearance.
- Added transactional multi-file terrain saves so failed height, flag, metadata,
  or seasonal texture writes restore earlier components and retain dirty state.

## PolyVeg

- Added the full-screen PolyVeg Schema Editor for route-local catalogs, asset
  search, static thumbnails, recipe defaults and limits, and atomic validation.
- Simplified schema authoring, standardized increments and displayed precision,
  and preserved compatibility with older catalog keys.
- Added transactional bake ownership: Discard/Quit restores replaced generated
  files and manifest state, while successful route Save commits them.
- Hardened baking and cleanup against malformed geometry, manifest failures,
  unreadable world files, orphan-only legacy assets, and failed staged deletes.
- Made the baker probe self-contained and registered it with CTest.

## Editing, rendering, and interface

- Remapped Auto Place to F5, PolyVeg to F6, Schema Editor to Shift+F6, Water
  Tools to F7, and Activity Builder to F10; plain M toggles saved map overlays.
- Standardized application checkboxes and spin controls and retained the compact
  GenX panel language across legacy and modern windows.
- Corrected Dynamic Track TrackDB guide transforms on graded curves.
- Restored authored static-track shapes instead of the incomplete procedural
  replacement, preserving ShapeTemplate tokens for file compatibility.
- Corrected missing/loading/failed shape textures so the active legacy renderer
  draws the standard magenta material instead of sampling stale terrain state.
- Added responsive desktop sizing, clearer selection labels, and focused Schema
  Editor and special-ruler workflow refinements.

## Reliability and validation

- Hardened ReadFile handling for missing, short, oversized, truncated, and
  failed-decompression inputs and removed the obsolete remote bootstrap path.
- Restored the version-matched `templateRoute_0.6` payload to the local package
  from its verified v0.12 manifest and corrected promotion so a runtime-only
  asset directory is retained and explicitly required instead of deleted.
- Added strict TrackDB/RoadDB structural validation and safer geometry mutation,
  including checked node/item references and interactive-object edit guards.
- Made close-time Save fail closed when the route reports failure and repaired
  PolyVeg manifest rollback and cleanup accounting.
- Added ACE dimension/allocation safeguards and a structural validator probe.
  Complete pixel-payload bounds checking remains inherited compatibility debt;
  enabling it safely requires a representative real-world ACE parity corpus.
- Fixed thumbnail decoded-pixel ownership and several terrain/map cache cleanup
  paths.

## Verification

- Qt 6.11.1 MinGW 13.1 clean Release build: pass.
- MinGW registered CTest suite: 15/15 passed in 2.00 seconds.
- MinGW build and unpacked distribution executable SHA-256:
  `327612590FF056EB82E6ACBE82A3E64411249A777565FF0839393C35284FBB78`.
- Qt 6.11.1 VS2026/MSVC v143 clean Release build: pass; 15/15 CTests
  passed in 2.41 seconds; executable SHA-256
  `4789690A99D4845DF3645C7CB24E4B19EE8E8EADE57C8BEDECE05CBD93B99C4B`.
- The operator accepted the v0.14 editor state with no outstanding release
  testing. Recommended future checks and inherited hardening are not represented
  as completed tests or v0.14 blockers.

## Distribution status

v0.14 is capped on `tsre-scomod-wip` with immutable tag `v0.14`. Complete local
binary and source packages plus SHA-256 companions are produced for archival,
but no v0.14 GitHub Release is created and no binary is uploaded. The latest
uploaded binary release therefore remains v0.12.
