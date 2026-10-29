# TSRE GenX v0.21 Test Matrix

Capped release, 29 October 2026. Baseline: capped v0.20 source a8013066.
Only newly executed v0.21 checks are evidence for this version.

## Release Gate

| Verification | Status |
| --- | --- |
| Operator incremental MSVC Release configure/build | PASS |
| Matched working-pack promotion | PASS; independent executable hashes match |
| Terrain-track CTest including cursor-ray/range/tile cases | PASS 1/1, 0.29 s |
| Focused format/conversion/cleanup CTests | PASS 6/6, 2.46 s |
| Reported tile-autopaint behavior | Operator accepted: "works like a champ" |
| Final clean MinGW Release configure/build and full suite | PASS 34/34, 8.22 s |
| Final clean MSVC Release configure/build and full suite | PASS 34/34, 7.90 s |
| Matched MinGW runtime import audit | PASS - 278 imports resolve to packaged DLLs or Windows |
| Release documents | Synchronized and frozen with SHA-256 manifest |
| Binary/source archive checksums | Recorded in matching SHA-256 companions |

Latest incremental MSVC build and promoted executable SHA-256:
131B1A7E0F824BED6E8CA802E28DE63DF04E413AB2D4ECE547F7CC1C8AF73F99.
This incremental hash is retained for development traceability.

Final clean Release executable SHA-256 hashes:
- MinGW/package: 56A89292A59653749CE722E2ACEEEB8C0719F18E4CA6AD85E2B187A8F4EC6995
- MSVC: 2C087C3CC7902C4BEE0842A0352A0B219726E9C88925CA47A237E8BAA829FD54

Both operator lane logs report successful configure, clean build and complete
CTest suites. Build-input hashes remained unchanged through final verification.
The MinGW package is assembled from current matched Qt/compiler/build binaries,
current assets, launchers and frozen documents; operator runtime state is excluded.

## Format and Cleanup Checks

The rebuilt registered CTests passed on 29 October 2026:
ace_format_validator, read_file_safety, auto_place_wire, route_distro,
forest_bake_manifest and dds_writer (all prefixed tsre_ and suffixed _probe).

- ACE: correct compressed wrapper/expanded-length handling; preserved and
  reader-normalized wrappers; malformed size/tail/truncation rejection.
- Reader integration: actual compressed fixture plus a read-only supplied ACE
  through ReadFile and AceFormatValidator. No production output was written
  from the supplied ACE during this check.
- Worlds: complete trailing Comment blocks, nesting/quotes, multiple comments,
  supported text encodings and compressed wrappers; malformed tails protected.
  Both read-only supplied world files passed cleanup validation.
- Synthetic Pack for Distro: retain referenced assets, remove the selected
  unreferenced asset, and restore conservative retention for malformed footers.
- Existing generated-asset cleanup, conversion/checkpoint/reference publication
  and DDS writer regression cases passed in the same focused run.

Input hashes remained unchanged. Private samples remain local and are not
included in source or packages. Fixture writes initially failed under the
restricted process sandbox; the same binaries passed outside that restriction.

## Tile Autopaint Checks and Coverage

The terrain-track probe covers flat/sloped terrain, invalid/short/upward rays,
unloaded intervals, neighboring and multiple tile offsets, exact boundaries,
and a farther off-axis target. Window shortcuts use the current pointer;
the Texture Panel must be visible and held keys do not repeat.

The operator accepted the reported workflow after the visible-range correction.
That report does not establish separate execution of every HiDPI, focus,
boundary, held-key, Undo, or save/reopen permutation.

## Remaining Manual Coverage

The affected full route was not supplied. Its owner must confirm:

1. Normal production TERRTEX conversion on a disposable route copy, including
   DDS output, mipmaps, terrain references and route reopen/appearance.
2. Full-route Pack for Distro rescan: valid footer worlds no longer cause
   blanket protection; review dependencies and removal candidates before deletion.
3. Wire/generated-asset cleanup and discard retain saved references.

The partial supplied files establish validation compatibility, not complete-route
or simulator acceptance. No performance improvement is claimed.
