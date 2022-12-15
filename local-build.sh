#!/bin/sh

set -e
set -x

cmake -S . -B build/native \
    -DCMAKE_INSTALL_PREFIX="$PWD/build/install" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_EXE_LINKER_FLAGS=-static \
    -GNinja
cmake --build build/native
cmake --install build/native
