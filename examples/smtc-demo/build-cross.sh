#!/bin/sh

set -e
set -x

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -GNinja
cmake --build build
