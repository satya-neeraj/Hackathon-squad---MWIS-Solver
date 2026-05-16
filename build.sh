#!/usr/bin/env bash
# Build the MWIS solver. Static linking is used to avoid MinGW DLL conflicts.
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build

CXX=${CXX:-g++}
FLAGS="-O2 -pipe -std=c++17 -DNDEBUG -static -static-libgcc -static-libstdc++"

echo "Building solve (this takes a few seconds)..."
$CXX $FLAGS solve.cpp -o build/solve

echo "Done."
ls -la build/
