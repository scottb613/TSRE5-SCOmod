# TSREvc agent instructions

## Project objective

Migrate the frozen TSRE GenX v0.8 application from Qt5/qmake to Qt6/CMake while
preserving editor behavior and file compatibility.

## Required working practices

- Read and follow `AI_HouseRules.txt` before changing or building v0.9.
- Never import from an uncommitted or untagged GenX working tree.
- Do not combine Qt6 migration, renderer replacement, and source reorganization
  in one change.
- Prefer small commits grouped by one migration concern.
- Preserve MSTS and Open Rails file encoding and serialization behavior.
- Do not use Qt6 Core5Compat as a permanent dependency.
- Do not copy GokuMK texture code wholesale. Adapt it after reviewing conflicts
  with GenX terrain painting, seasonal textures, cache invalidation, and saves.
- Keep the legacy renderer as the parity reference until the Qt6 port is stable.
- Update the migration checklist and test matrix with every completed gate.
- Never overwrite the frozen baseline record.
- Treat Open Rails, original TSRE5, GokuMK/TSRE5vc, and the frozen GenX checkout
  as read-only references during migration work.
- Treat `refs/eric-tsre5-qt6` as the primary source/API reference because it
  directly ports Eric v8.006m. Use `refs/pgroenbaek-tsre5-qt6` as the primary
  independent build-structure reference. Do not edit either submodule.
- Put all Qt6 port changes in this repository's `TSREvcWIP/`, never in a reference
  checkout.
- Windows 7 support is explicitly out of scope. Do not introduce compatibility
  shims, old toolchains, or dependency downgrades to retain it.

## Minimum verification

Every functional migration change must:

1. configure successfully with CMake;
2. compile the affected target;
3. run relevant automated tests;
4. record any required manual editor checks;
5. preserve clean `git status` except for the intended change.
