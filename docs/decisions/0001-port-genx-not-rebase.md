# Decision 0001: Port GenX rather than rebase onto TSRE5vc

Status: Accepted

## Context

GokuMK/TSRE5vc is a Qt6/CMake continuation of the original TSRE 0.7.6 line.
TSRE GenX is a substantial fork of Eric's TSRE 8.006 line with extensive route,
terrain, Dynamic Track, path, settings, save-safety, and interface work.

Reapplying GenX to the older tree would mix functional backporting with the Qt6
migration and make regression causes difficult to isolate.

## Decision

Freeze the GenX v0.8 line and port that source to Qt6/CMake. Use TSRE5vc as a
technical reference, especially for Qt6 API replacements and native compressed
texture handling.

## Consequences

- GenX features remain the functional baseline.
- Qt6 changes must be adapted rather than copied wholesale.
- Source reorganization is postponed.
- Collaboration can still happen through focused, reviewable technical changes.

