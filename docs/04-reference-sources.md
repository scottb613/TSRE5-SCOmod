# Reference source trees

These repositories answer different migration questions. They are evidence, not
one combined codebase.

| Workspace name | Current local path | Primary use |
|---|---|---|
| GenX v0.8 source | `../TSRE/SCOmodWIP` | Exact functional behavior to preserve |
| Goku TSRE5vc | `../TSRE/GokuMK-TSRE5vc-review` | Qt6 APIs, CMake, ACE/DDS/DXT, renderer research |
| Original TSRE5 | `../TSRE/.tsre5-src` | Original intent, history, and pre-fork behavior |
| Open Rails | `../TSRE/.openrails-src` | ORTS formats, compatibility, extensions, and runtime behavior |
| Peter's Qt6 fork | `refs/pgroenbaek-tsre5-qt6` | Proven CMake/vcpkg and Qt6 port of Eric v8.005-RC2 |
| Eric's Qt6 branch | `refs/eric-tsre5-qt6` | Direct Qt6/CMake port of Eric v8.006m, our immediate parent line |

## Rules

1. Never copy an entire reference implementation over a GenX file.
2. Identify the behavior or technique being studied.
3. Record the source repository and commit in the implementing commit message.
4. Compare against GenX changes before adapting code.
5. Verify licensing and attribution before incorporating code.
6. Keep Open Rails compatibility research separate from unrelated Qt6 changes.

## Reference update policy

Pin the reference commits when the migration baseline is frozen. Do not silently
update them during a migration gate. If a reference must move, record the old and
new commits here and explain why.

## Pinned commits

Complete this table at Gate 0.

| Reference | Commit | Date pinned | Notes |
|---|---|---|---|
| GenX v0.8 | `77c69f1` | 2026-07-30 | Local tag `v0.8-port-baseline`; public v0.8 release still pending |
| Goku TSRE5vc | `ad0b0dd` | 2026-07-29 | Qt6 and native compressed-texture reference |
| Original TSRE5 | `af99c14` | 2026-07-29 | Historical reference |
| Open Rails | `ded433da4` | 2026-07-29 | ORTS compatibility reference |
| Peter's Qt6 fork | `3c3e22aeb43b5ffe2f8c8042c0214324abe860e9` | 2026-07-29 | Permanent submodule; Eric-line Qt6 reference |
| Eric's Qt6 branch | `190b11e890b5aa0a27e2f7d2d96d5c6285443b69` | 2026-07-30 | Permanent submodule; branch `8.006n-QT6-VSCode`, based directly on v8.006m |

## Eric branch note

Eric's Qt6 work is on the separate `8.006n-QT6-VSCode` branch. It is not on
`master` or `trainsim-fork-8.006-Testing`. The pinned submodule commit must only
move after reviewing and recording a deliberate upstream update.
