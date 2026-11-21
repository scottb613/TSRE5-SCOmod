# Regression probes and test utilities for the TSRE build.

add_executable(tsre_terrtex_processing_probe
    "${TSRE_SOURCE_DIR}/tests/TerrtexProcessingProbe.cpp" "${TSRE_SOURCE_DIR}/TerrtexProcessing.cpp"
    "${TSRE_SOURCE_DIR}/DdsWriter.cpp" "${TSRE_SOURCE_DIR}/DdsDecoder.cpp" "${TSRE_SOURCE_DIR}/TerrtexMipmaps.cpp")
target_include_directories(tsre_terrtex_processing_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_terrtex_processing_probe PRIVATE tsre_ace_codec Qt6::Gui Qt6::Core)
add_test(NAME tsre_terrtex_processing_probe
    COMMAND "${CMAKE_COMMAND}" -E env "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_terrtex_processing_probe>")
set_tests_properties(tsre_terrtex_processing_probe PROPERTIES TIMEOUT 45)
add_executable(tsre_gltf_graphics_probe
    "${TSRE_SOURCE_DIR}/tests/GltfGraphicsProbe.cpp" "${TSRE_SOURCE_DIR}/GltfModel.cpp"
    "${TSRE_SOURCE_DIR}/GltfPreview.cpp" "${TSRE_SOURCE_DIR}/DdsDecoder.cpp")
