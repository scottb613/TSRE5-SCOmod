# TSRE GenX v0.9

TSRE GenX v0.9 modernizes the complete application for Qt6/CMake while
preserving the established editor and MSTS/Open Rails file behavior.

This is a source-only publication on `master` and at the `v0.9` tag. No v0.9
binary package or GitHub Release was created.

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
- Built v0.9 from the complementary work of Piotr Gadecki, Eric Olesen, and Peter Grønbæk Andersen, acknowledged in detail below.

## Acknowledgments and source lineage

Piotr Gadecki, known as GokuMK, is the original author of TSRE5. He created the
core editor, MSTS/Open Rails file handling, texture, terrain, activity, consist,
and rendering systems on which GenX is built and shared that invaluable body of
work with the community. His later TSRE5vc Qt6 and native compressed-texture
work also supplied an essential independent reference for DXT color,
transparency, alpha, and future GPU texture work.

Eric Olesen, known as eric-from-trainsim, carried TSRE forward through the TSRE
8.006 line that directly precedes GenX. His continuing editor, compatibility,
imagery, rolling-stock, route-reporting, and dedicated 8.006n Qt6 work provided
the primary source/API reference for preserving the behavior of the immediate
GenX parent line.

Peter Grønbæk Andersen produced the independent Eric-line Qt6/CMake port used
as the primary build-structure and dependency-management reference. His CMake,
vcpkg, deployment, and DDS-loading work supplied proven solutions that were
reviewed and narrowly adapted to the GenX source rather than copied wholesale.

TSRE GenX v0.9 exists because these three bodies of work complement one
another: Piotr's original application and technical foundation, Eric's
sustained TSRE 8.x development, and Peter's independent modern build and Qt6
migration work.
