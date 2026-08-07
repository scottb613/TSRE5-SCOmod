# TSRE GenX v0.10 WIP

TSRE GenX v0.10 continues the Qt6/CMake development line from v0.9. This is a
source-code checkpoint on the `tsre-scomod-wip` branch and `v0.10` tag. It does
not include an executable, a `dist` package, or a GitHub Release.

## WIP highlights

- Updated the shared application, CMake, vcpkg, and Windows resource identity to v0.10.
- Added an Object Selection `Ruler Water` launcher that starts the existing terrain-snapped Water Helper ruler from the placement click.
- Added one persistent per-route `Ruler Vegetation` placeholder with terrain-snapped control points, shaded selectable handles, and a terrain-following influence corridor for future foliage-spline work.
- Added F12 `Instance Protection`, allowing multiple Route Editor instances when disabled while leaving Shape Viewer unrestricted.
- Restored ACE-to-DDS texture cache reuse without overriding Image Upgrade, Image Substitution, route TERRTEX exclusions, or direct DDS loading behavior.
- Protected encoded object-picking colors by bypassing solid-marker lighting during selection renders in all maintained Fog and Bloom shaders.
- Added flat-face lighting to original 3D marker cubes and directional pyramids while preserving the newer 2D symbol option.
- Moved Sound Source posts to the shared shaded marker path while preserving their colors, selection behavior, and one-draw-call geometry.
- Made the original shaded 3D markers the fresh-install default by changing F12 `New Symbols` to Off; existing saved choices remain respected.

## Verification status

Focused `TexLib.cpp` and `Texture.cpp` compilation and the native DDS decoder
probe passed. Khronos glslang 16.4.0 validated all maintained Fog and Bloom
shader copies. The v0.10 running work log still identifies manual checks for
the ruler workflows, instance behavior, texture-cache memory behavior, marker
presentation, and selection as pending before release acceptance.

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
