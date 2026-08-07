foreach(required_variable IN ITEMS QT_ROOT QT_VERSION QT_ARCH QT_MINGW_VERSION)
    if(NOT DEFINED ENV{${required_variable}} OR "$ENV{${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} is not defined in the selected CMake preset.")
    endif()
endforeach()

file(TO_CMAKE_PATH "$ENV{QT_ROOT}" QT_ROOT_PATH)
set(QT_PATH "${QT_ROOT_PATH}/$ENV{QT_VERSION}/$ENV{QT_ARCH}")
set(MINGW_BIN "${QT_ROOT_PATH}/Tools/$ENV{QT_MINGW_VERSION}/bin")

set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe" CACHE FILEPATH "Qt-matched MinGW C compiler")
set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe" CACHE FILEPATH "Qt-matched MinGW C++ compiler")
set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe" CACHE FILEPATH "Qt-matched MinGW resource compiler")
set(CMAKE_MAKE_PROGRAM "${MINGW_BIN}/mingw32-make.exe" CACHE FILEPATH "MinGW Makefiles runner")
set(CMAKE_PREFIX_PATH "${QT_PATH}/lib/cmake" CACHE PATH "Qt6 CMake package directory")

