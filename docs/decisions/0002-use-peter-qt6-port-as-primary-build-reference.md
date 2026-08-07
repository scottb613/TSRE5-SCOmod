# Decision 0002: Use Peter's Eric-line Qt6 port as the primary build reference

Status: Accepted

## Context

Peter Grønbæk Andersen completed an experimental Qt6/CMake port of Eric's
v8.005-RC2 source. GenX is based on Eric's later v8.006 line, making Peter's
branch structurally closer than the separate GokuMK 0.7.x Qt6 continuation.

## Decision

Pin Peter's branch as a permanent submodule and use it as the primary reference
for CMake, vcpkg, MinGW, packaging flow, and inherited Qt API replacements.

Use GokuMK/TSRE5vc as the primary reference for later native compressed texture
work and additional renderer research.

## Consequences

- The initial Windows generator changes from Ninja to MinGW Makefiles.
- vcpkg manifest mode becomes part of the planned Windows build.
- Qt6 changes are adapted file by file rather than copied wholesale.
- The DDS and packaging implementations require GenX-specific redesign.

