# TSRE GenX v0.13 Release Notes

Released as a source checkpoint on 2026-08-19.

v0.13 turns the accumulated Route Editor work into a cohesive production pass.
It remains experimental and actively developed, but it is a full route editor
with maintained build, test, documentation, and compatibility gates.

## Route Editor interface

- Applied the PolyVeg-derived GenX style across remaining properties, helpers,
  Errors & Messages, the F5 map workspace, and utility windows.
- Standardized panel widths, charcoal cards, orange headings and active states,
  black button borders, aligned value columns, compact labels, and tooltips.
- Standardized `...` controls as latching toggles with sound, orange open state,
  second-click close, pinning, snapping, and mutual exclusion.
- Contextual popups close when their owning property panel is no longer active.
- Editable fields select their value on entry for immediate overwrite.

## Object properties and helpers

- Reworked Dynamic Track fields, sections, grade controls, and labels.
- Rebuilt Transform and Height Helper as compact two-column GenX popups.
- Simplified Signal properties, added value tooltips, promoted Subobjects to a
  styled popup, and made signal HACKS context-sensitive.
- Completed the shared treatment across Static, Terrain, Forest, Car Spawner,
  Sound, Transfer, Group, Platform, Siding, Speedpost, Level Crossing, and
  remaining property panels.

## Control Panel and diagnostics

- Cam X/Cam Y and Cam Lat/Cam Lon highlight and copy as displayed pairs.
- Added centered orange `* COPIED *` feedback for two seconds.
- Shortened Marker Location to Location and centered a low-contrast route title
  over the compass lubber line.
- Restyled Errors & Messages, moved it to F11, and added direct removal for
  identified invalid TrackDB/RoadDB items. F11 and F12 toggle normally.

## Terrain and map overlays

- Completed editable experimental 512-sample, 4 m terrain support across
  conforming, Undo, patch outlines, gaps, coordinate math, and rendering.
- Reworked native 2048/4096 overlays with indexed drawing and bounded residency.
- Added explicit Map Tiles controls, removed forced F5 loading, and purge
  overlays when maps are turned off.
- Tightened terrain/overlay retention during long travel and route changes.
- Exit to Main Load now performs a guarded restart after unsaved-work handling.

## PolyVeg and F5 Activity Builder

- Removed redundant Planting Options and Rows subtitles, added Map Tiles, and
  added the non-functional Edit Schema placeholder.
- Preserved deterministic planting, rows, exclusions, status, and hard-commit
  tile/LOD baking from v0.12.
- Refreshed F5 as a map-first workspace with fixed GenX side panels.
- Reworked Path Editor and Path Controls, compacted Activity Editor, shortened
  Jump/Del labels, removed resize controls, and kept Map Tiles user-controlled.

## Track, consist, and cleanup

- Improved projected hit regions so long Consist views select the clicked unit,
  including unresolved stock cards.
- Retained hardened NextGen Dynamic Track validation and route-owned Flex.
- Removed the unfinished activity simulator and unused OpenAL dependency.
- Updated the canonical UI style guide with the v0.13 rules.

## Verification

Clean Qt 6.11.1 Release builds completed on 2026-08-19.

- MinGW 13.1: `C6DDD2BA7C5E68FE7412C1BB98A4457A915ECC409254F412A1FADCE36B121D9F`
- Promoted `TSREvcTST`: identical to MinGW
- VS2026/MSVC v143: `062B82FFFBF945DCB52C617B9010E88C082E97FA180C1BEBF7F0CB96D6FDEE15`
- MinGW CTest: 7/7 passed in 1.60 seconds
- MSVC CTest: 7/7 passed in 1.63 seconds

Coverage includes encoding, DDS, Unicode whitespace, PolyVeg definitions,
bake-manifest cleanup, OSM caching, and the route-regression harness.

## Distribution status

v0.13 is capped as source code only on GitHub. Reviewed source is published to
`tsre-scomod-wip` and tagged `v0.13`. A local MinGW distributable and
checksums are produced, but no binary is uploaded and no GitHub Release is
created. The latest uploaded binary remains v0.12.
