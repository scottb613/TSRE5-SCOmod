cmake_minimum_required(VERSION 3.21)

project(
    TSREvc
    VERSION 0.13.0
    DESCRIPTION "TSRE GenX Qt6 route editor"
    LANGUAGES C CXX
)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(TSRE_ENABLE_WARNINGS "Enable the project warning set" ON)
option(TSRE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

set(TSRE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")


find_package(
    Qt6 6.5 REQUIRED
    COMPONENTS
        Core
        Gui
        Network
        OpenGL
        OpenGLWidgets
        WebSockets
        Widgets
)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

include(cmake/TSREWarnings.cmake)

# Keep the established public source layout at the repository root.
# Tests and rollback/build directories are intentionally outside this glob.
file(
    GLOB TSRE_SOURCES CONFIGURE_DEPENDS
    "${TSRE_SOURCE_DIR}/*.c"
    "${TSRE_SOURCE_DIR}/*.cpp"
    "${TSRE_SOURCE_DIR}/*.h"
    "${TSRE_SOURCE_DIR}/*.qrc"
    "${TSRE_SOURCE_DIR}/*.rc"
    "${TSRE_SOURCE_DIR}/*.ui"
)

# These are legacy IDE helpers or standalone test programs, not application
# translation units. Keep them in the imported baseline but out of TSRE5.
list(
    FILTER TSRE_SOURCES EXCLUDE
    REGEX "/(TerrainTrackMathTest\\.cpp|c_standard_headers_indexer\\.c|cpp_standard_headers_indexer\\.cpp|moc_.*\\.cpp|moc_predefs\\.h|HelloQtWorld_1_resource\\.rc)$"
)

add_executable(TSRE5 WIN32 ${TSRE_SOURCES})

if(MSVC)
    set_property(
        TARGET TSRE5
        PROPERTY VS_DEBUGGER_ENVIRONMENT
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;%PATH%"
    )
endif()

target_include_directories(
    TSRE5
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)

target_compile_definitions(
    TSRE5
    PRIVATE
        UNICODE
        _UNICODE
)

if(WIN32)
    # TSREvc intentionally targets modern Windows and does not retain Win7
    # compatibility. Qt6's supported Windows floor is Windows 10 1809.
    target_compile_definitions(
        TSRE5
        PRIVATE
            WINVER=0x0A00
            _WIN32_WINNT=0x0A00
    )
    target_link_libraries(TSRE5 PRIVATE winmm)
endif()

target_link_libraries(
    TSRE5
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::WebSockets
        Qt6::Widgets
        OpenGL::GL
        OpenGL::GLU
        GLUT::GLUT
)

if(TSRE_ENABLE_WARNINGS)
    tsre_enable_warnings(TSRE5)
endif()

if(BUILD_TESTING)
    add_executable(
        tsre_text_encoding_probe
        tests/TextStreamEncodingProbe.cpp
    )
    target_link_libraries(tsre_text_encoding_probe PRIVATE Qt6::Core)
    add_test(
        NAME tsre_text_encoding_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_text_encoding_probe>"
            "${CMAKE_CURRENT_BINARY_DIR}/qt6-text-encoding-probe.txt"
    )

    add_executable(
        tsre_dds_decoder_probe
        tests/DdsDecoderProbe.cpp
        DdsDecoder.cpp
        DdsDecoder.h
    )
    target_include_directories(
        tsre_dds_decoder_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_dds_decoder_probe PRIVATE Qt6::Core)
    add_test(
        NAME tsre_dds_decoder_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_dds_decoder_probe>"
    )

    add_executable(
        tsre_parser_unicode_whitespace_probe
        tests/ParserUnicodeWhitespaceProbe.cpp
        ParserX.cpp
        ParserX.h
        FileBuffer.h
    )
    target_include_directories(
        tsre_parser_unicode_whitespace_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_parser_unicode_whitespace_probe PRIVATE Qt6::Core)
    add_test(
        NAME tsre_parser_unicode_whitespace_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_parser_unicode_whitespace_probe>"
    )

    add_executable(
        tsre_forest_definition_probe
        tests/ForestDefinitionProbe.cpp
        ForestDefinition.cpp
        ForestDefinition.h
        ForestGenerator.cpp
        ForestGenerator.h
    )
    target_include_directories(
        tsre_forest_definition_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_forest_definition_probe PRIVATE Qt6::Core)
    add_test(
        NAME tsre_forest_definition_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_forest_definition_probe>"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/forest.v1.example.json"
    )

    add_executable(
        tsre_forest_osm_cache_probe
        tests/ForestOsmCacheProbe.cpp
        ForestOsmCache.cpp
        ForestOsmCache.h
    )

    add_executable(
        tsre_forest_patch_baker_probe
        tests/ForestPatchBakerProbe.cpp
        ForestPatchBaker.cpp
        ForestPatchBaker.h
    )

    add_executable(
        tsre_forest_bake_manifest_probe
        tests/ForestBakeManifestProbe.cpp
        ForestBakeManifest.cpp
        ForestBakeManifest.h
    )
    target_include_directories(
        tsre_forest_bake_manifest_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_forest_bake_manifest_probe PRIVATE Qt6::Core Qt6::Gui)
    add_test(
        NAME tsre_forest_bake_manifest_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_forest_bake_manifest_probe>"
    )
    target_include_directories(
        tsre_forest_patch_baker_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_forest_patch_baker_probe PRIVATE Qt6::Core Qt6::Gui)

    add_executable(
        tsre_bake_forest_patch
        tools/BakeForestPatch.cpp
        ForestBakeManifest.cpp
        ForestBakeManifest.h
        ForestPatchBaker.cpp
        ForestPatchBaker.h
    )
    target_include_directories(
        tsre_bake_forest_patch
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_bake_forest_patch PRIVATE Qt6::Core Qt6::Gui)
    target_include_directories(
        tsre_forest_osm_cache_probe
        PRIVATE
            "${TSRE_SOURCE_DIR}"
    )
    target_link_libraries(tsre_forest_osm_cache_probe PRIVATE Qt6::Core)
    add_test(
        NAME tsre_forest_osm_cache_probe
        COMMAND
            "${CMAKE_COMMAND}"
            -E env
            "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            "$<TARGET_FILE:tsre_forest_osm_cache_probe>"
    )

    if(WIN32)
        add_test(
            NAME tsre_route_regression_harness
            COMMAND
                powershell
                -NoProfile
                -ExecutionPolicy Bypass
                -File
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/RouteRegressionHarnessSelfTest.ps1"
                -Harness
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/Invoke-RouteRegression.ps1"
                -WorkingDirectory
                "${CMAKE_CURRENT_BINARY_DIR}/route-regression-self-test"
        )
    endif()
endif()

install(TARGETS TSRE5 RUNTIME DESTINATION .)

foreach(runtime_dir IN ITEMS content icons tsre_appdata tsre_assets)
    if(EXISTS "${TSRE_SOURCE_DIR}/${runtime_dir}")
        install(DIRECTORY "${TSRE_SOURCE_DIR}/${runtime_dir}" DESTINATION .)
    endif()
endforeach()
