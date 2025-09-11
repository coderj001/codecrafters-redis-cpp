#!/bin/sh
# Debug build script

set -e # Exit early if any commands fail

echo "Building in Debug mode..."

(
  cd "$(dirname "$0")"
  # Clean previous build
  rm -rf build
  
  # Configure with Debug mode
  cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  
  # Build
  cmake --build ./build
)

echo "Debug build complete. Binary: ./build/src/server"
echo "To debug: gdb ./build/src/server"