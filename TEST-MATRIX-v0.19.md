# TSRE GenX v0.19 Test Matrix

Status: full release, 26 October 2026, relative to immutable v0.18.
Both operator clean Release builds passed all 32 tests; final manual editor
checks were accepted by the operator. Development passes remain historical.

## Automated and Build Evidence

| Check | Evidence / remaining gate |
|---|---|
| Version identity | Static PASS: CMake/vcpkg 0.19.0, Game v0.19, resource 0.19.0.0 |
| Source inventory | Reviewed against immutable v0.18: 75 changed/new code and regression files, plus build registrations and operator/export scripts |
| Existing production audit | Save staging/rollback, texture boundaries/cache, encoder inputs, glTF allocation budgets and cleanup reviewed; final builds passed |
| Source whitespace | Changed existing code passes snapshot/baseline-relative checks; complete public export whitespace check is part of the verified release gate |
| Focused development probes | Existing MSVC binaries PASS 14/14 on 26 October: 11 core texture/glTF/PolyVeg/AP/encoding probes in 3.73 s, plus 3 save/ownership probes in 0.41 s. These do not replace final rebuilt suites |
| MinGW final clean configure/build/full CTest | PASS: operator clean build/full suite, 32/32; log and executable hash independently reviewed |
| MSVC final clean configure/build/full CTest | PASS: operator clean build/full suite, 32/32; log and executable hash independently reviewed |
| Current-source regression coverage | Registered texture output/mips/DXT1, save transaction, glTF CPU/GPU, PolyVeg geometry/ownership, AP ruler/wires, parser/encoding and existing probes; compiled and passed in both final Release lanes |
| Final executable hashes | PASS: final MinGW and MSVC hashes recorded below; resources identify 0.19.0.0 |
| Runtime/dependency and package audit | PASS: matched MinGW runtime promoted, 35 DLL/plugin files with recursive imports resolved; package/document manifests and ZIP checksum accompany release |
| Source export and publication | Reviewed public export, immutable v0.19 source tag and exact source snapshot; publication evidence retained in the local release record |


## Final Executable Hashes

- MinGW: `85A426A788C4798AFD4C00D21201651A21C0D5AB42AEA7BFF6EB4239DFBF8197` (10,238,740 bytes).
- MSVC: `22B734D7836AF3163C4C6B3FB7021B51EFABD7AF405FB718E6A1362F61559C3D` (5,919,232 bytes).
- Both resources identify 0.19.0.0. Compiler warnings remain; no zero-warning claim.
- The distribution uses the verified MinGW executable and compiler-matched runtime.

## Manual Editor Acceptance

The operator accepted the final manual editor checks on 26 October 2026.
The rows below record that acceptance scope, not independent automated proof
of every visual edge case or exhaustive Open Rails compatibility.

| Workflow | Manual acceptance scope |
|---|---|
| Startup and compatibility | Operator acceptance: Route Editor, Shape Viewer, Consist Builder, version, assets, restore session and legacy-shape controls |
| Texture presets | Operator acceptance: selecting applies immediately, no Apply button; missing/invalid source reports clearly; settings/paint tool/preview agree |
| Texture scale/output | Operator acceptance: rotation/scale, RGB/RGBA rectangular edges, Undo, ACE/RGB24 DDS, mip toggles, original 2048 base resolution and seasonal companions after save/reopen |
| Shape seasonal lookup | Operator acceptance: DDS priority per directory, Snow/WinterSnow vegetation, seasonal ACE before base DDS, missing-file fallback |
| Save progress and failure | Operator acceptance: bake + texture + publication + cleanup total progress, 100% only on success, disk staging, failure/rollback and saved generated references |
| Production DXT1 | Operator acceptance on a final upload copy: all seasonal ACE/DDS, opaque output/dimensions, reference updates, progress, interruption/resume, once-only warning/refusal and no retained TERRTEX originals/backups |
| Context-sensitive HACKS | Operator acceptance: terrain, track, signal, vegetation and static selections expose appropriate actions; destructive confirmation scopes remain correct |
| Ruler (ap) | Operator acceptance: F1 launcher opens F5 Objects/Follow, half-height handles/picking, latch/second-click clear, Add/Edit/Clear, cross-tile points, corner preservation, smooth ordinary headings, offsets and terrain following |
| Ruler placement lifecycle | Operator acceptance: successful placement clears/resets, partial retry does not duplicate, Undo restores, helper omitted from saved world files |
| Wire discovery and appearance | Operator acceptance: existing SNAP ruler poles away from track/road, Plain control, short/thick blunt brown Barbed, thin/default/thick width, straight alpha strips, seam/mip/oblique views and attachment alignment |
| Wire ownership | Operator acceptance: repeated Commit/Bake/Save, delete/rebake new artwork, old texture versions retained while referenced, disconnected/parallel lines reviewed, limits and cleanup/save/reopen |
| PolyVeg panels/OSM | Operator acceptance: compact alignment, no obsolete Jump controls, shared F5/F6 settings, OSM-off local planting without derivatives and OSM-on mapped coverage/exclusions |
| PolyVeg planting | Operator acceptance: ruler/area/full tile/corridor sides-setback-width-range, one-click arm/cancel, terrain/database/water/slope guards, duplicate area, zero-result report/copy/suppression, bake/Undo/save/reopen |
| glTF/GLB consists | Operator acceptance: bodies/freight with legacy controls, readable artwork, direction/Flip, offsets, 4K textures, transparency/picking, roster redraw and context reopen; rest pose only |
| Unicode activity paths | Operator acceptance: German umlaut/sharp-s metadata/create/clone, service/activity references, exact PAT/SRV/ACT save/reopen and invalid filename-character controls |
| Retained production workflows | Operator acceptance: terrain/water, Dynamic Track, diagnostics, Forest replacement/visibility, generated-work cancel/discard, path editing and full-map behavior |

## Evidence and Coverage Limits

The operator's final dual-lane script records clean configure/build, each full
registered suite and executable SHA-256. MinGW passed 32/32 in 5.90 s; MSVC
passed 32/32 in 6.08 s. Both final logs and hashes were independently reviewed. No zero-warning, leak-free-renderer, exhaustive simulator or
full PBR/animation compatibility claim is made.

The legacy global texture cache and per-context GPU lifetime remain inherited
technical debt. Large transaction payload staging bounds retained save buffers;
it does not remove the editable texture cache or guarantee constant total memory.
Route saves comprise multiple stages; retain complete route backups. Production
DXT1 is deliberately lossy and irreversible and is intended for a separate final
upload copy. Historical v0.18 verification remains in TEST-MATRIX-v0.18.md.
