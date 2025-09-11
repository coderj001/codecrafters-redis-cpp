#!/bin/sh
#
# Use this script to run your unit tests LOCALLY.
#

set -e # Exit early if any commands fail

# Build the tests
(
  cd "$(dirname "$0")"
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  cmake --build ./build
)

# Run the tests
(
  cd "$(dirname "$0")/build"
  ctest --output-on-failure
)
