# This CMake build file is intended for use with the llvm-mingw toolchain:
# https://github.com/mstorsjo/llvm-mingw
#
# It also works for building natively on Linux, or cross-building from Linux
# for running on Windows with a mingw-w64 toolchain.
#
# It most probably doesn't work with MSVC.

cmake_minimum_required(VERSION 3.16)

project(cppwinrt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CPPWINRT_BUILD_VERSION "2.3.4.5" CACHE STRING "The version string used for cppwinrt.")
if(CPPWINRT_BUILD_VERSION STREQUAL "2.3.4.5" OR CPPWINRT_BUILD_VERSION STREQUAL "0.0.0.0")
    message(WARNING "CPPWINRT_BUILD_VERSION has been set to a dummy version string. Do not use in production!")
endif()
message(STATUS "Using version string: ${CPPWINRT_BUILD_VERSION}")

if(WIN32)
    # WinMD uses CreateFile2 which requires Windows 8.
    add_compile_definitions(_WIN32_WINNT=0x0602)
endif()


# === prebuild: Generator tool for strings.cpp, strings.h, version.rc ===

if(CMAKE_CROSSCOMPILING)
    include(ExternalProject)
    ExternalProject_Add(cppwinrt-prebuild
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/prebuild"
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> "-DCPPWINRT_BUILD_VERSION=${CPPWINRT_BUILD_VERSION}"
    )
    ExternalProject_Get_Property(cppwinrt-prebuild INSTALL_DIR)
    set(PREBUILD_TOOL "${INSTALL_DIR}/bin/cppwinrt-prebuild")
    unset(INSTALL_DIR)
else()
    add_subdirectory(prebuild)
    set(PREBUILD_TOOL cppwinrt-prebuild)
endif()


# === Step to create autogenerated files ===

