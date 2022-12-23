# SPDX-FileCopyrightText: 2022 Alvin Wong
#
# SPDX-License-Identifier: MIT


cmake_minimum_required(VERSION 3.16)

# The project version should follow upstream cppwinrt, except when we have
# to include commits which haven't made it into an official release yet.
project(mingw-w64-cppwinrt
    VERSION 2.0.221221.0
    HOMEPAGE_URL https://github.com/alvinhochun/mingw-w64-cppwinrt
    LANGUAGES CXX
)

include(GNUInstallDirs)


include(FeatureSummary)
option(INSTALL_CPPWINRT_TOOL "Install the cppwinrt binary tool to the output" ON)
add_feature_info(INSTALL_CPPWINRT_TOOL INSTALL_CPPWINRT_TOOL "Install the cppwinrt binary tool")
option(INSTALL_WINDOWS_WINMD "Install the Windows.winmd metadata file to the output" ON)
add_feature_info(INSTALL_WINDOWS_WINMD INSTALL_WINDOWS_WINMD "Install the Windows.winmd metadata file")


# === Include cppwinrt executable target ===

set(BUILD_TESTING FALSE)
set(CPPWINRT_BUILD_VERSION "${PROJECT_VERSION}")
set(CPPWINRT_BUILD_VERSION "${PROJECT_VERSION}" CACHE STRING "The version string used for cppwinrt." FORCE)

if(INSTALL_CPPWINRT_TOOL)
    add_subdirectory(cppwinrt)
else()
    add_subdirectory(cppwinrt EXCLUDE_FROM_ALL)
endif()

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
        set(NATIVE_CPPWINRT_TOOL_DEPS native-cppwinrt)
        unset(INSTALL_DIR)
    else()
        set(NATIVE_CPPWINRT_TOOL cppwinrt)
        set(NATIVE_CPPWINRT_TOOL_DEPS cppwinrt)
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
        DEPENDS ${NATIVE_CPPWINRT_TOOL_DEPS}
        VERBATIM
    )
    add_custom_target(build-cppwinrt-headers ALL
        DEPENDS "${PROJECT_BINARY_DIR}/headers.gen/winrt/base.h"
    )
    set(CPPWINRT_HEADERS_DIR "${PROJECT_BINARY_DIR}/headers.gen")
endif()


# === Install interface target and necessary files ===

set(BIN_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/cppwinrt-${PROJECT_VERSION}")
set(WINMD_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cppwinrt-${PROJECT_VERSION}/metadata")
set(CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cppwinrt-${PROJECT_VERSION}/cmake")
set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig")
set(LICENSE_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/licenses/cppwinrt")

add_library(CppWinRT_Lib INTERFACE)
add_dependencies(CppWinRT_Lib build-cppwinrt-headers)
target_include_directories(CppWinRT_Lib
    INTERFACE
        $<BUILD_INTERFACE:${CPPWINRT_HEADERS_DIR}>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)
target_compile_features(CppWinRT_Lib
    INTERFACE cxx_std_20
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

if(INSTALL_WINDOWS_WINMD)
    install(
        FILES "${PROJECT_SOURCE_DIR}/metadata/Windows.winmd"
        DESTINATION ${WINMD_INSTALL_DIR}
    )
endif()

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

configure_file(CppWinRT.pc.in ${PROJECT_BINARY_DIR}/CppWinRT.pc @ONLY)
install(
    FILES ${PROJECT_BINARY_DIR}/CppWinRT.pc
    DESTINATION ${PKGCONFIG_INSTALL_DIR}
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

feature_summary(WHAT ALL)
