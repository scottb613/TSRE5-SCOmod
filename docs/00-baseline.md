# Migration baseline

Status: **FROZEN LOCALLY FOR v0.9 MIGRATION**

The public GenX v0.8 release is still pending. This record identifies the local
working state approved for the v0.9 Qt6 port; it does not publish or replace the
existing public `v0.8` tag.

## Source

- Upstream lineage: Eric TSRE 8.006
- GenX repository: `Y:\DEVCODE\VSwork\TSRE\SCOmodWIP`
- Frozen branch: `main`
- Frozen tag: `v0.8-port-baseline` (local only)
- Frozen commit SHA: `77c69f1`
- Freeze date: 2026-07-30
- Working tree clean: YES for tracked application content; local `tmp/`
  rollback snapshots remain deliberately untracked

The v0.9 import extracted all 681 files from this tag. Eight generated qmake
`Makefile-*.mk`/`Makefile-variables.mk` outputs remain excluded by the inherited
ignore rules; their `.pro` inputs are retained. The v0.9 repository tracks the
remaining 673 imported source, asset, test, and reference files.

## Build artifact

- Qt5 reference executable: `Y:\DEVCODE\VSwork\TSRE\SCOmodTST\TSRE5.exe`
- Executable SHA-256: `43D9C426DCB3D003F5CF9FC44561065174585780EBD8B3345F441AD5E1E3FD85`
- Public package: pending; v0.8 has not been published
- Compiler/toolchain: MSYS2 MinGW-w64 64-bit, GCC 16.1.0
- Qt version: 5.15.19
- Build command: `Y:\msys64\usr\bin\bash.exe -lc 'export PATH=/mingw64/bin:/usr/bin:$PATH; cd "/y/DEVCODE/VSwork/TSRE/SCOmodWIP" && mingw32-make -j6'`
- Build result: accepted working TST executable dated 2026-07-30; no additional
  long rebuild was run during the freeze

## Baseline verification

- Route loader: accepted v0.8 reference behavior
- Route editor: accepted v0.8 reference behavior
- Terrain painting: accepted v0.8 reference behavior
- Seasonal TERRTEX: accepted v0.8 reference behavior
- Dynamic Track Classic: accepted v0.8 reference behavior
- Dynamic Track NextGen: accepted v0.8 reference behavior
- TrackDB/RoadDB save and reload: accepted v0.8 reference behavior
- F4 path create/edit/save/reload: accepted v0.8 reference behavior
- Consist Editor: accepted v0.8 reference behavior
- Shape Viewer: accepted v0.8 reference behavior
- Settings save/recovery: accepted v0.8 reference behavior

## Known baseline defects

No additional defects were declared at freeze. Detailed manual verification is
still required at each v0.9 parity gate; accepted v0.8 behavior is the reference.