file(GLOB PREBUILD_STRINGS_FILES
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    strings/*.h
)
add_custom_command(
    OUTPUT
        ${PROJECT_BINARY_DIR}/strings.cpp
        ${PROJECT_BINARY_DIR}/version.rc
    COMMAND "${PREBUILD_TOOL}" ARGS "${PROJECT_SOURCE_DIR}/strings" "${PROJECT_BINARY_DIR}"
    DEPENDS
    cppwinrt-prebuild
        ${PREBUILD_STRINGS_FILES}
    VERBATIM
)


# === cppwinrt ===

set(CPPWINRT_SRCS
    cppwinrt/main.cpp
    "${PROJECT_BINARY_DIR}/strings.cpp"
)

set(CPPWINRT_HEADERS
    cppwinrt/pch.h
    cppwinrt/cmd_reader.h
    cppwinrt/code_writers.h
    cppwinrt/component_writers.h
    cppwinrt/file_writers.h
    cppwinrt/helpers.h
    cppwinrt/pch.h
    cppwinrt/settings.h
    cppwinrt/task_group.h
    cppwinrt/text_writer.h
    cppwinrt/type_writers.h
)

if(WIN32)
    add_custom_command(
        OUTPUT
            "${PROJECT_BINARY_DIR}/app.manifest"
        COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/cppwinrt/app.manifest" "${PROJECT_BINARY_DIR}/app.manifest"
        DEPENDS "${PROJECT_SOURCE_DIR}/cppwinrt/app.manifest"
        VERBATIM
    )
    # Do the configure_file dance so that app.manifest.rc don't get modified every
    # single time the project is reconfigured and trigger a rebuild.
    file(WRITE "${PROJECT_BINARY_DIR}/app.manifest.rc.in" "1 24 \"app.manifest\"\n")
    configure_file(
        "${PROJECT_BINARY_DIR}/app.manifest.rc.in"
        "${PROJECT_BINARY_DIR}/app.manifest.rc"
        COPYONLY
    )

    set(CPPWINRT_RESOURCES
        "${PROJECT_BINARY_DIR}/app.manifest"
        "${PROJECT_BINARY_DIR}/app.manifest.rc"
        "${PROJECT_BINARY_DIR}/version.rc"
    )
endif()

add_executable(cppwinrt ${CPPWINRT_SRCS} ${CPPWINRT_RESOURCES} ${CPPWINRT_HEADERS})
target_compile_definitions(cppwinrt PRIVATE CPPWINRT_VERSION_STRING="${CPPWINRT_BUILD_VERSION}")
target_include_directories(cppwinrt PRIVATE ${PROJECT_BINARY_DIR})

if(WIN32)
    target_link_libraries(cppwinrt shlwapi)
endif()

install(TARGETS cppwinrt)


# HACK: Handle the xmllite import lib.
# mingw-w64 before commit 5ac1a2c is missing the import lib for xmllite. This
# checks whether the current build environment provides libxmllite.a, and
# generates the import lib if needed.

set(XMLLITE_LIBRARY xmllite)
if(MINGW)
    function(TestLinkXmlLite OUTPUT_VARNAME)
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_LIBRARIES xmllite)
        check_cxx_source_compiles("
#include <xmllite.h>
int main() {
    CreateXmlReader(__uuidof(IXmlReader), nullptr, nullptr);
}
        " ${OUTPUT_VARNAME})
    endfunction()

    function(TestIsI386 OUTPUT_VARNAME)
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles("
#if !defined(__i386__) && !defined(_M_IX86)
#  error Not i386
#endif
int main() {}
        " ${OUTPUT_VARNAME})
    endfunction()

    TestLinkXmlLite(HAS_LIBXMLLITE)
    if(NOT HAS_LIBXMLLITE)
        TestIsI386(TARGET_IS_I386)
        if(TARGET_IS_I386)
            set(XMLLITE_DEF_FILE xmllite_i386)
        else()
            set(XMLLITE_DEF_FILE xmllite)
        endif()
        include(CMakeFindBinUtils)
        add_custom_command(
            OUTPUT
                "${PROJECT_BINARY_DIR}/libxmllite.a"
            COMMAND "${CMAKE_DLLTOOL}" -k -d "${PROJECT_SOURCE_DIR}/mingw-support/${XMLLITE_DEF_FILE}.def" -l "${PROJECT_BINARY_DIR}/libxmllite.a"
            DEPENDS "${PROJECT_SOURCE_DIR}/mingw-support/${XMLLITE_DEF_FILE}.def"
            VERBATIM
        )
        add_custom_target(gen-libxmllite
            DEPENDS "${PROJECT_BINARY_DIR}/libxmllite.a"
        )
        set(XMLLITE_LIBRARY "${PROJECT_BINARY_DIR}/libxmllite.a")
        add_dependencies(cppwinrt gen-libxmllite)
    endif()
endif()
if(WIN32)
    target_link_libraries(cppwinrt "${XMLLITE_LIBRARY}")
endif()


# === winmd: External header-only library for reading winmd files ===

set(EXTERNAL_WINMD_INCLUDE_DIR "" CACHE PATH "Path to the include dir of an\
 external copy of the winmd library headers. Leave empty (default) to have\
 it downloaded as ExternalProject during build.")

if(EXTERNAL_WINMD_INCLUDE_DIR STREQUAL "")
    message(STATUS "The winmd library will be downloaded using ExternalProject.")
    include(ExternalProject)
    ExternalProject_Add(winmd
        GIT_REPOSITORY https://github.com/microsoft/winmd.git
        GIT_TAG 0f1eae3bfa63fa2ba3c2912cbfe72a01db94cc5a
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
    )
    add_dependencies(cppwinrt winmd)
    ExternalProject_Get_Property(winmd SOURCE_DIR)
    set(winmd_INCLUDE_DIR "${SOURCE_DIR}/src")
else()
message(STATUS "Using winmd library headers at ${EXTERNAL_WINMD_INCLUDE_DIR}")
    set(winmd_INCLUDE_DIR "${EXTERNAL_WINMD_INCLUDE_DIR}")
endif()
target_include_directories(cppwinrt PRIVATE "${winmd_INCLUDE_DIR}")


if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()
