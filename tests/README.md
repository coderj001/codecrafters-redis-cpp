# Redis Server E2E Tests

This directory contains end-to-end tests for the Redis server implementation.

## Setup

1. Make sure Node.js is installed
2. Install dependencies: `npm install`
3. Make sure `redis-cli` is installed (see below)

## Installing redis-cli

### macOS

```bash
brew install redis
```

### Ubuntu/Debian

```bash
sudo apt-get install redis-tools
```

### Windows

Download from: https://redis.io/download

## Running Tests

### Full E2E Test Suite

```bash
# From project root
./run-e2e-tests.sh
```

This will:

- Build the Redis server
- Start the server
- Run all tests
- Clean up

### Quick Tests (assumes server is built)

```bash
./quick-test.sh
```

### Individual Test Types

#### Node.js Redis Client Tests

```bash
cd tests
npm run test
```

#### redis-cli Tests

```bash
cd tests
npm run test:cli
```

#### All Tests

```bash
cd tests
npm run test:all
```

### Watch Mode (for development)

```bash
cd tests
npm run test:watch
```

## Test Structure

- `test-server.js`: Tests using Node.js Redis client library
- `test-redis-cli.js`: Tests using redis-cli commands
- `package.json`: Test configuration and scripts

## Test Coverage

The tests cover:

- Basic SET/GET operations
- Non-existent key handling
- Value overwriting
- Multiple key operations
- Error handling for malformed commands

## Troubleshooting

### Server won't start

- Make sure the build completed successfully
- Check that port 6379 is not already in use
- Verify VCPKG_ROOT is set correctly

### redis-cli not found

- Install redis-cli as described above
- Make sure it's in your PATH

### Tests fail

- Check server logs for errors
- Verify the server is running on port 6379
- Make sure no other Redis instance is running
