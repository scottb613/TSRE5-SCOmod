# TSRE GenX v0.11

TSRE GenX v0.11 advances the Qt6/CMake development line from the immutable
v0.10 source checkpoint and packages the complete post-v0.10 work for community
open testing. The reviewed source belongs on `tsre-scomod-wip`; the release is
not an official TSRE release and does not replace Eric Olesen's continuing TSRE
work.

## Highlights

- Updated the shared application, CMake, vcpkg, Windows resource, launcher, and document identity to v0.11.
- Removed the final retained qmake application projects, qmake-only test project, obsolete qmake prerequisite, and `.qmake.stash` rule. CMake is the only maintained build path.
- Corrected ENG/WAG token parsing for Unicode whitespace found in community Steam locomotive files, allowing affected rolling stock to appear in Consist Editor while preserving normal MSTS and Open Rails parsing.
- Corrected NVIDIA-style uncompressed DDS loading for rolling stock such as SCO_MEC282 and SCO_RUT282. `DDSD_LINEARSIZE` is no longer mistaken for row pitch, with support retained for 24-bit RGB and 32-bit RGBA textures as well as DXT1, DXT3, and DXT5.
- Fixed excessive memory growth when repeated ACE references resolve to DDS. Resolved textures now retain both names and reuse one decoded cache entry without overriding Image Upgrade, Image Substitution, direct DDS behavior, or route TERRTEX exclusions.
- Corrected cached-texture reload bookkeeping so terrain reloads retain the intended texture ID instead of falling back to texture 0, and release short-lived decoder worker objects after loading.
- Retired Classic Flex and made NextGen S-C-S-C-S Auto-Flex the only Dynamic Track solver. Object Selection supplies one editor-owned `NextGen Dynamic Track` category containing `Dynamic Track (Auto-Flex)`; route and addon REF-file `dyntrack` entries are ignored.
- Unified `Ruler (water)` and `Ruler (vegetation)` as terrain-snapped special rulers with fifty-meter posts, highway-cone-orange cube handles, Select-mode point movement, terrain snap on release, and Undo.
- Retained the forest-green vegetation planning ruler with its terrain-following magenta influence corridor fifty meters to each side, bounded mitered joins, and perpendicular end caps for future foliage-spline work.
- Added a magenta two-endpoint `Ruler (grade)`. Its Properties panel reports `Average Grade` as per-mille, 1-in-X, angle, and percent using endpoint rise over horizontal run.
- Made the water, vegetation, and grade rulers mutually exclusive. Starting any one removes the existing special ruler while leaving the ordinary measurement Ruler independent.
- Improved special-ruler endpoint selection with a seven-meter invisible pick volume around each visible four-meter cube. The clicked encoded point is honored directly, with terrain proximity used only for genuine index collisions above sixteen points.
- Added F12 `Instance Protection`, allowing multiple Route Editor instances when disabled while leaving Shape Viewer unrestricted. A blocked launch uses the standard stopped-action presentation and sound.
- Hardened New Route latitude/longitude entry for up to twelve decimal places, valid geographic ranges, checked MSTS projection and tile conversion, and stopped-action feedback instead of a crash.
- Protected encoded object-picking colors by bypassing solid-marker lighting during selection renders in every maintained Fog and Bloom shader.
- Added flat-face lighting to original 3D marker cubes, directional pyramids, and Sound Source posts while preserving exact selection colors and the newer 2D-symbol option.
- Made the original shaded 3D markers the fresh-install default by changing F12 `New Symbols` to Off; existing saved choices remain respected.
- Added guarded manual GitHub Actions for immutable source tags, draft releases, approved-ZIP SHA-256 verification, and deliberate publication.
- Completed a production-readiness compiler-warning audit without globally suppressing diagnostics. The pass corrected correctness, ownership, file-write, overload, deprecated-API, and control-flow issues and removed dead local allocations while preserving required interface parameters and parser reads with side effects.

## Verification status

The complete v0.11 Release build configured, compiled, and linked successfully. Its clean-build diagnostic count was reduced from 1,272 to 744 before the final low-risk cleanup; the final incremental changed-file pass completed with six warnings, all required but unused `TerrainLibQt` interface parameters, and no actionable warning in the rebuilt files.

All four configured automated tests passed in 2.28 seconds: MSTS/Open Rails text-encoding compatibility, DDS decoding (including NVIDIA-style 24-bit RGB and 32-bit RGBA linear-size headers), Unicode-whitespace parser handling, and the route-regression harness self-test. The maintained Fog and Bloom shaders also passed Khronos glslang validation.

Community manual testing is still required for the new water, vegetation, and grade ruler placement/edit/save workflows; special-ruler picking from difficult camera angles; Dynamic Track Auto-Flex placement and Open Rails operation; SCO_MEC282/SCO_RUT282 Consist Editor appearance; ACE-to-DDS cache reuse and VSL peak memory; Instance Protection and unrestricted Shape Viewer launches; high-precision and invalid New Route coordinates; shaded marker appearance and exact selection; New Symbols defaults; and representative route save/reload behavior. These items remain explicitly marked pending in `TEST-MATRIX-v0.11.md` and are not claimed as passed.

## Download and installation

Download `tsre-scomod-v0.11.zip` from the GitHub Releases page and extract it to
a separate folder. Do not install it over another TSRE copy. Run
`AddShortcutDesktop.cmd` if a desktop shortcut is wanted.

The ZIP includes the executable, required runtime libraries and plugins,
runtime content and assets, `LICENSE.md`, `THIRD-PARTY-NOTICES.txt`, and the
public v0.11 documents. User settings, logs, build files, debug symbols, and
source files are not included in the executable package.

## Release status

v0.11 is a work-in-progress community open-testing release for Windows 10
version 1809 or later and Windows 11. The immutable source tag and packaged ZIP
must identify the same reviewed source. Problems should be reported with the
affected route or rolling stock, the operation performed, and the relevant TSRE
log where available.

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
