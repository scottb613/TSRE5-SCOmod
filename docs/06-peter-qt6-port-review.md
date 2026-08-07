# Peter Grønbæk Andersen Qt6 port review

Status: **Accepted as a primary migration reference**

Reviewed reference:

- Repository: `pgroenbaek/tsre5-trainsimcom-fork-qt6`
- Branch: `cmake-and-qt6-upgrade`
- Pinned commit: `3c3e22aeb43b5ffe2f8c8042c0214324abe860e9`
- Local submodule: `refs/pgroenbaek-tsre5-qt6`
- Source lineage: experimental Qt6 port of Eric's v8.005-RC2 line

## Conclusion

Peter's branch is the closest proven technical reference for the GenX migration
because it ports Eric's 8.x family rather than GokuMK's older 0.7.x tree.

Use Peter's work for the build system and inherited Qt API migration. Continue
using GokuMK's later work for native compressed ACE/DDS/DXT research.

## Adopted recommendations

### Initial Windows toolchain

Use the same proven toolchain shape for the parity port:

- official prebuilt Qt6 desktop package;
- Qt-matched 64-bit MinGW-w64 compiler;
- CMake 3.21 or later;
- `MinGW Makefiles` for the initial Windows generator;
- vcpkg manifest mode for non-Qt dependencies;
- VS Code with CMake Tools as the normal interface.

Ninja remains a valid future option, but changing the generator offers no
porting benefit while reproducing Peter's working environment.

### Dependency management

Adopt a project-owned `vcpkg.json` for:

- fmt;
- openal-soft;
- OpenGL;
- OpenSSL.

Pass the vcpkg toolchain explicitly through CMake presets. Do not require global
`vcpkg integrate install`; the project should remain isolated and reproducible.

Keep local Qt and vcpkg paths in ignored `CMakeUserPresets.json`, never in the
tracked project presets.

### Qt6 source migration

Use Peter's commits as a file-by-file checklist for:

- `QRegExp` and validators;
- `QTextStream` encoding;
- `QDesktopWidget` and screen handling;
- mouse/event position APIs;
- `QXmlStreamReader`;
- AUTOMOC/AUTOUIC/AUTORCC;
- signal/slot conversion to compile-checked pointer syntax;
- OpenGL widget and shader compatibility.

Adapt each change to the frozen GenX file. Do not replace a GenX file wholesale.

### Texture handling

Do not adopt Peter's DDS loader as the final design. It restores DXT1/DXT3/DXT5
loading by expanding compressed data into RGBA memory. That solves Qt6's missing
QImage DDS support but loses the memory advantage of GPU-native compression.

Use Peter's loader only as behavior/reference evidence. Design the final GenX
path from GokuMK's later native compressed upload work, with specific regression
coverage for terrain painting, seasonal TERRTEX, cache invalidation, and saves.

### Packaging

Adopt the deployment sequence, not the batch files verbatim:

1. build into separate Debug/Release directories;
2. run `windeployqt6`;
3. copy vcpkg runtime libraries;
4. copy TSRE runtime assets;
5. verify the packaged executable on a clean machine.

Rewrite deployment as a noninteractive CMake/PowerShell process. Avoid `pause`,
broad `xcopy`, unconditional deletion, and stale files remaining in `dist`.

### Renderer work

Peter's runtime selection between GLSL 130 and 330 is useful evidence for the
legacy-renderer parity stage. It is not permission to combine renderer redesign
with the initial Qt6 compiler port.

## Items intentionally not copied

- root-wide source globbing;
- unused CMake variables and redundant system-library declarations;
- global vcpkg integration;
- interactive post-build scripts;
- Peter's DDS implementation as the final texture architecture;
- `_GLIBCXX_DEBUG` until ABI interactions with dependencies are validated.

## Implementation order

1. Freeze GenX v0.8.
2. Add the vcpkg manifest and local user-preset example.
3. Adapt Peter's Qt/MinGW chainload toolchain.
4. Reach CMake configuration with the unmodified frozen source.
5. Port Qt6 compiler failures using Peter's commits as the primary checklist.
6. Convert and validate signal/slot connections.
7. Reach legacy-renderer and editor parity.
8. Integrate native compressed textures separately.

