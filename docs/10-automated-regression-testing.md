# Automated regression testing

The route regression harness surrounds a real editor scenario with repeatable
evidence collection. It does not inject input into TSRE, run background work in
the editor, or add anything to the render loop.

## Captured evidence

`scripts/Invoke-RouteRegression.ps1` hashes every route file and preserves exact
copies of the route data that an editing operation can normally mutate:

- world tiles;
- terrain and low-resolution terrain tiles;
- paths;
- TrackDB, RoadDB, item tables, route metadata, and route configuration files.

For the current test route this is approximately 84 MB of copied evidence while
the full 1.3 GB route remains covered by the hash manifest.

The comparison pass reports:

- added, removed, content-changed, timestamp-only, and unchanged files;
- changed 16-bit terrain sample count, maximum raw delta, and total raw delta;
- changed byte count and first/last changed offsets for other preserved files;
- normalized SIMISA text hashes plus declared and actual TrackDB node, junction,
  vector-node, item-table, path-PDP, and path-node counts for `.tdb`, `.rdb`,
  `.tit`, `.rit`, and `.pat` files;
- reverse and wait/advanced `TrPathNode` flag counts for saved paths;
- Undo-state, Undo, save, warning, and critical counts from runtime logs;
- CSV details and a JSON summary suitable for later tooling.

## Use

Capture a baseline immediately before an editor scenario:

```powershell
.\scripts\Invoke-RouteRegression.ps1 `
    -Action Capture `
    -Scenario terrain-height-undo `
    -Expectation UndoRestoresData
```

Perform the editor operation, save, close, and reload. Then compare:

```powershell
.\scripts\Invoke-RouteRegression.ps1 `
    -Action Compare `
    -Scenario terrain-height-undo
```

Evidence is stored under `Gate5Evidence\automated\<scenario>\<run-id>`.
`Gate5Evidence` remains local and is not release content.

Available expectations are:

- `NoMaterialChange`: any added, removed, or content-changed file fails.
- `UndoRestoresData`: non-metadata residue or an Undo-log imbalance fails;
  rewritten terrain `.t` metadata is reported as a warning for review.
- `AllowChanges`: changes are measured and reported without assuming whether
  the deliberate edit should persist.
- `PathEdit`: requires a changed or added `.pat` file and fails if any non-path
  route file changes.
- `TrackDbEdit`: requires a TrackDB/item-table change and fails if RoadDB or its
  item table also changes.
- `RoadDbEdit`: requires a RoadDB/item-table change and fails on a TrackDB
  structural or item-table change. The known save-time TrackDB normalization
  rewrite (16 changed bytes or fewer with every structural count unchanged) is
  retained in evidence and reported as a warning rather than a failure.

## UI smoke driver

`scripts/ui-tests/Invoke-TSREPlacementPrototype.ps1` drives the real Route
Editor interface against only the disposable `Gate5Sandbox\Train Simulator`
root. It filters and physically clicks an object row, places one track or road
shape, deselects the object so TSRE commits it to the corresponding database,
saves, reloads, and captures screenshots.

The initial saved-route verification established:

- track placement changed one world tile and grew TrackDB from 761 to 764
  nodes;
- road placement changed one world tile and grew RoadDB from 0 to 6 nodes;
- neither operation changed a terrain or terrain-height file;
- the source test route was never edited, and the disposable clone was
  restored to all 2,181 source-file hashes after the tests.

This is a deterministic smoke-test foundation, not a completed arbitrary-view
or 100-cycle UI stress system.

Run the harness self-test through CTest:

```powershell
& 'Y:\DEVTOOLS\TSREvc\Qt\Tools\CMake_64\bin\ctest.exe' `
    --test-dir .\build\windows-debug-local `
    --output-on-failure
```

## Future testability boundary

No AI route-building hook is part of the Qt6 parity port. Future editor
architecture should nevertheless avoid blocking safe automation:

- editor operations should converge on validated command functions rather than
  duplicating behavior in button handlers;
- commands should support dry-run validation, transactions, Undo, and factual
  result reporting;
- stable object identifiers and explicit route/tile coordinates should be used
  instead of screen positions;
- automation must be opt-in and local, with no telemetry or network dependency;
- commands must execute outside the render loop and enter editor state through
  the same serialized main-thread boundary used by ordinary UI actions;
- destructive or database-changing commands must require a route backup and
  explicit approval.

That boundary can later support deterministic test drivers, batch route tools,
or AI-assisted route construction without imposing a performance cost on normal
editing. Implementation begins only after Qt6 behavior parity is established.
