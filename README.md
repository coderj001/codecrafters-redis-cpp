[![progress-banner](https://backend.codecrafters.io/progress/redis/9a24da7e-f318-43d5-a2a6-76bf645a21e1)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

This is a starting point for C++ solutions to the
["Build Your Own Redis" Challenge](https://codecrafters.io/challenges/redis).

In this challenge, you'll build a toy Redis clone that's capable of handling
basic commands like `PING`, `SET` and `GET`. Along the way we'll learn about
event loops, the Redis protocol and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Setup

To build and run this project, you will need:

1.  **CMake**: Ensure `cmake` is installed locally.
2.  **C++ Compiler**: A C++17 (or later) compatible compiler (e.g., Clang on macOS, GCC on Linux).
3.  **Node.js**: Required to run the end-to-end tests.
4.  **redis-cli** (Optional, for manual testing): A command-line tool for interacting with Redis.

### Installing `redis-cli`

- **macOS (Homebrew)**: `brew install redis`
- **Ubuntu/Debian**: `sudo apt-get install redis-tools`
- **Windows**: Download from [redis.io/download](https://redis.io/download)

# Build & Run

The easiest way to build and run the server is to use the provided script. This is the same script the CodeCrafters platform uses.

```bash
./your_program.sh
```

This script will:

1.  Configure the project with CMake into a `build/` directory.
2.  Build the C++ source code, producing an executable.
3.  Launch the server, which will listen on port 6379.

Once the server is running, you can submit your solution to CodeCrafters with `git push origin master`.

### Manual Build

If you prefer to run the build steps manually:

```bash
# 1. Create a build directory
mkdir -p build

# 2. Configure the project with CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 3. Build the executable
cmake --build build

# 4. Run the server
./build/redis_server
```

# Testing

This project includes an end-to-end test suite written in JavaScript that uses Node.js and Jest.

### Test Setup

Navigate to the tests directory and install the dependencies:

```bash
cd tests
npm install
```

### Running Tests

You can run tests from the project root or from within the `tests` directory.

**From the project root:**

```bash
# Builds and runs all e2e tests
./run-e2e-tests.sh

# Runs tests assuming the server is already built
./quick-test.sh
```

**From the `tests` directory:**

```bash
# Run all tests (Node.js client and redis-cli)
npm run test:all

# Run only the Node.js client-based tests
npm test

# Run only the redis-cli based tests
npm run test:cli

# Run tests in watch mode for active development
npm run test:watch
```

### Manual Testing

You can interact with your running server manually using `redis-cli` or `netcat`.

**Using `redis-cli`:**

```bash
# Connect to the server and run a single command
redis-cli -p 6379 PING

# Or, start an interactive session
redis-cli -p 6379
```

**Using `netcat`:**

Send raw RESP commands to the server.

```bash
nc localhost 6379
*1\r\n$4\r\nPING\r\n
```

You should receive `+PONG\r\n` in response.

# Codebase Overview

- `src/Server.cpp`: Program entry point. Manages the TCP socket, accepts client connections, and orchestrates the request/response cycle.
- `src/handle_command.cpp` & `include/handle_command.h`: Dispatch layer that maps parsed RESP commands to their C++ implementations (e.g., PING, ECHO, SET, GET).
- `src/resp_parser.cpp` & `include/resp_parser.h`: Implements the RESP (Redis Serialization Protocol) decoder.
- `include/resp_datatypes.h`: Data structures representing RESP types (Simple Strings, Bulk Strings, Arrays, etc.).
- `include/store_value.h`: Abstractions for the key-value store, including expiry metadata.
- `tests/`: End-to-end tests that validate server behavior against the Redis protocol.
- `CMakeLists.txt`: The main build configuration file for CMake.
- `your_program.sh`: The wrapper script used to build and run the server.

## Directory Structure

```
.
├── CMakeLists.txt            # Build configuration
├── src/                      # C++ source files
│   ├── Server.cpp
│   ├── handle_command.cpp
│   └── resp_parser.cpp
├── include/                  # Public headers
│   ├── handle_command.h
│   ├── resp_datatypes.h
│   └── resp_parser.h
├── tests/                    # E2E tests (JS)
├── your_program.sh           # Build & run wrapper used by platform
├── vcpkg.json                # Dependency manifest
└── README.md
```

## Extending Commands

To add a new command:

1.  Update the `if/else` chain in `handle_command.cpp` to recognize the new command (case-insensitively).
2.  Create a new handler function for the command.
3.  In the handler, validate the number of arguments and return a RESP Error (`-ERR ...\r\n`) on mismatch.
4.  Implement the command's logic.
5.  Encode the response using the appropriate RESP format and send it back to the client.

## Troubleshooting

- **Port already in use**: Check for another process using port 6379 (`lsof -i :6379`) and stop it, or modify the port in the source code.
- **Build errors**: If you encounter issues, try a clean build by removing the `build` directory (`rm -rf build`) and running the build script again.
- **`redis-cli` not found**: Ensure `redis-tools` is installed and that its location is in your system's `PATH`.
- **Tests fail**: Check the server logs in your terminal for errors. Verify the server is running on the correct port and that no other Redis instance is interfering.

# Contributing

Contributions are welcome! Please feel free to submit a pull request.

## How to Contribute

1.  Fork the repository.
2.  Create a new branch (`git checkout -b feature/your-feature-name`).
3.  Make your changes.
4.  Commit your changes (`git commit -m 'Add some feature'`).
5.  Push to the branch (`git push origin feature/your-feature-name`).
6.  Open a pull request.

# License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

# Code of Conduct

Please note that this project is released with a [Contributor Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project you agree to abide by its terms.
