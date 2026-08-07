# TSREvc TST

This directory contains the controlled Qt6 Debug test runtime.

The installed executable is built from GenX v0.9 WIP. Reference repositories
are never copied over GenX source.

Current handoff: 2026-07-31

- Build: Qt 6.11.1, MinGW 13.1, CMake Debug
- Executable: `TSRE5.exe`
- SHA-256:
  `942D9FAED2B95D38D9CFBBD9790A1B8868215EF50A5ADAD260C7004DEB13189B`
- Full compile/link: passed
- Qt AUTOMOC: passed
- Qt5/Qt6 UTF-16 compatibility probe: passed byte-for-byte
- DDS DXT1/DXT3/DXT5 and 24/32-bit decoder probe: passed
- Route regression harness self-test: passed
- Recursive runtime dependency scan: passed
- Route Editor, Consist Editor, and Shape Viewer startup: passed
- Consist Editor DDS display: passed

Changes enter this directory only after they compile in `../TSREvcWIP` and are
ready for regression testing against the frozen Qt5 baseline.

The test tree is for:

- repeatable Debug and Release builds;
- route and asset regression tests;
- editor workflow testing;
- deployment/package validation;
- promotion decisions.

Experimental edits belong in `../TSREvcWIP`, not here.

## First startup order

1. Run `TSRE5.exe` and verify the Route Editor load screen appears.
2. Do not open or save a production route during the first startup test.
3. Close normally and retain any `tsre-log-*.txt` file.
4. Test Consist Editor startup from the normal Main Load workflow.
5. Test Shape Viewer startup.

Report any missing-DLL dialog, immediate exit, blank window, OpenGL message, or
crash before beginning route regression tests.
