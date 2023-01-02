# SPDX-FileCopyrightText: 2022 Alvin Wong
#
# SPDX-License-Identifier: MIT


cmake_minimum_required(VERSION 3.16)

project(smtc-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -municode")
add_compile_definitions(NOMINMAX)

function(TestIsX64 OUTPUT_VARNAME)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("
#if !defined(__x86_64__) && !defined(_M_X64)
#  error Not x86_64
#endif
    " ${OUTPUT_VARNAME})
endfunction()
TestIsX64(TARGET_IS_X64)
if(TARGET_IS_X64)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcx16")
endif()

# This is assuming you've built and installed the library with the given
# local-build.sh, but you may also point to a custom install by setting
# CMAKE_PREFIX_PATH from the cmake command line, or install it system-wide.
list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/../../build/install")

# First you need to tell CMake to find the library:
find_package(CppWinRT)

# You can check the status with CppWinRT_FOUND:
if(NOT CppWinRT_FOUND)
    message(FATAL_ERROR "Missing required dependency CppWinRT!")
endif()

# Create targets normally.
add_executable(smtc-demo main.cpp)

# After `find_package(CppWinRT)`, this is all you need to use CppWinRT:
target_link_libraries(smtc-demo PRIVATE CppWinRT::CppWinRT)

# This showcases some of the variables provided by `find_package(CppWinRT)`:
include(CMakePrintHelpers)
cmake_print_variables(CppWinRT_FOUND)
cmake_print_variables(CppWinRT_TOOL_PATH)
cmake_print_variables(CppWinRT_METADATA_PATH)
cmake_print_variables(CppWinRT_INCLUDE_DIR)
