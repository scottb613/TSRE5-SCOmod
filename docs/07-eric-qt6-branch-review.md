# Eric v8.006n Qt6 branch review

Reviewed upstream branch:

- Repository: `https://github.com/eric-from-trainsim/TSRE5-Trainsim.Com-Fork`
- Branch: `8.006n-QT6-VSCode`
- Pinned commit: `190b11e890b5aa0a27e2f7d2d96d5c6285443b69`
- Review date: 2026-07-30

## Standing

This is now the primary source-level Qt6 migration reference because it is a
direct three-commit conversion of Eric's `8.006m` testing line—the immediate
ancestor of GenX. Peter's port remains the stronger independent reference for
build organization and vcpkg usage. Goku's TSRE5vc remains the specialist
reference for native compressed ACE/DDS/DXT handling and renderer research.

## What the branch changes

- Moves the project from Qt5/qmake/NetBeans to Qt6/CMake/VS Code.
- Adds Qt6 API conversions for input events, screen handling, containers,
  networking, and signal/slot connections.
- Enables CMake AUTOMOC and removes checked-in generated `moc_*.cpp` files.
- Adds a CMake executable target and automatic `windeployqt` execution.
- Retains the v8.006m application feature line rather than rebasing onto a
  different TSRE fork.

## What not to copy unchanged

- `cmakelists.txt` contains machine-specific `E:/DEV` OpenAL and GLUT paths.
- `CMakeUserPresets.json` contains machine-local Qt and vcpkg paths and belongs
  outside a shared repository.
- Source collection uses a broad root-level `file(GLOB ...)`.
- Deployment runs automatically after every Windows build.
- The file is named `cmakelists.txt`; our project will retain the conventional
  `CMakeLists.txt` name and its existing preset/toolchain structure.

## Migration use

For each GenX file, compare Eric's v8.006m-to-v8.006n change first. Adapt the
smallest applicable Qt6 API change into `TSREvcWIP`, preserving GenX behavior.
Use Peter or Goku only where Eric's branch does not solve the issue adequately.
Do not merge or copy the upstream tree wholesale.
