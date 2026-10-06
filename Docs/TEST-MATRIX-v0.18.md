# TSRE GenX v0.18 Test Matrix

Status: full v0.18 release, 6 October 2026. Published v0.17 is the
immutable baseline. The operator accepted all remaining manual checks on 6 October 2026. The immutable v0.18 tag identifies the accompanying source.
Earlier successful checks apply only to the sources/binaries actually tested.

## Automated and Build Evidence

| Check | Current evidence / remaining gate |
|---|---|
| Version identity | CMake/vcpkg 0.18.0, Game v0.18 and Windows resource 0.18.0.0 aligned |
| Routine MinGW Release builds | Operator builds reviewed during development; final clean candidate build PASS on 6 October |
| Forest definition/replacement/baker/manifest/save cleanup | Updated probes passed 5/5 on 5 October after forest review cleanup (0.52 s) |
| Reviewed forest build/runtime hash | Both D9F9D95F1822649E34C4C549BAA87C68C368B6C83DDCA236FC6AA78FDD249067 at that gate; this is not the final candidate hash |
| glTF CPU/GPU probes | Hardening Debug probes passed on 5 October: model 0.32 s, graphics 0.93 s in both GL profiles; final Release probes PASS in both lanes |
| PolyVeg visibility | Source/render-filter review complete; final integration compiled in both lanes; operator manual acceptance PASS |
| Wire exit prompt | Prior wire registry probe passed; final prompt integration compiled; operator manual acceptance PASS |
| Targeted wire deletion | New discovery/cleanup probe cases added; rebuilt probe PASS in both lanes; operator manual acceptance PASS |
| Release-prep focused existing binaries, 6 October | Debug graphics/model/encoding PASS; Release forest and wire probes PASS 6/6. Sandbox model fixture-write failure resolved by repository-contained outside-sandbox retry; these runs do not validate today's editor cleanup or replace final full-suite builds |
| MinGW final clean Release configure/build/full CTest | PASS: clean configure/build and 28/28 tests (4.82 s); reviewed AAA_Publish-v0.18-mingw-release.log |
| MSVC final clean Release configure/build/full CTest | PASS: clean configure/build and 28/28 tests (5.00 s); reviewed AAA_Publish-v0.18-msvc-release.log |
| Final executable hashes / dependency audit | PASS: both log hashes independently matched; 29 runtime DLL/plugin hashes match installed Qt 6.11.1/MinGW 13.1, all recursive imports resolved |
| Final promoted runtime / package checksum / document manifest | Prepared from the verified MinGW executable and matched runtime; final ZIP/checksum and document manifest accompany local candidate |

## Final Executable Hashes

- MinGW: `B30846A2561AE98BA602098C744179E51D56E152D198973F514279E24992B4F9` (9,838,915 bytes).
- MSVC: `5330279CFDFC27FDC0A5756838AB093D339F0736809E6755AE380162427560F9` (5,684,224 bytes).
- Both executable resources identify 0.18.0.0. Compiler warnings remain; no zero-warning claim.

## Manual Editor Acceptance

The operator accepted **all remaining manual checks** on 6 October 2026. The checklist below records the acceptance scope; this is operator evidence, not automated coverage. The package uses the final matched MinGW build.

| Workflow | Evidence / required check |
|---|---|
| Forest replacement success | Operator reports successful complete route replacement. Confirm normal Save/reopen retains generated bakes and zero original Forest regions, including originals outside terrain coverage |
| Forest preflight | Validate clipped missing coverage, fatal unreadable existing terrain and conflict refusal; ordinary scenery must remain |
| Forest cancellation | Cancel during generation, placement and bake; originals and previous assets remain, replacement output rolls back. Placement-stage interaction accepted by operator |
| Empty replacement | All-empty/entirely uncovered plans cannot commit; mixed valid/empty tiles can complete |
| PolyVeg visibility | Hide/show raw, baked and schema-listed vegetation, shadows and bake markers. Hidden objects cannot be picked; ordinary scenery and Forest Region visibility remain independent |
| Visibility persistence | Save/reopen while hidden retains vegetation; new session starts visible |
| glTF/GLB baseline | Basic GLB placement, Save/reopen and corrected test-model OR display accepted by operator. This does not claim animation or full material parity |
| glTF editing | Both containers: selection/cutouts, bounds, translation/rotation/scale, clone, Undo/Redo, reload and context recreation; verify legacy .s coexistence |
| Pickup models | glTF/GLB placement, interactive-marker selection, alignment, capacities, Save/reopen and legacy .s parity; rest-pose preview only |
| Unsupported models | Explicit rejection/warnings, missing-resource diagnostics and unchanged source assets; DDS-only, visibility/LOD and animation playback are outside preview support |
| Exit prompt | Baked-only changes show Save & Quit; raw wires/PolyVeg show Bake & Save. Cancel retains work; Discard protects previously saved/shared assets |
| Wire duplicate protection | Save & Quit, reopen, then Commit/Bake/Save the same run; placement count remains unchanged |
| Route-wide wire deletion | Progress stays responsive; only wire candidate tiles are loaded. Poles/vegetation remain; saved references retain assets until Save; shared textures survive if used |
| Wire cleanup retry | Repeat Delete All, pending-cleanup reopen and new Commit/Bake before Save; new spans survive cleanup. Malformed worlds stop discovery before removal |
| Startup and compatibility smoke | Route Editor, Shape Viewer, Consist Builder, expected assets/version, representative route Save/reload and legacy shapes |

Final clean compiler suites and operator manual acceptance passed. The package uses the verified MinGW runtime. Detailed development logs and private route evidence remain local; the public document manifest, package manifest and ZIP checksum identify the distributed files.
