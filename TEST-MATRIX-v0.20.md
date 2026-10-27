# TSRE GenX v0.20 Test Matrix

Status: official cap, 27 October 2026, relative to immutable v0.19.
Both operator-run clean Release builds and all registered tests passed. The
operator authorized release after the verified desktop test-copy update.

## Final Build Evidence

| Lane | Clean configure/build | Full CTest | Time | Executable SHA-256 |
| --- | --- | --- | --- | --- |
| MinGW Release | PASS | 34/34 PASS | 9.21 s | `D29E43D6EA3ED52DE957869243FD35917DFA3C1A54D4C77C3A55D04DD4BDFAFB` |
| MSVC Release | PASS | 34/34 PASS | 8.16 s | `C86D58809ABF26BDEDB1BBDC46CAC5729343C0B097AB97F36250E3FFB5CA3250` |

Both executable resources identify 0.20.0.0. The finalized runtime and
distribution use the verified MinGW baseline and its complete
matched runtime. Compiler warnings remain; this is not a zero-warning claim.

## Review and Automated Coverage

- Reviewed all nineteen changed/new WIP C++ source/header files against v0.19,
  plus build registration and root parser-probe changes.
- Cleanup: complete retained dependency graph, optional terrain buffers with
  RAW/elevation/flag protection, exact full TERRTEX families, staged reference
  updates, partial cancellation, net-size measurement and audit reports.
- Route maps: option off/on, nested `terrain_maps` images, category/bytes and
  exact savings; runtime/catalog, RAW and uncertain-dependency protection.
- Copy To: native/compressed models, SD, seasonal ACE/DDS, glTF/GLB resources,
  reuse/conflicts, stale inputs, rollback, working folders and repeat exports.
- Unicode: truncated buffers, isolated surrogates and valid supplementary
  characters; real REF/TRK fields despite quoted/commented lookalikes.
- Shared operation locks protect both cleanup and copying; current save,
  rejected-world, texture, terrain, glTF, PolyVeg and other registered probes
  remain part of the full suites. The original failing Unicode case now passes.
- Frozen document/package/source manifests and public-export checks accompany
  the release. No compiler or source changes followed the verified builds.

## Manual Acceptance and Limits

The operator authorized the official cap and Git publication after working-pack
testing. This records operator acceptance, not an automated claim that every
possible route or failure condition was exercised.

Use a disposable route copy to check selected cleanup/consolidation, unchanged
elevation/flags, reopening/appearance, Copy To resources/settings, working exports,
remembered folders and menu controls with representative local content. Reports
and callbacks cannot certify model pixels, simulator appearance or external data.
External editors do not honor internal locks. Abrupt crashes can leave documented
partial results. Retain editable route backups outside production cleanup copies.

Both build logs and detailed development evidence are retained locally, outside
the public package. Final package checksum is published with the binary ZIP.
