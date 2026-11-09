# Shared CPU codec for production ACE reads and standalone regression probes.
add_library(tsre_ace_codec STATIC AceDocument.cpp AceDocument.h DxtCodec.cpp DxtCodec.h AceInputValidation.h)
target_include_directories(tsre_ace_codec PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(tsre_ace_codec PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_link_libraries(tsre_ace_codec PUBLIC Qt6::Core)
set_target_properties(tsre_ace_codec PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

if(BUILD_TESTING)
    # Compile the production adapter/writer alongside the frozen legacy reader oracle.
    # The support file supplies data ownership/settings only; no GL context is used.
    add_executable(tsre_ace_codec_parity_probe
        ../tests/AceCodecParityProbe.cpp ../tests/AceParitySupport.cpp
        ../tests/AceLegacyReader.cpp ../tests/AceLegacyReader.h
        ../AceLib.cpp ../AceLib.h ../ReadFile.cpp ../FileBuffer.h
        ../AceFormatValidator.cpp ../TerrtexMipmaps.cpp)
    target_include_directories(tsre_ace_codec_parity_probe PRIVATE "${TSRE_SOURCE_DIR}")
    target_link_libraries(tsre_ace_codec_parity_probe PRIVATE tsre_ace_codec Qt6::Gui Qt6::OpenGL)
    add_test(NAME tsre_ace_codec_probe
        COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_ace_codec_parity_probe>")
    add_test(NAME tsre_ace_pixel_parity_corpus
        COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_ace_codec_parity_probe>" --corpus)
    add_test(NAME tsre_ace_input_preflight_probe
        COMMAND "${CMAKE_COMMAND}" -E env
        "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        "$<TARGET_FILE:tsre_ace_codec_parity_probe>" --preflight)
    set_tests_properties(tsre_ace_codec_probe PROPERTIES TIMEOUT 30)
    set_tests_properties(tsre_ace_pixel_parity_corpus PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
    set_tests_properties(tsre_ace_input_preflight_probe PROPERTIES TIMEOUT 30)
endif()
