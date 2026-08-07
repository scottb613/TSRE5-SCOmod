# TSRE GenX v0.10 WIP

TSRE GenX v0.10 continues the Qt6/CMake development line from v0.9. Current
source development is published on `tsre-scomod-wip`; the immutable `v0.10`
tag records the initial v0.10 source checkpoint. It does not include an
executable, a `dist` package, or a GitHub Release.

## WIP highlights

- Updated the shared application, CMake, vcpkg, and Windows resource identity to v0.10.
- Removed the seven retained application qmake projects, the qmake-only RouteSaveTransaction test project, the obsolete qmake prerequisite check, and the `.qmake.stash` ignore entry. CMake/CTest is now the only maintained build path.
- Added an Object Selection `Ruler Water` launcher that starts the existing terrain-snapped Water Helper ruler from the placement click.
- Added one persistent per-route `Ruler Vegetation` placeholder with terrain-snapped control points, shaded selectable handles, and a terrain-following influence corridor for future foliage-spline work.
- Added F12 `Instance Protection`, allowing multiple Route Editor instances when disabled while leaving Shape Viewer unrestricted.
- Hardened New Route latitude/longitude entry for up to twelve decimal places, valid geographic ranges, checked MSTS projection/tile conversion, and standard stopped-action feedback instead of a crash.
- Fixed excessive memory growth from repeated ACE references resolving to DDS. The resolved DDS texture now also records the requested ACE path, so later references reuse one decoded texture instead of retaining duplicate DDS copies, without overriding Image Upgrade, Image Substitution, route TERRTEX exclusions, or direct DDS behavior.
- Protected encoded object-picking colors by bypassing solid-marker lighting during selection renders in all maintained Fog and Bloom shaders.
- Added flat-face lighting to original 3D marker cubes and directional pyramids while preserving the newer 2D symbol option.
- Moved Sound Source posts to the shared shaded marker path while preserving their colors, selection behavior, and one-draw-call geometry.
- Made the original shaded 3D markers the fresh-install default by changing F12 `New Symbols` to Off; existing saved choices remain respected.
- Added guarded manual GitHub Actions that can create an immutable source tag, optionally prepare a draft release, verify one approved ZIP against its SHA-256, and publish only after explicit confirmation and environment approval.

## Verification status

Focused `TexLib.cpp` and `Texture.cpp` compilation and the native DDS decoder
probe passed. Khronos glslang 16.4.0 validated all maintained Fog and Bloom
shader copies. The v0.10 running work log still identifies manual checks for
the ruler workflows, instance behavior, New Route coordinate cases,
texture-cache memory behavior, marker presentation, and selection as pending
before release acceptance.

The release workflows passed actionlint 1.7.12 validation. They remain inactive
until the workflow files are explicitly pushed to the default branch and the
protected `release` environment is configured on GitHub.

The focused prerequisite check passes without qmake. A fresh CMake configure
requested an external OpenAL vcpkg rebuild, so that longer dependency compile
remains operator-controlled; the qmake metadata cleanup does not change the
application executable.

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

TSRE GenX continues because these three bodies of work complement one another:
Piotr's original application and technical foundation, Eric's sustained TSRE
8.x development, and Peter's independent modern build and Qt6 migration work.
