#!/bin/bash
set -e

# Start redis in background
build/redis-server &
REDIS_PID=$!

# Navigate to the tests directory
cd "$(dirname "$0")/../tests"

# npm install
npm test
# npm test -- --detectOpenHandles

# Ensure redis is stopped when script exits (success or error)
trap "kill $REDIS_PID" EXIT
