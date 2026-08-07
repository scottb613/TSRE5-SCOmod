# TSRE GenX v0.9

TSRE GenX v0.9 modernizes the complete application for Qt6/CMake while
preserving the established editor and MSTS/Open Rails file behavior.

## Highlights

- Moved the complete application from Qt5/qmake to Qt 6.11.1, CMake, a current 64-bit MinGW compiler, and vcpkg-managed supporting libraries.
- Established one reproducible Windows build and deployment system for the Route Editor, Consist Editor, Shape Viewer, Activity Path Editor, runtime assets, and required libraries.
- Targeted native 64-bit Windows 10 version 1809 or later and Windows 11, retiring obsolete Windows 7 toolchain constraints.
- Preserved MSTS/Open Rails UTF-16 serialization with a byte-identical Qt5/Qt6 compatibility probe.
- Completed controlled startup, route save/reload, scenery editing, Place Guard, Camera Terrain Lock, and Activity Path Editor parity tests.
- Added a route regression evidence harness that hashes every route file and reports terrain, database, path, Undo, and save changes.
- Verified Activity Path Editor create, save, reload, and extension behavior while preserving all non-PAT route files, including TrackDB, RoadDB, and both item tables.
- Restored DDS rolling-stock textures in the Qt6 Consist Editor with tested DXT1, DXT3, DXT5, 24-bit, and 32-bit decoding.
- Stabilized native Windows captions, dropdown placement, Route Editor helper shutdown, rejected-Load sound feedback, Terrain Brush sizing, and Water Helper height steps.
- Retained the legacy renderer as the parity reference while keeping native compressed GPU texture work and renderer replacement as separate later work.
- Built v0.9 from the combined foundation of Piotr Gadecki (GokuMK), original author of TSRE5 who shared that invaluable work with the community; Eric Olesen (eric-from-trainsim), who continued the direct TSRE 8.006 source line; and Peter Grønbæk Andersen, whose independent Qt6/CMake and DDS-loading work provided proven modernization references.

## Acknowledgments

Piotr Gadecki (GokuMK) is the original author of TSRE5. He created its core
editor, file, texture, terrain, activity, consist, and rendering systems and
shared that invaluable work with the community. His later TSRE5vc texture work
also provided an independent DXT reference.

Eric Olesen (eric-from-trainsim) carried TSRE forward through the TSRE 8.006
line that directly precedes GenX. His continuing compatibility and editor work,
including the dedicated 8.006n Qt6 branch, provided the primary source/API
migration reference.

Peter Grønbæk Andersen produced the independent Eric-line Qt6/CMake port used
as the primary build-structure and dependency-management reference. His CMake,
vcpkg, deployment, and DDS-loading work supplied proven solutions adapted for
GenX v0.9.

Their combined contributions made the TSRE GenX v0.9 modernization possible.
