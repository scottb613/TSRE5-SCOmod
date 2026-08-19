# TSRE GenX v0.13 Test Matrix

v0.13 follows the immutable v0.12 source checkpoint.

## Automated release gate

| Area | Test | MinGW | MSVC | Result |
|---|---|---:|---:|---|
| Encoding | MSTS/Open Rails text encoding | Pass | Pass | Compatibility passed |
| Images | DDS decoder | Pass | Pass | Supported formats passed |
| Parser | Unicode whitespace | Pass | Pass | Community whitespace passed |
| PolyVeg | Definition/generator | Pass | Pass | Deterministic generation passed |
| PolyVeg | Bake-manifest cleanup | Pass | Pass | Generated cleanup passed |
| PolyVeg | OSM cache | Pass | Pass | Geodata cache passed |
| Regression | Route harness self-test | Pass | Pass | Change detection passed |

MinGW passed 7/7 in 1.60 seconds. MSVC passed 7/7 in 1.63 seconds.

## Build evidence

| Lane | Status | Executable SHA-256 |
|---|---|---|
| Qt 6.11.1 MinGW 13.1 Release | Pass | `C6DDD2BA7C5E68FE7412C1BB98A4457A915ECC409254F412A1FADCE36B121D9F` |
| Promoted TSREvcTST | Identical | `C6DDD2BA7C5E68FE7412C1BB98A4457A915ECC409254F412A1FADCE36B121D9F` |
| Qt 6.11.1 VS2026/MSVC v143 Release | Pass | `062B82FFFBF945DCB52C617B9010E88C082E97FA180C1BEBF7F0CB96D6FDEE15` |

## Manual evidence completed during development

| Area | Status | Evidence |
|---|---|---|
| GenX styling | Pass | Operator reviewed Control Panel, helpers, properties, PolyVeg, and F5. |
| Popup lifecycle | Pass | Latching, pins, snapping, exclusion, and panel-change closure corrected. |
| Field overwrite | Pass | Global select-on-entry corrected after Grade and Transform checks. |
| Control Panel copy | Pass | Paired hover/copy and copied confirmation reviewed. |
| F5 Activity Builder | Pass | Side panels, labels, map ownership, and fixed widths reviewed. |
| F11/F12 | Pass | Styled windows and toggle behavior reviewed. |

## Focused checks before production-route use

| Area | Status | Manual verification |
|---|---|---|
| Route saves | Recommended | Save/reload a copied route after scenery, track, terrain, and PolyVeg edits. |
| Native 4 m terrain | Experimental | Check conform, Undo, gaps, seams, save/reload, and simulator display. |
| Residency | Recommended | Travel beyond map radius with Map Tiles on/off; confirm purge and settled memory. |
| Error deletion | Recommended | Delete an identified invalid item on a copy and confirm save/reload. |
| Main Load | Recommended | Confirm prompts, restart, reopen, and clean helper/overlay state. |
| Activity paths | Recommended | Create, edit, save, reload, reverse, wait, and passing controls. |
| PolyVeg commit | Recommended | Plant, bake, save, reload, delete, and verify cleanup on a copy. |

v0.13 remains experimental. Back up routes and use disposable copies for these
focused checks.
