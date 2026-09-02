# TSRE GenX v0.16 Test Matrix

Status: release-candidate functional matrix accepted by the operator. All
applicable v0.16 manual checks are satisfactory. The final clean MinGW/MSVC
Release builds and both complete registered CTest suites passed on 2026-09-02
after the last texture-scale correction and source-header pass.

## Release identity and inherited baseline

| Gate | Status | Evidence |
|---|---|---|
| v0.15 release cap | Pass; immutable | Published v0.15 tag, binary assets, frozen public documents, and external source snapshot remain unchanged |
| v0.16 source starting point | Pass | Exact 740-file frozen v0.15 public source copied into the external `v0.16-starting-point` snapshot; archive SHA-256 `2925A76706BAC6D19189BBCC4D3F5BF3E0B2C76AB4DE4EFDEDA3FB3416AD1416` |
| Version identity alignment | Static pass | CMake, `Game::AppVersion`, vcpkg, Windows resource metadata, launchers, test defaults, and active house rules identify v0.16/0.16.0 |
| Working TST seed | Pass | `dist\tsre-scomod-v0.16` contains the complete inherited MinGW runtime and current assets |
| First v0.16 MinGW build | Pass | Clean opening compile promoted an 8,956,070-byte executable; build/TST SHA-256 `D0A2A282CF94A87071906A9B85FD5F074EBE90CA1699E0F57F5A3E35A6D90925` |

## v0.16 functional verification ledger

| Area | Change or gate | Automated/static result | Manual result | Evidence |
|---|---|---|---|---|
| 4 m terrain-texture hotfix | Resolution-aware saved matrices for R=8/16/32; default/reset/rotation/mirror/scale/map paths; conservative legacy fixed-16 normalization | Pass in both final clean Release lanes; terrain-grid probe includes the rebuilt rotation-independent scale cases | Pass — operator reports the complete 4 m/8 m texture matrix satisfactory, including save/reload and simulator-facing behavior | Snapshot `terrain-uv-matrix-before-20260831`; final CTest 2026-09-02 |
| Texture scale at arbitrary rotation | Measure matrix-axis magnitude instead of summing components, preventing 45-degree cancellation and later division by zero | Pass in both final clean Release lanes at 0°, 45°, 90°, and invalid base scale | Covered by accepted terrain-texture workflow; no separate serialization change | Final CTest 2026-09-02 |
| PolyVeg bake properties | Dedicated read-only all-bake tile-group panel; generic Group/Static edit and movement commands excluded; shared HACKS retained | First implementation compiled; group-aware correction source reviewed; no focused automated UI probe exists | Pass — corrected group panel, mixed group, raw PolyVeg, and ordinary Static behavior accepted | Snapshots `polyveg-bake-properties-v0.16-20260828` and `polyveg-bake-group-properties-v0.16-20260829` |
| Track section removal | Track changes/deletions proceed with attached interactives; surviving positions adjust and items on removed geometry are deleted | Static review and source whitespace checks pass; both final clean Release lanes compile | Pass — first/middle/last/complete section removal, no guard popup, save/reload accepted | Snapshot `track-interactive-guard-removal-v0.16-20260831` |
| Grade precision | Percent, per-mille, 1-in-X, and degree displays use consistent precision across Track, Dynamic Track, Ruler, Selection, Grade Helper, and Lock Grade | Static review pass; physical-grade formulas unchanged; both final clean Release lanes compile | Pass — displays, edits, unit switching, placement, and save/reload accepted | Snapshots `grade-display-precision-v0.16-20260831` and `dyntrack-grade-display-before-20260901` |
| Editor mode keys | Plain E/Q/R/T/Y activate Select/Place/Rotate/Translate/Size idempotently while special-ruler ownership remains protected | Static control-flow review pass; both final clean Release lanes compile | Pass — repeated keys, cross-mode transitions, panel toggles, and Water/PolyVeg ruler behavior accepted | Snapshot `qe-keyboard-enable-before-20260901` |
| Terrain mesh identity | Read-only Mesh Res field identifies supported detailed, distant, and nonstandard layouts from actual metadata | Static review pass; display-only; both final clean Release lanes compile | Pass — 8 m Normal, 4 m High Def, distant Mosaic/TSRE, and nonstandard labeling accepted | Snapshot `terrain-mesh-res-field-before-20260901` |
| F1 object categories | FULL DATABASE, NEXGEN TRACK, TSRE TOOLS lead Ref selector; track categories sort numerically | Static review pass; both final clean Release lanes compile | Pass — special-list population and A9/A10 order accepted | Snapshot `f1-category-natural-sort-v0.16-20260831` |
| Path Editor serialization | Neutral vector anchors preserve selected parallel main/passing connections while retaining legacy fallback | Static review and source whitespace checks pass; both final clean Release lanes compile | Pass — save, close/reopen, parallel-route retention, PAT inspection, and Open Rails behavior accepted | Snapshot `path-parallel-vector-anchor-v0.16-20260901` |
| Terrain-shape shortcut | Shift+M mirrors View action and temporarily hides/restores a visible saved map without affecting M or N | Static shortcut/help synchronization pass; both final clean Release lanes compile | Pass — map initially Off/On, auto-repeat, menu check, M, N, and F12 behavior accepted | Snapshot `20260901-n-terrain-map-toggle` |
| Source headers | LIDEX-style GenX ownership/license notice only on first-party files created from scratch | Audit pass: Piotr headers, inherited/third-party code, vendored OpenAL, and generated files unchanged | N/A | Release-preparation audit 2026-09-02 |
| Existing focused CTest evidence | Terrain grid, terrain track, and route-save transaction probes | Pass 3/3 against the latest existing MinGW tree; route-save fixture required normal temporary-directory access | N/A | Focused CTest 2026-09-02 |
| Final dual-lane publication gate | Clean MinGW and MSVC Release configure/build plus complete registered CTest suites | **Pass** — MinGW 15/15 in 2.09 s; MSVC 15/15 in 1.95 s; zero compiler errors; warnings retained in lane logs | N/A | MinGW `TSRE5.exe`: 9,006,025 bytes, SHA-256 `3CDA231C4FCE074D26EAE8264BE53F28D21F2E5195D53F9F4820B4C1FAC62BBE`; MSVC: 5,126,144 bytes, SHA-256 `AE3BDA8ED28ACE6EAC7B9F207121B2CA61F6B9DA91D31642EF64A5748DB68204` |

## Publication acceptance

The final dual-lane build/test gate is accepted. v0.16 may proceed to freeze,
package, tag, `master` promotion, push, and publication. The package manifest,
ZIP checksum companion, source-snapshot record, public commit, and peeled tag
target are retained with the release evidence rather than self-embedded in the
checksummed package matrix.
