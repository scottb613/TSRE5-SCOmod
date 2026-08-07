# Tests

The v0.9 CMake build includes focused automated checks for file compatibility,
texture decoding, and the route regression evidence harness.

Current automated coverage includes:

- route save transaction and interrupted-save recovery;
- terrain/track geometry math;
- route loading without an interactive window;
- texture decode and upload fixtures for ACE, DDS/DXT1, DXT3, and DXT5;
- path serialization and validation;
- settings JSON recovery and atomic saving.

`scripts/Invoke-RouteRegression.ps1` automates the evidence around manual
Route Editor scenarios. `Capture` records a full-route hash manifest and exact
copies of mutable route data. After the editor scenario, `Compare` reports
added, removed, content-changed, and timestamp-only files; measures changed
16-bit terrain samples; records binary byte ranges; and checks Undo/save log
counts. By default, evidence is written below the ignored
`.route-regression-evidence` directory; callers may select another location.

The `tsre_route_regression_harness` CTest exercises both an unchanged fixture
and a one-sample terrain mutation, so changes to the PowerShell harness are
verified without opening the application.
