#!/bin/bash
set -euo pipefail
SOURCE_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
AE_SDK_ROOT="${AE_SDK_ROOT:?AE_SDK_ROOT를 Adobe After Effects SDK 폴더로 지정하세요.}"
BUILD_DIR="${BUILD_DIR:-$SOURCE_ROOT/build/macos}"
[[ "$(uname -s)" == Darwin ]] || { echo 'macOS에서 빌드하세요.' >&2; exit 1; }
cmake -S "$SOURCE_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DAE_SDK_ROOT="$AE_SDK_ROOT" -DCMAKE_OSX_ARCHITECTURES='arm64;x86_64'
cmake --build "$BUILD_DIR" --parallel 4
ctest --test-dir "$BUILD_DIR" --output-on-failure
/usr/bin/codesign --verify --deep --strict "$BUILD_DIR/EdgeLightWrap.plugin"
/usr/bin/lipo "$BUILD_DIR/EdgeLightWrap.plugin/Contents/MacOS/EdgeLightWrap" -verify_arch arm64 x86_64
python3 -B "$SOURCE_ROOT/scripts/pipl.py" --sdk "$AE_SDK_ROOT" \
  --compare-rsrc "$BUILD_DIR/EdgeLightWrap.plugin/Contents/Resources/EdgeLightWrap.rsrc"
if /usr/sbin/pkgutil --pkg-info com.apple.pkg.RosettaUpdateAuto >/dev/null 2>&1; then
  /usr/bin/arch -x86_64 "$BUILD_DIR/ae-host-tests" "$BUILD_DIR/EdgeLightWrap.plugin/Contents/MacOS/EdgeLightWrap"
fi
