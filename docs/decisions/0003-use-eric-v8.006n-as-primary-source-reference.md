# Decision 0003: Use Eric v8.006n as the primary source-port reference

Date: 2026-07-30

## Decision

Use Eric's `8.006n-QT6-VSCode` branch first when resolving Qt5-to-Qt6 source API
changes because it directly ports the v8.006m line from which GenX developed.
Keep Peter's fork as the primary independent CMake/vcpkg build reference and
Goku's TSRE5vc as the compressed-texture and renderer research reference.

## Consequence

Eric's source changes are compared file by file and adapted into
`TSREvcWIP/`. His machine-specific CMake paths and deployment choices are not
copied into the v0.9 build.
