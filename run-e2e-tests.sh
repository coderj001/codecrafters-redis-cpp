#!/bin/bash

# Build the Redis server
echo "Building Redis server..."
cd "$(dirname "$0")/.."
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
cmake --build ./build

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Check if redis-cli is available
if ! command -v redis-cli &> /dev/null; then
    echo "redis-cli not found. Please install Redis CLI to run these tests."
    echo "On macOS: brew install redis"
    echo "On Ubuntu: sudo apt-get install redis-tools"
    exit 1
fi

# Run the server in background
echo "Starting Redis server..."
./build/server &
SERVER_PID=$!

# Wait for server to start
sleep 3

# Function to cleanup on exit
cleanup() {
    echo "Cleaning up..."
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null
    echo "Done."
}

# Set trap to cleanup on script exit
trap cleanup EXIT

# Run tests
echo "Running e2e tests..."
cd tests

echo "Running Node.js Redis client tests..."
npm run test

if [ $? -ne 0 ]; then
    echo "Node.js client tests failed!"
    exit 1
fi

echo "Running redis-cli tests..."
npm run test:cli

if [ $? -ne 0 ]; then
    echo "redis-cli tests failed!"
    exit 1
fi

echo "🎉 All tests passed!"
