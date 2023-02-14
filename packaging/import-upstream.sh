#!/bin/bash

# SPDX-FileCopyrightText: 2022 Alvin Wong
#
# SPDX-License-Identifier: MIT

set -e
set -x

CPPWINRT_COMMIT=3bbee2cf2c8da944a29f493c42f06ce57086f28f
WINMD_COMMIT=0f1eae3bfa63fa2ba3c2912cbfe72a01db94cc5a

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

if [[ -n "$(git status --porcelain cppwinrt/)" ]]; then
    echo "$PWD/cppwinrt/ contains uncommitted changes!"
    exit 1
fi

mkdir -p .import/

if [[ -d .import/cppwinrt ]]; then
    pushd .import/cppwinrt
    git fetch
else
    git clone https://github.com/microsoft/cppwinrt.git .import/cppwinrt
    pushd .import/cppwinrt
fi
git checkout $CPPWINRT_COMMIT
[[ -z "$(git status --porcelain)" ]]
popd

if [[ -d .import/winmd ]]; then
    pushd .import/winmd
    git fetch
else
    git clone https://github.com/microsoft/winmd.git .import/winmd
    pushd .import/winmd
fi
git checkout $WINMD_COMMIT
[[ -z "$(git status --porcelain)" ]]
popd

if [[ -d cppwinrt ]]; then
    mv cppwinrt cppwinrt.old
fi

mkdir cppwinrt

for dir in cppwinrt mingw-support prebuild strings; do
    cp -r .import/cppwinrt/$dir cppwinrt/$dir
done

for file in CMakeLists.txt LICENSE README.md cross-mingw-toolchain.cmake; do
    cp .import/cppwinrt/$file cppwinrt/
done

mkdir cppwinrt/winmd
cp -r .import/winmd/src cppwinrt/winmd/src
cp .import/winmd/LICENSE cppwinrt/winmd/

echo
echo Done.
