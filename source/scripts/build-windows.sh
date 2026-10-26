#!/bin/bash
# Cross-compile the Windows x64 .aex on macOS/Linux with MinGW-w64 (brew install mingw-w64).
set -euo pipefail
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
AE_SDK_ROOT="${AE_SDK_ROOT:?AE_SDK_ROOT를 Adobe After Effects SDK 폴더로 지정하세요.}"
BUILD_DIR="${BUILD_DIR:-$SOURCE_ROOT/build/windows}"
command -v x86_64-w64-mingw32-g++ >/dev/null || { echo 'MinGW-w64가 필요합니다: brew install mingw-w64' >&2; exit 1; }
cmake -S "$SOURCE_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE="$SOURCE_ROOT/cmake/mingw-w64-x86_64.cmake" -DAE_SDK_ROOT="$AE_SDK_ROOT"
cmake --build "$BUILD_DIR" --parallel 4
python3 -B "$SOURCE_ROOT/scripts/pipl.py" --dump-pe "$BUILD_DIR/EdgeLightWrap.aex"
echo "$BUILD_DIR/EdgeLightWrap.aex"
