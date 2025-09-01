#!/bin/bash

# Quick test runner for development
# This script assumes the server is already built

# Check if server binary exists
if [ ! -f "./build/server" ]; then
    echo "Server binary not found. Please run ./run-e2e-tests.sh first to build it."
    exit 1
fi

# Check if redis-cli is available
if ! command -v redis-cli &> /dev/null; then
    echo "redis-cli not found. Please install Redis CLI."
    exit 1
fi

# Start server in background
echo "Starting Redis server..."
./build/server &
SERVER_PID=$!

# Wait for server
sleep 2

# Run quick tests
echo "Running quick redis-cli tests..."

# Test 1: Basic SET/GET
echo "Test 1: SET/GET"
redis-cli -p 6379 SET quick_test "Hello World" > /dev/null
RESULT=$(redis-cli -p 6379 GET quick_test)
if [ "$RESULT" = "Hello World" ]; then
    echo "✓ SET/GET test passed"
else
    echo "✗ SET/GET test failed"
    kill $SERVER_PID
    exit 1
fi

# Test 2: Non-existent key
echo "Test 2: Non-existent key"
RESULT=$(redis-cli -p 6379 GET nonexistent_key)
if [ "$RESULT" = "(nil)" ] || [ -z "$RESULT" ]; then
    echo "✓ Non-existent key test passed"
else
    echo "✗ Non-existent key test failed"
    kill $SERVER_PID
    exit 1
fi

# Cleanup
kill $SERVER_PID
echo "Quick tests completed successfully!"
