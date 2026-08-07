# Gate 5 save, close, and reload test

A separate backup of `SCO_LHR` was completed before this test.
The baseline manifest is stored under `Gate5Evidence/pre-save`.

## Baseline

- Route: `M:\ORTSmini_F\SCO_CLEAN\Train Simulator\routes\SCO_LHR`
- Files: 2,180
- Bytes: 1,309,049,754
- Manifest SHA-256:
  `46F8D939852A4CD2D5836238F941BAC5EEFEF0A5881052E889C4A028591827E6`

## First test: save without intentional editing

1. Close every existing TSRE instance.
2. Launch `Open-Gate5-SCO-LHR.bat`.
3. Confirm the title and loaded route are `SCO_LHR`.
4. Do not move, paint, place, delete, rebuild, or otherwise edit content.
5. Use the normal Route Save command once and allow it to finish.
6. Close Route Editor normally.
7. Launch the same BAT again and confirm the route reloads.
8. Close normally.
9. Run `scripts/Compare-Gate5Route.ps1`.

Review every `Added`, `Removed`, and `ContentChanged` result. Timestamp-only
changes are recorded separately and are not assumed harmless until the file
type and save behavior are understood.

No terrain, object, TrackDB, Dynamic Track, path, or recovery test begins until
this no-edit save has a reviewed file-change set.

## Result

Passed for Qt6 migration parity on 2026-07-30.

- 2,175 of 2,180 route files remained byte-identical.
- `sco_lhr.rdb`, `sco_lhr.rit`, `sco_lhr.tit`, and `tsection.dat` were
  timestamp-only rewrites.
- `sco_lhr.tdb` retained all 87,946 serialized tokens in their original order.
  Twelve floating-point tokens changed only in their final decimal places;
  the largest absolute delta was `0.00001`.
- A second no-edit save repeated the same bounded 12-field normalization. No
  topology, node count, index, tile, world, path, or terrain content changed.
- GenX atomic-save history from the final v0.8 session shows the exact same 12
  tokens drifting in the same directions and magnitudes. This is inherited
  v0.8 floating-point serialization behavior, not a Qt6 regression.
- The route reloaded successfully after each save. Logs contain no save,
  recovery, connection, OpenGL, shader, fatal, or crash failure.
- Camera movement during the test affected session state outside the route
  files and did not affect the result.

## Second test: deliberate scenery-object move

Passed on 2026-07-30.

- The test used the backed-up `SCO_LHR` route.
- One ordinary scenery object was deliberately moved.
- The route was saved, closed, and reloaded normally.
- The moved object remained in its new position after reload.
- Visual inspection found the result satisfactory, with no save, reload,
  rendering, or editor showstopper.

This establishes basic Qt6 parity for an intentional world-object edit. More
destructive placement, deletion, undo, TrackDB, terrain, and path tests remain
separate Gate 5 cases.

## Third test: place and undo a scenery object

Passed on 2026-07-30.

- Captured a fresh 2,180-file baseline after the successful move test.
- Placed one ordinary static scenery object and confirmed it appeared.
- Used Undo once and confirmed the temporary object disappeared.
- Saved, closed, reloaded, and confirmed the object remained absent.
- 2,174 files remained byte-identical and five coordinated database files
  were timestamp-only rewrites.
- No world file changed, proving that Undo left no serialized object record.
- `sco_lhr.tdb` retained identical length, structure, and numeric-token count.
  Seven numeric fields changed by no more than `0.00001`, within the inherited
  floating-point normalization already established by the no-edit tests.

This passes ordinary scenery-object placement/undo persistence for Qt6
migration parity.

## Fourth test: retain a newly placed scenery object

Passed on 2026-07-30.

- Captured a fresh 2,180-file baseline after the placement/undo test.
- Placed one ordinary static scenery object, saved, closed, and reloaded.
- The new object remained visible at the intended position after reload.
- Exactly one world file changed: `world/w-011020+014358.w`.
- The world file grew by 240 bytes and ended with one complete new `Static`
  record for `tree1.s`, UID 48.
- `sco_lhr.tdb` retained identical length, nonnumeric structure, and 83,296
  numeric tokens. Seven numeric fields changed by no more than `0.00001`,
  continuing the established inherited normalization.
- Four coordinated database files were timestamp-only rewrites; all other
  route files remained byte-identical.

This passes retained ordinary scenery-object placement and persistence for
Qt6 migration parity.

## Fifth test: delete the retained scenery object

Passed on 2026-07-30.

- Captured a fresh baseline containing the retained `tree1.s` object, UID 48.
- Deleted that object, saved, closed, and reloaded.
- The deleted object remained absent while the preceding UID 47 object
  remained present.
- The same world file, `world/w-011020+014358.w`, shrank by exactly 240 bytes,
  reversing the retained-placement object block.
- No other world file changed.
- `sco_lhr.tdb` retained identical nonnumeric structure and all 83,296 numeric
  tokens. Seven float fields changed by no more than `0.00001`; one value
  shortened from `4.334199` to `4.3342`, accounting for the four-byte file-size
  reduction in UTF-16.
- Four coordinated database files were timestamp-only rewrites; all remaining
  route files were byte-identical.

This passes ordinary scenery-object deletion and persistence for Qt6 migration
parity.

## Exploratory Place Guard note

An ordinary physical track section was placed above terrain while Place Guard
was enabled. This is permitted by design: physical track retains a special
height allowance for bridges, trestles, chasms, cuts, and elevated
construction. The stricter database-proximity validation applies to
track-linked interactive objects, not ordinary physical track sections.

The isolated test track was removed by restoring its otherwise-empty world
file to the exact 44-byte pre-test content and SHA-256 recorded in the fresh
baseline. A full comparison confirmed every world file returned to baseline.
Only the established bounded TDB floating-point normalization remained, with
five coordinated files changed by timestamp only. No Place Guard defect was
recorded from this invalid test case.

## Sixth test: Object Search and valid Place Guard rejection

Passed on 2026-07-30.

- Object Search filtered the object database for `tree1` and Reset restored
  the complete list.
- With Place Guard enabled, a signal placement was attempted on terrain well
  away from TrackDB.
- Place Guard produced its rejection feedback, automatically invoked Undo,
  and the signal disappeared.
- Save, close, and reload left no signal at the attempted location.
- The runtime log confirms the placement state followed by automatic Undo.
- The affected world tile contains no `Signal`, `TrItemId`, or signal-unit
  data. Its original 145 TrackObj records remain, with the same maximum UID
  of 472.
- The world tile was rewritten because the rejected placement marked it dirty
  before Undo; its 24-byte size change is serialization formatting, not
  retained route content.
- No other world file changed. The remaining database changes were confined
  to the established bounded serializer normalization and coordinated
  timestamp rewrites.

This passes Object Search and track-linked interactive Place Guard rejection
for Qt6 migration parity.
