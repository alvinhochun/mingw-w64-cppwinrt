# SPDX-FileCopyrightText: 2022 Alvin Wong
#
# SPDX-License-Identifier: MIT


cmake_minimum_required(VERSION 3.16)

# The project version should follow upstream cppwinrt, except when we have
# to include commits which haven't made it into an official release yet.
project(mingw-w64-cppwinrt
    VERSION 2.0.221215.0
    LANGUAGES CXX
)

include(GNUInstallDirs)


# === Include cppwinrt executable target ===

set(BUILD_TESTING FALSE)
set(CPPWINRT_BUILD_VERSION "${PROJECT_VERSION}")
set(CPPWINRT_BUILD_VERSION "${PROJECT_VERSION}" CACHE STRING "The version string used for cppwinrt." FORCE)

add_subdirectory(cppwinrt)

unset(CPPWINRT_BUILD_VERSION CACHE)


# === Include headers ===
# Select prebuilt headers if available (release source archive), otherwise
# use a native build of cppwinrt to generate the headers from the winmd file.

if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/prebuilt-headers")
    message(STATUS "Installing bundled prebuilt headers")
    add_custom_target(build-cppwinrt-headers)
    set(CPPWINRT_HEADERS_DIR "${PROJECT_SOURCE_DIR}/prebuilt-headers")
else()
    message(STATUS "Bundled prebuilt headers not found, generating fresh headers from metadata with cppwinrt")
    if(CMAKE_CROSSCOMPILING)
        include(ExternalProject)
        ExternalProject_Add(native-cppwinrt
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/cppwinrt"
            CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> "-DCPPWINRT_BUILD_VERSION=${CPPWINRT_BUILD_VERSION}"
        )
        ExternalProject_Get_Property(native-cppwinrt INSTALL_DIR)
        set(NATIVE_CPPWINRT_TOOL "${INSTALL_DIR}/bin/cppwinrt")
        unset(INSTALL_DIR)
    else()
        set(NATIVE_CPPWINRT_TOOL cppwinrt)
        add_custom_target(native-cppwinrt
            DEPENDS cppwinrt
        )
    endif()
    add_custom_command(
        OUTPUT
            "${PROJECT_BINARY_DIR}/headers.gen/winrt/base.h"
            "${PROJECT_BINARY_DIR}/headers.gen/winrt/Windows.Foundation.h"
        COMMAND
            ${NATIVE_CPPWINRT_TOOL}
            -input "${PROJECT_SOURCE_DIR}/metadata/Windows.winmd"
            -output "${PROJECT_BINARY_DIR}/headers.gen"
            -verbose
        DEPENDS native-cppwinrt
        VERBATIM
    )
    add_custom_target(build-cppwinrt-headers ALL
        DEPENDS "${PROJECT_BINARY_DIR}/headers.gen/winrt/base.h"
    )
    set(CPPWINRT_HEADERS_DIR "${PROJECT_BINARY_DIR}/headers.gen")
endif()


# === Install interface target and necessary files ===

set(BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/cppwinrt")
set(WINMD_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cppwinrt/metadata")
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cppwinrt/cmake")
set(LICENSE_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/licenses/cppwinrt")

add_library(CppWinRT_Lib INTERFACE)
add_dependencies(CppWinRT_Lib build-cppwinrt-headers)
target_include_directories(CppWinRT_Lib
    INTERFACE
        $<BUILD_INTERFACE:${CPPWINRT_HEADERS_DIR}>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)

install(
    TARGETS CppWinRT_Lib
    EXPORT CppWinRT_Targets
)
install(
    EXPORT CppWinRT_Targets
    NAMESPACE CppWinRT::
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    FILE CppWinRTTargets.cmake
)
install(CODE "message(STATUS \"Installing headers to: ${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR}\")")
install(
    DIRECTORY ${CPPWINRT_HEADERS_DIR}/
    DESTINATION ${INCLUDE_INSTALL_DIR}
    MESSAGE_NEVER
)

install(
    FILES "${PROJECT_SOURCE_DIR}/metadata/Windows.winmd"
    DESTINATION ${WINMD_INSTALL_DIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(CppWinRTConfig.cmake.in
    ${PROJECT_BINARY_DIR}/CppWinRTConfig.cmake
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
    PATH_VARS
        BIN_INSTALL_DIR
        INCLUDE_INSTALL_DIR
        WINMD_INSTALL_DIR
)
write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/CppWinRTConfigVersion.cmake
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion
    ARCH_INDEPENDENT
)
install(
    FILES
        ${PROJECT_BINARY_DIR}/CppWinRTConfig.cmake
        ${PROJECT_BINARY_DIR}/CppWinRTConfigVersion.cmake
    DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

# License files:
install(
    FILES ${PROJECT_SOURCE_DIR}/cppwinrt/LICENSE
    DESTINATION ${LICENSE_INSTALL_DIR}/cppwinrt
)
install(
    FILES ${PROJECT_SOURCE_DIR}/cppwinrt/winmd/LICENSE
    DESTINATION ${LICENSE_INSTALL_DIR}/winmd
)
install(
    FILES
        ${PROJECT_SOURCE_DIR}/metadata/license-apache-2.0
        ${PROJECT_SOURCE_DIR}/metadata/license-mit
    DESTINATION ${LICENSE_INSTALL_DIR}/metadata
)
if(CMAKE_PROJECT_NAME STREQUAL "mingw-w64-cppwinrt_toplevel-placeholder")
    install(
        FILES ${PROJECT_SOURCE_DIR}/../LICENSE
        DESTINATION ${LICENSE_INSTALL_DIR}/
    )
else()
    install(
        FILES ${PROJECT_SOURCE_DIR}/LICENSE
        DESTINATION ${LICENSE_INSTALL_DIR}/
    )
endif()