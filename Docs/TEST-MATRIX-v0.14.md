# TSRE GenX v0.14 Test Matrix

v0.14 follows the immutable v0.13 source checkpoint. The operator accepted the
working editor on 2026-08-26 and declared no outstanding v0.14 release testing.
This record distinguishes actual automated/build evidence from accepted future
hardening; it does not convert unrun checks into passes.

## Automated release gate

| Area | Test | MinGW | MSVC | Result |
|---|---|---:|---:|---|
| Terrain | Track/road border-coordinate math | Pass | Pass | Same-tile and cross-X/Z cases pass |
| Terrain | Water-bed clearance math | Pass | Pass | Mask edge, clearance, and idempotence pass |
| Terrain | Terrain-grid transaction math | Pass | Pass | Multi-resolution and rollback cases pass |
| Encoding | MSTS/Open Rails text encoding | Pass | Pass | Compatibility pass |
| Images | DDS decoder | Pass | Pass | Supported layouts pass |
| Parser | Unicode whitespace | Pass | Pass | Community whitespace pass |
| File input | ReadFile malformed/compressed safety | Pass | Pass | Guard and compatibility fixtures pass |
| Images | ACE structural validator | Pass | Pass | Structural fixtures pass; not pixel parity |
| Database | TrackDB/RoadDB validator | Pass | Pass | Counts, references, and numeric guards pass |
| Saving | Route transaction | Pass | Pass | Atomic terrain/key-route cases pass |
| PolyVeg | Definition/generator | Pass | Pass | Catalog and deterministic generation pass |
| PolyVeg | Bake-manifest cleanup | Pass | Pass | Rollback/orphan cleanup pass |
| PolyVeg | Patch baker | Pass | Pass | Self-contained geometry validation pass |
| PolyVeg | OSM cache | Pass | Pass | Cache behavior pass |
| Regression | Route harness self-test | Pass | Pass | Change detection pass |

MinGW passed 15/15 in 2.00 seconds. MSVC passed 15/15 in 2.41 seconds.

## Build evidence

| Lane | Status | Executable SHA-256 |
|---|---|---|
| Qt 6.11.1 MinGW 13.1 Release | Pass | `327612590FF056EB82E6ACBE82A3E64411249A777565FF0839393C35284FBB78` |
| Unpacked v0.14 distribution | Identical | `327612590FF056EB82E6ACBE82A3E64411249A777565FF0839393C35284FBB78` |
| Qt 6.11.1 VS2026/MSVC v143 Release | Pass | `4789690A99D4845DF3645C7CB24E4B19EE8E8EADE57C8BEDECE05CBD93B99C4B` |

The packaged `tsre_assets/templateRoute_0.6` contains 123 files whose hashes
match the immutable v0.12 package manifest. Promotion now refuses a pack that
lacks that required New Route payload.

## Operator acceptance

The v0.14 development cycle included focused use of border conforming, 4 m and
8 m Water Tools, long special rulers, PolyVeg planting/schema/baking, authored
static track and missing-texture rendering, controls, shortcuts, save/reload,
and the versioned working distribution. The operator accepted the resulting
state on 2026-08-26 and explicitly closed v0.14 with no outstanding testing.

## Accepted inherited and future hardening

- The legacy ACE decoder still needs payload-range checks validated against a
  representative real-world parity corpus. The v0.14 structural probe and
  allocation limits reduce risk but do not claim decoded-pixel parity.
- Some legacy world/activity writers do not propagate every possible I/O error
  into the aggregate route-save result. The v0.14 close guard and transactional
  terrain/manifest paths cover the changed high-risk workflows, not every
  inherited writer.
- The MSVC lane retains the established legacy warning backlog. Warnings are
  recorded as cleanup debt; the release gate is zero compiler errors.

These items are not outstanding v0.14 tests and did not block the operator's
source-checkpoint acceptance. They remain candidates for v0.15 hardening.
