# TSRE GenX v0.22 Test Matrix

Verification recorded 9 November 2026. Evidence is fresh for v0.22.

| Check | MinGW Release | MSVC Release |
| --- | --- | --- |
| Configure and clean application/test build | PASS | PASS |
| Full registered CTest suite | 39 PASS; zero skipped | 39 PASS; zero skipped |
| Enabled reference corpus | PASS, 68/68 exact | PASS, 68/68 exact |
| Rebuilt TERRTEX processing/resume probe | 1,171 checks; zero failures | 1,171 checks; zero failures |
| Overall registered-test coverage | All 39 passed in full suite | All 39 passed in full suite |

## Executable Identity

- MinGW SHA-256: `4316CC24E532BE01BF2CE0E7C817B10633E17CC7C916CE3A65231ED1BA740F5A`.
- MSVC and distributed executable SHA-256:
  `B4924CE167AE3195E68833F47D2E0A88C7C4CE0817533208291E7FB2E1A28106`.
- The capped MSVC working/package executable matches the final verified build. All 23 Qt
  DLL/plugin hashes match its configured compiler-specific Qt installation.
- The package contains a complete MSVC runtime and no MinGW runtime DLLs.

## Manual Acceptance

The operator accepted the refreshed functional runtime and the requested manual TERRTEX
conversion/reference/seasonal/save/reopen workflow on 9 November 2026. The final
source cleanup changed whitespace only; both final clean builds passed afterward. Route
painting and final processing preserve native dimensions; format/size choices
are absent from route autopaint, and processing has no size choice. Processing
options use Title Case.

## Automated Scope and Limits

The processing probe includes both conversion directions, mip choices, alpha,
compressed-block retention, malformed/unsupported inputs, collisions, seasonal
references, interruption/resume and deletion after verified publication.
The new edited-original cases prove that resume stops before the reference
updater, preserves originals/references/replacements and permits a valid resume
when the original fixture is restored. This is automated fixture coverage, not
an exhaustive simulation of every external editor or storage failure.

The full suites also cover route-save transactions, file encoding, terrain math,
world/TrackDB parsing, DDS decoding/writing, route painting, vegetation ownership,
wire placement and other registered regression probes. Existing compiler
warnings remain; warning-free builds and universal route/simulator coverage are
not claimed. Internal logs and personal route evidence are not distributed.
