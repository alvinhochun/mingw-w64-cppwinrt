#!/bin/bash

set -e
set -x

if [[ -z "$1" ]]; then
    echo "Usage: $0 <output_path>"
    exit 1
fi

if [[ -e "$1" ]]; then
    echo "Output path already exists."
    exit 1
fi

mkdir -p "$1"
pushd "$1"
outdir="$PWD"
popd

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

workspace="$(mktemp -d)"

if [[ -z "$workspace" ]]; then
    echo "Error: Did not get a temp directory."
    exit 1
fi

cleanup() {
    rm -rf "$workspace"
}

trap cleanup EXIT

mkdir -p "$outdir"

for dir in cppwinrt metadata; do
    cp -r "$dir" "$outdir/$dir"
done

for file in CMakeLists.txt *.cmake.in; do
    cp "$file" "$outdir/"
done
cp ../LICENSE "$outdir/"

cmake -S . -B "$workspace/cppwinrt-build" -GNinja
cmake --build "$workspace/cppwinrt-build" --target cppwinrt
"$workspace/cppwinrt-build/cppwinrt/cppwinrt" \
    -in metadata/Windows.winmd \
    -out "$workspace/cppwinrt-headers" \
    -verbose
cp -r "$workspace/cppwinrt-headers" "$outdir/prebuilt-headers"
