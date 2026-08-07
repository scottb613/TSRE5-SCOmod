# Qt6/CMake migration plan

## Gate 0 — Freeze GenX v0.8

- [x] Finish or explicitly defer open v0.8 work for the migration baseline.
- [x] Identify the accepted Qt5 TST executable and record its SHA-256.
- [x] Accept the current v0.8 behavior as the migration reference.
- [x] Commit and tag the exact source locally as `v0.8-port-baseline`.
- [ ] Publish/push v0.8 only after the separate public release is approved.
- [x] Complete `00-baseline.md`.

Migration exit condition: met locally at commit `77c69f1`. Public release remains
deliberately pending and does not block private v0.9 port work.

## Gate 1 — Exact source import

- [x] Extract all 681 frozen tracked files without Qt6 functional edits.
- [x] Preserve directories and runtime assets.
- [x] Import existing tests.
- [x] Confirm no frozen tracked file is missing.
- [x] Apply only the documented v0.9 version/working-tree identity edits.
- [x] Exclude eight generated `Makefile-*.mk`/`Makefile-variables.mk` qmake
  outputs from the v0.9 repository while retaining the `.pro` inputs.

Exit condition: met. The 673 retained files are traceable to `77c69f1`;
intentional post-import identity changes are limited to `Game.cpp` and
`README.md`, and the eight generated-file exclusions are listed above.

## Gate 2 — CMake build inventory

- [x] Classify application, generated Qt, bundled third-party, and runtime files.
- [x] Establish Qt6, OpenGL/GLU, OpenAL, freeglut, and WebSockets dependencies.
- [x] Add the Qt5/Qt6 text-stream compatibility probe as the first CTest target.
- [x] Convert the applicable automated probes and route-regression harness to
  CTest; legacy qmake-only scaffolding is retained as reference material.
- [x] Keep qmake files only as migration references.

Exit condition: CMake reaches compilation and reports only source/API failures.

## Gate 3 — Mechanical Qt6 API port

- [x] Replace QRegExp/QRegExpValidator.
- [x] Replace QDesktopWidget usage.
- [x] Convert QTextStream codec handling to QStringConverter.
- [x] Replace removed QDataStream device and QStringRef APIs encountered by
  the focused build.
- [x] Update event position and wheel APIs.
- [x] Resolve the container/API removals and overload changes found by the
  source audit, reference review, and complete compiler pass.
- Avoid functional editor redesign.

Exit condition: met. The complete GenX application linked against Qt6 on
2026-07-30. Interactive behavior testing was completed and accepted in Gate 4.

## Gate 4 — Startup and legacy-renderer parity

- [x] Start Route Editor and Consist Editor from the controlled
  `TSREvcTST` package.
- [x] Start Shape Viewer from the controlled package and confirm basic
  route-object and rolling-stock rendering.
- [x] Load the reference route and inspect the initial scene and GenX panels.
- [x] Replace dead Qt5 string-based signal connections reported at runtime.
- [x] Apply and visually verify the shared Windows 10/11 native-caption style.
- [x] Complete the no-save Main smoke test in
  `docs/08-gate4-main-smoke-test.md`.
- [x] Keep the legacy renderer as the reference path.

Exit condition: met. Baseline content opens and remains interactively usable.

## Gate 5 — File and editing parity

- [x] Verify no-edit save, close, reload, atomic backup, and database
  serialization against a full-route hash manifest.
- [x] Verify an intentional world-object edit persists with only the expected
  world and key-database files changed.
- [x] Verify terrain editing and seasonal texture workflows.
- [x] Verify Dynamic Track and TrackDB alignment.
- [x] Verify F4 paths, consists, shapes, settings, undo, and recovery.

Exit condition: met. The closed test matrix has no unexplained Qt6 regressions.

## Gate 6 — Native compressed textures

Status for v0.9: DDS/DXT CPU decoding, normal OpenGL texture upload, and display
are complete. Optional direct upload of compressed DDS blocks and its memory
comparison fixtures are explicitly deferred to post-v0.9 work. The working
OpenGL parity renderer is not an open release test.

- Review GokuMK's current ACE/DDS/DXT implementation and license lineage.
- Adapt compressed GPU upload to the GenX texture model.
- Preserve CPU fallback decoding.
- Add memory and visual comparison fixtures.
- Re-test terrain painting, seasonal lookup, cache invalidation, and saving.

Exit condition: correct visuals with documented memory behavior and no painting
regressions.

## Gate 7 — Packaging and platform work

- [x] Produce a relocatable Windows production package.
- [x] Accept the 64-bit Windows 10 1809-or-later and Windows 11 package after
  extensive operator TST use.
- [x] Do not test or package for Windows 7.
- Defer Linux x64 and Linux ARM builds until after the v0.9 Windows release.
- [x] Document supported compilers and Qt versions.

Exit condition for v0.9: met for the supported Windows production package.
Cross-platform packaging remains future work rather than open v0.9 testing.

## Deferred until after parity

- New renderer completion or replacement.
- Broad source-directory reorganization.
- Architectural rewrites unrelated to Qt6.
- New editor features.
- Windows 7 compatibility.
