# TSREvc v0.9

TSREvc is the Qt6/CMake migration workspace for TSRE GenX.

The migration starts from a frozen, tested TSRE GenX v0.8 baseline based on
Eric's TSRE 8.006 line. It does **not** rebase GenX onto GokuMK's older 0.7.x
source tree. GokuMK/TSRE5vc is used as a technical reference for Qt6 API
changes, CMake, native ACE/DXT texture handling, and later renderer work.

Windows 7 compatibility is intentionally discontinued. The initial supported
Windows target is 64-bit Windows 10 version 1809 or later and Windows 11.

## Current status

The approved local v0.8 state is frozen at commit `77c69f1` and tag
`v0.8-port-baseline`. Its tracked application tree has been imported unchanged
into `TSREvcWIP/`; the Qt6/CMake result is v0.9.

The public v0.8 release is still pending. Nothing in this migration workspace
publishes, pushes, or replaces that release.

## Migration rules

1. Preserve GenX behavior before changing architecture.
2. Port Qt5 to Qt6 and qmake to CMake without redesigning the renderer.
3. Keep each migration step buildable and reviewable.
4. Do not add Qt6 Core5Compat as a permanent solution.
5. Integrate native DXT support separately from the basic Qt6 port.
6. Treat terrain painting, seasonal TERRTEX, texture caching, and saving as
   high-risk regression areas.
7. Reorganize source only after the Qt6 build reaches functional parity.
8. Do not add Windows 7 compatibility workarounds or constrain new code to
   obsolete Windows APIs.

## Intended workflow

1. Preserve the frozen GenX v0.8 reference.
2. Run `scripts/Audit-Qt6.ps1` against `TSREvcWIP/`.
3. Configure with the local Windows CMake preset.
4. Port one Qt6 migration concern at a time in `TSREvcWIP/`.
5. Hand verified runtime output to `TSREvcTST/`.
6. Work through `docs/01-migration-plan.md` one gate at a time.
7. Record regression results in `docs/03-test-matrix.md`.

## Layout

- `TSREvcWIP/` — active v0.9 Qt6 migration working tree
- `TSREvcTST/` — controlled build and regression-test tree placeholder
- `cmake/` — shared CMake helpers
- `scripts/` — prerequisite and migration-audit tools
- `docs/` — baseline record, migration plan, audit notes, and test matrix
- `masterDocs/` — v0.9 master release documentation
- `dist/` — approved release-package staging only
- `tests/` — migrated and new automated regression tests
- `.vscode/` — recommended VS Code configuration and tasks
- `TSREvc.code-workspace` — multi-root view of the migration and reference trees

See `docs/05-supported-platforms.md` for the platform policy.

## Reference source trees

Open `TSREvc.code-workspace` in VS Code to work with these trees together:

- `TSREvc` — this Qt6 migration workspace
- `GenX-v0.8-source` — current/frozen GenX source used as the port baseline
- `Goku-TSRE5vc-reference` — GokuMK's Qt6/CMake implementation reference
- `Original-TSRE5-reference` — original TSRE source/history reference
- `OpenRails-reference` — ORTS compatibility and file-behavior reference
- `Peter-Qt6-reference` — proven Qt6/CMake port of Eric's v8.005-RC2 line
- `Eric-v8.006n-Qt6-reference` — Eric's direct Qt6/CMake port of the v8.006m line

Reference trees are read-only evidence unless a task explicitly targets their
own repository. Never make migration edits in a reference checkout.

Peter's and Eric's repositories are pinned as Git submodules under `refs/`.
Initialize them after a fresh clone with:

```powershell
git submodule update --init --recursive
```

The reviews and adopted recommendations are recorded in
`docs/06-peter-qt6-port-review.md` and `docs/07-eric-qt6-branch-review.md`.
