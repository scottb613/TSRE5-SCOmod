# Supported platforms

## Windows

The initial TSREvc release target is:

- 64-bit Windows 10 version 1809 or later
- 64-bit Windows 11

Windows 7 and Windows 8 are not supported migration targets. The project will
not downgrade Qt, select an obsolete compiler, or add compatibility shims to
keep those systems operational.

The Windows build uses:

- Qt6
- CMake
- MinGW Makefiles for the initial parity port
- a Qt-supported 64-bit MinGW-w64 toolchain

`WINVER` and `_WIN32_WINNT` are set to the Windows 10 API level for Windows
builds. Qt's generated application manifest supplies modern Windows
compatibility declarations.

## Linux

Linux x64 follows after Windows Qt6 parity. Linux ARM follows after the x64
build and runtime dependencies are stable.

## Why Windows 7 is left behind

- Qt6 does not list Windows 7 as a supported Windows platform.
- Supporting it would require obsolete Qt/toolchain combinations or unofficial
  compatibility work.
- It would constrain APIs, packaging, testing, and future maintenance.
- The migration is intended to modernize the application rather than reproduce
  every limitation of its Qt5 environment.