target_include_directories(tsre_gltf_graphics_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_gltf_graphics_probe PRIVATE Qt6::Core Qt6::Gui Qt6::OpenGL)
add_test(NAME tsre_gltf_graphics_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_gltf_graphics_probe>")
set_tests_properties(tsre_gltf_graphics_probe PROPERTIES TIMEOUT 30)
add_executable(tsre_gltf_model_probe
    "${TSRE_SOURCE_DIR}/tests/GltfModelProbe.cpp" "${TSRE_SOURCE_DIR}/GltfModel.cpp" "${TSRE_SOURCE_DIR}/DdsDecoder.cpp")
target_include_directories(tsre_gltf_model_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_gltf_model_probe PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME tsre_gltf_model_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_gltf_model_probe>"
        "${TSRE_SOURCE_DIR}/tests/fixtures/orts-static")
add_executable(tsre_auto_place_wire_probe
    "${TSRE_SOURCE_DIR}/tests/AutoPlaceWireProbe.cpp"
    "${TSRE_SOURCE_DIR}/AutoPlaceWire.cpp")
target_include_directories(tsre_auto_place_wire_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_auto_place_wire_probe PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME tsre_auto_place_wire_probe
    COMMAND "${CMAKE_COMMAND}" -E env
    "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_auto_place_wire_probe>")
add_executable(tsre_rejected_world_file_probe "${TSRE_SOURCE_DIR}/tests/RejectedWorldFileTest.cpp")
add_executable(tsre_static_copy_probe
    "${TSRE_SOURCE_DIR}/tests/StaticCopyProbe.cpp" "${TSRE_SOURCE_DIR}/StaticCopy.cpp" "${TSRE_SOURCE_DIR}/RouteDistro.cpp")
target_include_directories(tsre_static_copy_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_static_copy_probe PRIVATE Qt6::Core)
add_test(NAME tsre_static_copy_probe
    COMMAND "${CMAKE_COMMAND}" -E env
    "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_static_copy_probe>")
add_executable(tsre_route_distro_probe
    "${TSRE_SOURCE_DIR}/tests/RouteDistroProbe.cpp" "${TSRE_SOURCE_DIR}/RouteDistro.cpp")
target_include_directories(tsre_route_distro_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_route_distro_probe PRIVATE Qt6::Core)
add_test(NAME tsre_route_distro_probe
    COMMAND "${CMAKE_COMMAND}" -E env
    "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_route_distro_probe>")
target_include_directories(tsre_rejected_world_file_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_rejected_world_file_probe PRIVATE Qt6::Core)
add_test(NAME tsre_rejected_world_file_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_rejected_world_file_probe>"
)
add_executable(tsre_route_section_policy_probe
    "${TSRE_SOURCE_DIR}/tests/RouteSectionPolicyTest.cpp"
    "${TSRE_SOURCE_DIR}/RouteSectionPolicy.h"
)
target_include_directories(tsre_route_section_policy_probe PRIVATE "${TSRE_SOURCE_DIR}")
add_test(NAME tsre_route_section_policy_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_route_section_policy_probe>"
)
add_executable(tsre_track_overlap_scan_probe
    "${TSRE_SOURCE_DIR}/tests/TrackOverlapScanTest.cpp"
    "${TSRE_SOURCE_DIR}/TrackOverlapScan.h"
)
target_include_directories(tsre_track_overlap_scan_probe PRIVATE "${TSRE_SOURCE_DIR}")
add_test(NAME tsre_track_overlap_scan_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_track_overlap_scan_probe>"
)

add_executable(
    tsre_control_panel_attention_probe
    tests/ControlPanelAttentionProbe.cpp
    "${TSRE_SOURCE_DIR}/ControlPanelAttention.h"
)
target_include_directories(
    tsre_control_panel_attention_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
add_test(
    NAME tsre_control_panel_attention_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_control_panel_attention_probe>"
)

add_executable(tsre_pickup_track_alignment_probe "${TSRE_SOURCE_DIR}/tests/PickupTrackAlignmentProbe.cpp")
target_include_directories(tsre_pickup_track_alignment_probe PRIVATE "${TSRE_SOURCE_DIR}")
add_test(NAME tsre_pickup_track_alignment_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_pickup_track_alignment_probe>")

add_executable(tsre_pole_wobble_probe "${TSRE_SOURCE_DIR}/tests/PoleWobbleProbe.cpp")
target_include_directories(tsre_pole_wobble_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_pole_wobble_probe PRIVATE Qt6::Gui)
add_test(NAME tsre_pole_wobble_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_pole_wobble_probe>"
)

add_executable(tsre_orts_turntable_config_probe
    "${TSRE_SOURCE_DIR}/tests/OrtsTurntableConfigTest.cpp"
    "${TSRE_SOURCE_DIR}/OrtsTurntableConfig.cpp"
)
target_include_directories(tsre_orts_turntable_config_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_orts_turntable_config_probe PRIVATE Qt6::Core)
add_test(NAME tsre_orts_turntable_config_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_orts_turntable_config_probe>"
)

add_executable(
    tsre_place_guard_math_probe
    tests/PlaceGuardMathProbe.cpp
    "${TSRE_SOURCE_DIR}/PlaceGuardMath.h"
)
target_include_directories(
    tsre_place_guard_math_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
add_test(
    NAME tsre_place_guard_math_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_place_guard_math_probe>"
)

add_executable(
    tsre_terrain_track_math_probe
    "${TSRE_SOURCE_DIR}/tests/TerrainTrackMathTest.cpp"
    "${TSRE_SOURCE_DIR}/TerrainTrackMath.h"
)
target_include_directories(
    tsre_terrain_track_math_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
add_test(
    NAME tsre_terrain_track_math_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_terrain_track_math_probe>"
)

add_executable(
    tsre_water_bed_clearance_math_probe
    tests/WaterBedClearanceMathProbe.cpp
    "${TSRE_SOURCE_DIR}/WaterBedClearanceMath.h"
)
target_include_directories(
    tsre_water_bed_clearance_math_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
add_test(
    NAME tsre_water_bed_clearance_math_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_water_bed_clearance_math_probe>"
)

add_executable(
    tsre_terrain_grid_math_probe
    tests/TerrainGridMathProbe.cpp
    "${TSRE_SOURCE_DIR}/TerrainGridMath.h"
    "${TSRE_SOURCE_DIR}/TerrainMesh.cpp"
    "${TSRE_SOURCE_DIR}/Vector3f.cpp"
)
target_include_directories(
    tsre_terrain_grid_math_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
target_link_libraries(tsre_terrain_grid_math_probe PRIVATE Qt6::Gui)
add_test(
    NAME tsre_terrain_grid_math_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_terrain_grid_math_probe>"
)

add_executable(tsre_texture_paint_math_probe "${TSRE_SOURCE_DIR}/tests/TexturePaintMathProbe.cpp")
add_executable(tsre_terrtex_mipmaps_probe
    "${TSRE_SOURCE_DIR}/tests/TerrtexMipmapsProbe.cpp"
    "${TSRE_SOURCE_DIR}/TerrtexMipmaps.cpp"
    "${TSRE_SOURCE_DIR}/AceFormatValidator.cpp")
target_include_directories(tsre_terrtex_mipmaps_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_terrtex_mipmaps_probe PRIVATE Qt6::Gui Qt6::Core)
add_test(NAME tsre_terrtex_mipmaps_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_terrtex_mipmaps_probe>")
target_include_directories(tsre_texture_paint_math_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_texture_paint_math_probe PRIVATE Qt6::Core)
add_test(NAME tsre_texture_paint_math_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_texture_paint_math_probe>"
)

add_executable(tsre_dds_writer_probe
    "${TSRE_SOURCE_DIR}/tests/DdsWriterProbe.cpp"
    "${TSRE_SOURCE_DIR}/DdsWriter.cpp"
    "${TSRE_SOURCE_DIR}/DdsDecoder.cpp"
    "${TSRE_SOURCE_DIR}/TerrtexMipmaps.cpp"
    "${TSRE_SOURCE_DIR}/RouteSaveTransaction.cpp")
target_include_directories(tsre_dds_writer_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_dds_writer_probe PRIVATE Qt6::Gui Qt6::Core)
add_test(NAME tsre_dds_writer_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_dds_writer_probe>")

add_executable(tsre_shape_texture_path_probe "${TSRE_SOURCE_DIR}/tests/ShapeTexturePathProbe.cpp")
target_include_directories(tsre_shape_texture_path_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_shape_texture_path_probe PRIVATE Qt6::Core)
add_test(
    NAME tsre_shape_texture_path_probe
    COMMAND
        "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_shape_texture_path_probe>"
)
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
    "${TSRE_SOURCE_DIR}/DdsDecoder.cpp"
    "${TSRE_SOURCE_DIR}/DdsDecoder.h"
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
    "${TSRE_SOURCE_DIR}/ParserX.cpp"
    "${TSRE_SOURCE_DIR}/TrackShape.cpp"
    "${TSRE_SOURCE_DIR}/ParserX.h"
    "${TSRE_SOURCE_DIR}/FileBuffer.h"
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
    tsre_read_file_safety_probe
    tests/ReadFileSafetyProbe.cpp
    "${TSRE_SOURCE_DIR}/ReadFile.cpp"
    "${TSRE_SOURCE_DIR}/ReadFile.h"
    "${TSRE_SOURCE_DIR}/FileBuffer.h"
    "${TSRE_SOURCE_DIR}/AceFormatValidator.cpp"
    "${TSRE_SOURCE_DIR}/AceFormatValidator.h"
)
target_include_directories(
    tsre_read_file_safety_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
target_link_libraries(tsre_read_file_safety_probe PRIVATE Qt6::Core)
add_test(
    NAME tsre_read_file_safety_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_read_file_safety_probe>"
)

add_executable(
    tsre_ace_format_validator_probe
    tests/AceFormatValidatorProbe.cpp
    "${TSRE_SOURCE_DIR}/AceFormatValidator.cpp"
    "${TSRE_SOURCE_DIR}/AceFormatValidator.h"
)
target_include_directories(
    tsre_ace_format_validator_probe
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(tsre_ace_format_validator_probe PRIVATE Qt6::Core)
add_test(
    NAME tsre_ace_format_validator_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_ace_format_validator_probe>"
)

add_executable(
    tsre_trackdb_format_validator_probe
    tests/TrackDbFormatValidatorProbe.cpp
    "${TSRE_SOURCE_DIR}/TrackDbFormatValidator.cpp"
    "${TSRE_SOURCE_DIR}/TrackDbFormatValidator.h"
)
target_include_directories(
    tsre_trackdb_format_validator_probe
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(tsre_trackdb_format_validator_probe PRIVATE Qt6::Core)
add_test(
    NAME tsre_trackdb_format_validator_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_trackdb_format_validator_probe>"
)

add_executable(
    tsre_route_save_transaction_probe
    "${TSRE_SOURCE_DIR}/tests/RouteSaveTransactionTest.cpp"
    "${TSRE_SOURCE_DIR}/RouteSaveTransaction.cpp"
    "${TSRE_SOURCE_DIR}/RouteSaveTransaction.h"
)
target_include_directories(
    tsre_route_save_transaction_probe
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${TSRE_SOURCE_DIR}"
)
target_link_libraries(tsre_route_save_transaction_probe PRIVATE Qt6::Core)
add_test(
    NAME tsre_route_save_transaction_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_route_save_transaction_probe>"
)

add_executable(
    tsre_forest_definition_probe
    tests/ForestDefinitionProbe.cpp
    "${TSRE_SOURCE_DIR}/ForestDefinition.cpp"
    "${TSRE_SOURCE_DIR}/ForestDefinition.h"
    "${TSRE_SOURCE_DIR}/ForestGenerator.cpp"
    "${TSRE_SOURCE_DIR}/ForestGenerator.h"
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
        "${TSRE_FOREST_TEST_CATALOG}"
)

add_executable(
    tsre_forest_osm_cache_probe
    tests/ForestOsmCacheProbe.cpp
    "${TSRE_SOURCE_DIR}/ForestOsmCache.cpp"
    "${TSRE_SOURCE_DIR}/ForestOsmCache.h"
)

add_executable(
    tsre_forest_patch_baker_probe
    tests/ForestPatchBakerProbe.cpp
    "${TSRE_SOURCE_DIR}/ForestPatchBaker.cpp"
    "${TSRE_SOURCE_DIR}/ForestPatchBaker.h"
)

add_executable(
    tsre_forest_bake_manifest_probe
    tests/ForestBakeManifestProbe.cpp
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.cpp"
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.h"
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
add_executable(tsre_forest_save_cleanup_probe
    "${TSRE_SOURCE_DIR}/tests/ForestBakeSaveCleanupProbe.cpp"
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.cpp")
target_include_directories(tsre_forest_save_cleanup_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_forest_save_cleanup_probe PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME tsre_forest_save_cleanup_probe
    COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_forest_save_cleanup_probe>")
target_include_directories(
    tsre_forest_patch_baker_probe
    PRIVATE
        "${TSRE_SOURCE_DIR}"
)
target_link_libraries(tsre_forest_patch_baker_probe PRIVATE Qt6::Core Qt6::Gui)
add_test(
    NAME tsre_forest_patch_baker_probe
    COMMAND
        "${CMAKE_COMMAND}"
        -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_forest_patch_baker_probe>"
)

add_executable(tsre_forest_replacement_probe
    "${TSRE_SOURCE_DIR}/tests/ForestReplacementProbe.cpp"
    "${TSRE_SOURCE_DIR}/ForestGenerator.cpp"
    "${TSRE_SOURCE_DIR}/ForestPatchBaker.cpp"
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.cpp")
target_include_directories(tsre_forest_replacement_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_forest_replacement_probe PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME tsre_forest_replacement_probe
    COMMAND "${CMAKE_COMMAND}" -E env
    "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_forest_replacement_probe>")

add_executable(
    tsre_bake_forest_patch
    tools/BakeForestPatch.cpp
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.cpp"
    "${TSRE_SOURCE_DIR}/ForestBakeManifest.h"
    "${TSRE_SOURCE_DIR}/ForestPatchBaker.cpp"
    "${TSRE_SOURCE_DIR}/ForestPatchBaker.h"
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

add_executable(tsre_route_paint_plan_probe
    tests/RoutePaintPlanProbe.cpp
    "${TSRE_SOURCE_DIR}/RouteSaveTransaction.cpp"
    "${TSRE_SOURCE_DIR}/RouteSaveTransaction.h"
    "${TSRE_SOURCE_DIR}/RoutePaintPlan.h")
target_include_directories(tsre_route_paint_plan_probe PRIVATE "${TSRE_SOURCE_DIR}")
target_link_libraries(tsre_route_paint_plan_probe PRIVATE Qt6::Core)
add_test(NAME tsre_route_paint_plan_probe
    COMMAND "${CMAKE_COMMAND}" -E env
    "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    "$<TARGET_FILE:tsre_route_paint_plan_probe>")

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
