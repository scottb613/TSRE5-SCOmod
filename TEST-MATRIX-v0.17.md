# TSRE GenX v0.17 Test Matrix

Status: release verification record. Final automated verification passed on 29 September
2026. This consolidated public record replaces superseded prototype/build notes;
the detailed development history remains in the internal work log. Publication and asset identity are checked separately against the release tag.

## Build and automated verification

| Lane | Clean Release build | Full registered CTest suite | Test time |
|---|---|---|---|
| Qt 6.11.1 / MinGW 13.1 | Pass | 25/25 passed | 3.12 s |
| Qt 6.11.1 / MSVC v143 | Pass | 25/25 passed | 3.51 s |

Executable SHA-256 (not ZIP checksums):

- MinGW, 9,498,687 bytes: `8F37A4B8755D0B6A52AEFAA4847BF34BB8238E2D0B665A11AB938C66FDC8202A`.
- MSVC, 5,459,968 bytes: `3B0D539DC523A8EAFDE050A33ECAF88A67EE3F10970A4D7A53139558FF06305E`.

Operator configure/build/full-suite logs were reviewed and hashes checked against
actual build outputs. Compiler warnings remain; this is not a warning-free claim.
Synthetic-fixture tests do not establish complete real-route or simulator coverage.

### Registered tests passed in both lanes
- `tsre_auto_place_wire_probe`.
- `tsre_rejected_world_file_probe`.
- `tsre_route_section_policy_probe`.
- `tsre_track_overlap_scan_probe`.
- `tsre_control_panel_attention_probe`.
- `tsre_pickup_track_alignment_probe`.
- `tsre_pole_wobble_probe`.
- `tsre_orts_turntable_config_probe`.
- `tsre_place_guard_math_probe`.
- `tsre_terrain_track_math_probe`.
- `tsre_water_bed_clearance_math_probe`.
- `tsre_terrain_grid_math_probe`.
- `tsre_text_encoding_probe`.
- `tsre_dds_decoder_probe`.
- `tsre_parser_unicode_whitespace_probe`.
- `tsre_read_file_safety_probe`.
- `tsre_ace_format_validator_probe`.
- `tsre_trackdb_format_validator_probe`.
- `tsre_route_save_transaction_probe`.
- `tsre_forest_definition_probe`.
- `tsre_forest_bake_manifest_probe`.
- `tsre_forest_save_cleanup_probe`.
- `tsre_forest_patch_baker_probe`.
- `tsre_forest_osm_cache_probe`.
- `tsre_route_regression_harness`.

## Functional acceptance and remaining checks

The operator reported earlier requested checks complete. That general acceptance
predates the last discard-selection and registry-limit corrections and the final
example packaging. It is not blanket acceptance of every case below.

| Area | Automated evidence and manual acceptance scope |
|---|---|
| Wire limits and cleanup | Probe passes exact limit, overflow rejection, file preservation, retry and malformed-world protection. Verify rejected Commit preserves the viewport preview. |
| Discard object lifetime | Reviewed and compiled in both lanes. Select/copy generated objects and groups, exercise a later discard failure, then F/Shift+F, selection and Paste. Individual post-correction manual results are not separately recorded. |
| SCOsnapPole example | Texture references and ZIP integrity checked. Install on a disposable route and inspect all 16 attachments, Commit/Bake/Save/reopen and simulator rendering. Individual post-correction manual results are not separately recorded. |
| Wire discovery | Geometry/serialization and prefix probes pass. Visually check opposite headings, suffix variants, sparse/more-than-16 attachments, node boundaries, Max Span and duplicate prevention. |
| Generated PolyVeg/wires | Cleanup/save probes pass. Check blue ownership tint, manual scenery preservation, inactive records, partial-save retry and Bake & Save / Discard / Cancel. |
| F11 and track sections | Policy, rejected-file and overlap probes pass. Check cancellation, coverage, No Factor/reset, Jump/Select, recovery and low-ID save/reopen. |
| Placement/pickups/tables | Math/config probes pass. Check elevated scenery, registered-track X guard, finite/Shift range, wobble, pickup snapping and actual turntable/transfer assets. |
| Terrain/textures | Grid, encoding, DDS/ACE and transaction probes pass. Check unloaded tiles, shared/seasonal texture save/reload, Snow switching and shape texture refresh. |
| Activity Builder | Compiled; no integration probe. Check existing/new previews with debug off/on, services, traffic/timetables and save/reload. Unrecognized extension preservation remains limited. |
| Consist Builder/UI | Compiled; no startup-audio probe. Check one launcher cue with saved layouts, View toggles, save-time attention and focus behavior. |

## Release package and document gates

- Documentation follows the previous release format and includes the SNAP Pole
  and Wire guide. Versioned document copies have a SHA-256 manifest.
- The distribution uses the verified MinGW executable above with matched runtime files. The ZIP has a separate archive checksum.
- Package sanitation and checksum review are separate from CTest. The optional model ZIP and checksum are separate attachments.
- Preparing documentation does not create a public tag, upload or publication.
- Whole-route save is not transactional. Keep route backups and check simulator
  behavior after major edits. The v0.16 baseline remains immutable.
