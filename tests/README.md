# Tests

Import existing GenX tests with the frozen source baseline, then convert them to
CMake targets before functional porting begins.

Initial automated coverage should include:

- route save transaction and interrupted-save recovery;
- terrain/track geometry math;
- route loading without an interactive window;
- texture decode and upload fixtures for ACE, DDS/DXT1, DXT3, and DXT5;
- path serialization and validation;
- settings JSON recovery and atomic saving.

Manual editor coverage is tracked in `Docs/TEST-MATRIX-v0.15.md`.

`scripts/Invoke-RouteRegression.ps1` automates the evidence around manual
Route Editor scenarios. `Capture` records a full-route hash manifest and exact
copies of mutable route data. After the editor scenario, `Compare` reports
added, removed, content-changed, and timestamp-only files; measures changed
16-bit terrain samples; records binary byte ranges; and checks Undo/save log
counts. Evidence is written below the ignored `Gate5Evidence/automated`
directory.

The `tsre_route_regression_harness` CTest exercises both an unchanged fixture
and a one-sample terrain mutation, so changes to the PowerShell harness are
verified without opening the application.

Critical-format probes added during the v0.14 audit:

- `tsre_terrain_grid_math_probe` verifies checked render-buffer counts for the
  256/8 m and 512/4 m terrain layouts, rejects invalid or oversized layouts,
  and verifies exact UInt16 height/byte flag payload sizes for both grids.
- `tsre_route_save_transaction_probe` verifies multi-component commit,
  rollback after a later component failure, and route-root confinement.
- `tsre_ace_format_validator_probe` verifies ACE header, dimension, allocation,
  offset, and uncompressed/DXT payload bounds.
- `tsre_trackdb_format_validator_probe` verifies TDB/RDB node, section,
  item-reference, and exact pin-layout limits shared by file/network parsing.
