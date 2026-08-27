# TSRE GenX v0.15 Test Matrix

v0.15 is the active corrective development line after the withdrawn v0.14 tag.
This matrix records the completed working-pack checks and clean dual-compiler
gate used for the branch-and-tag source safety push.

## Automated release gate

| Area | Registered CTest | MinGW | MSVC |
|---|---|---:|---:|
| Terrain | Track/road border-coordinate math | Pass | Pass |
| Terrain | Water-bed and PolyVeg water-clearance math | Pass | Pass |
| Terrain | Terrain-grid transaction math | Pass | Pass |
| Encoding | MSTS/Open Rails text encoding | Pass | Pass |
| Images | DDS decoder | Pass | Pass |
| Parser | Unicode whitespace | Pass | Pass |
| File input | ReadFile malformed/compressed safety | Pass | Pass |
| Images | ACE structural validator | Pass | Pass |
| Database | TrackDB/RoadDB validator | Pass | Pass |
| Saving | Route transaction | Pass | Pass |
| PolyVeg | Definition/generator | Pass | Pass |
| PolyVeg | Bake-manifest cleanup and binary-world retention | Pass | Pass |
| PolyVeg | Patch baker | Pass | Pass |
| PolyVeg | OSM cache | Pass | Pass |
| Regression | Route harness self-test | Pass | Pass |

## Build evidence

| Lane | Status | Executable SHA-256 |
|---|---|---|
| Qt 6.11.1 MinGW 13.1 Release | Clean configure/build; 15/15 tests passed in 1.93 seconds | `CCA4F213A983DB840C97ED5B004634C046387468C95F3D20D39C538964CA5810` |
| Unpacked v0.15 working distribution | Exact final MinGW executable promoted and hash-verified | `CCA4F213A983DB840C97ED5B004634C046387468C95F3D20D39C538964CA5810` |
| Qt 6.11.1 VS2026/MSVC v143 Release | Clean configure/build; 15/15 tests passed in 1.94 seconds | `3BEC2AC6895CEFDFFADDFBD12F55FA76205F25ADFD33D0F544812C1DB7583DBA` |

The complete combined-output records are
`AAA_PushGit-v0.15-mingw-release.log` and
`AAA_PushGit-v0.15-msvc-release.log`. Review found no compiler or linker error,
no test failure, and no recurrence of the corrected `layoutError` or baker-loop
warnings. The logs remain local and are excluded from public source and binary
packages.

## Completed working-pack evidence

- The corrected binary-world manifest and adjacent route transaction probes
  passed after rebuild. A subsequent incremental MinGW Release suite passed
  15/15, and later focused review passed the ACE, route transaction, forest
  definition, bake manifest, and patch baker probes 5/5.
- The operator loaded, inspected, saved, and reloaded the recovered route with
  corrected v0.15 code and confirmed its non-PolyVeg work remained intact.
- The operator subsequently confirmed schema-controlled submerged-water
  clearance and setback behavior, bake viewport blanking, save, reload, and
  general editor operation are working well.

These results supplement the completed clean dual-compiler publication gate
above.

## Accepted inherited hardening

- Complete legacy ACE pixel-payload validation remains blocked on a
  representative parity corpus; current allocation and structural guards remain
  active.
- Some inherited world/activity writers do not propagate every possible I/O
  failure into the aggregate route-save result. The corrected PolyVeg ownership
  rule prevents a later Discard from deleting possibly referenced bake assets.
- The MSVC lane retains its established legacy warning backlog. Zero compiler
  errors, not zero historical warnings, is the release requirement.

## Publication disposition

The remote v0.14 tag was withdrawn; its exact annotated tag remains local. The
v0.15 final build gate is complete and the active source is approved for a
direct branch push and annotated `v0.15` source safety tag. Development remains
open after that commit; no frozen binary release, GitHub Actions run, binary
upload, or GitHub Release is authorized.
