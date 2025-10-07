#!/bin/sh
set -e

# Navigate to the project root
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$PROJECT_ROOT"

# Copied from .codecrafters/compile.sh
#
# - Edit this to change how your program compiles locally
# - Edit .codecrafters/compile.sh to change how your program compiles remotely
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
cmake --build ./build

# Run Unit Testcases
echo "Running Unit Testcases: "
exec ./build/unit_tests "$@"

# Run Server
echo "Running Server: "
exec ./build/redis-server "$@"
