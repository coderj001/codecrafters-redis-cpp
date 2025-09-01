[![progress-banner](https://backend.codecrafters.io/progress/redis/9a24da7e-f318-43d5-a2a6-76bf645a21e1)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

This is a starting point for C++ solutions to the
["Build Your Own Redis" Challenge](https://codecrafters.io/challenges/redis).

In this challenge, you'll build a toy Redis clone that's capable of handling
basic commands like `PING`, `SET` and `GET`. Along the way we'll learn about
event loops, the Redis protocol and more.

**Note**: If you're viewing this repo on GitHub, head over to
[codecrafters.io](https://codecrafters.io) to try the challenge.

# Setup

Note: This section is for stages 2 and beyond.

1. Ensure you have `cmake` installed locally
2. Run `./your_program.sh` to run your Redis server, which is implemented in
   `src/Server.cpp`.
3. Commit your changes and run `git push origin master` to submit your solution
   to CodeCrafters. Test output will be streamed to your terminal.

## Codebase Overview

This repository is a minimal C++ implementation scaffold for a Redis-compatible server. Core pieces:

- `src/Server.cpp`: Program entry point. Responsible for:
  - Creating and binding a TCP listening socket (per challenge instructions)
  - Accepting client connections (blocking for early stages; later you'll add evented I/O)
  - Reading raw bytes from the socket and delegating to the RESP parser / command handler
- `src/handle_command.cpp` & `include/handle_command.h`: Dispatch layer turning parsed RESP Arrays into command responses (e.g. PING, ECHO, SET, GET). Extend this as new commands are required by later stages.
- `src/resp_parser.cpp` & `include/resp_parser.h`: Implements a minimal RESP (Redis Serialization Protocol) decoder (and possibly helpers for encoding). It should incrementally parse incoming buffers into strongly-typed RESP values.
- `include/resp_datatypes.h`: Data structures (enums / structs / variants) representing RESP types (Simple Strings, Bulk Strings, Integers, Arrays, Errors, Nulls). Central to type-safe command handling.
- `CMakeLists.txt`: Build configuration (targets, C++ standard, include paths, dependency linkage through vcpkg if needed).
- `vcpkg.json` / `vcpkg-configuration.json`: Declares external C/C++ dependencies resolved via vcpkg (currently likely empty or minimal for early stages).
- `your_program.sh`: Wrapper script executed by the CodeCrafters platform. It configures & builds (via CMake) then launches the compiled server.
- `tests/`: JavaScript end-to-end tests (Node + `redis-cli` style interactions) executed by the platform to validate protocol behavior. Not compiled into your binary; they exercise the running server.

### Execution Flow (High-Level)

1. Process starts in `main()` inside `src/Server.cpp` (or equivalent initialization code there).
2. Server sets up a listening socket, then waits for a client connection.
3. Upon receiving bytes, data is fed into the RESP parser producing a `RespValue` (or similar).
4. Parsed command array is passed to `handle_command(...)` which:
   - Validates arity
   - Executes logic (e.g. respond with `+PONG\r\n` for PING)
   - Returns an encoded RESP reply string/bytes.
5. Reply bytes are written back to the client socket.

Later stages introduce:

- In-memory key-value store (map) with expiry metadata
- Concurrency / multiplexing via `select`, `poll`, or `epoll`-like abstractions (macOS: `kqueue` or `select` for simplicity)
- Persistence / replication features (depending on challenge scope)

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

## Build & Run Locally

Prerequisites:

- CMake (>=3.16 recommended)
- A C++17 (or later) compiler (clang on macOS is fine)
- `vcpkg` (automatically bootstrapped by CodeCrafters scripts; local optional if you add dependencies)

Quick start (macOS / Linux):

```
./your_program.sh
```

This script will:

1. Configure a build directory (usually `build/`)
2. Invoke `cmake --build` producing the executable (often named `redis_server` or similar)
3. Launch the server listening on the required port (default from challenge spec, typically 6379 or a provided env var)

Manual build (if you prefer explicit steps):

```
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/redis_server
```

## Build System (CMake + vcpkg)

This project does NOT use a hand-written Makefile or `make build` target. Instead it relies on:

- CMake (project generator & build orchestration)
- vcpkg (manifest mode) for dependency acquisition via `vcpkg.json`
- Wrapper script `./your_program.sh` that encapsulates: configure + build + run

What `./your_program.sh` conceptually does:

1. (Optional) Bootstraps vcpkg if not present (handled by the CodeCrafters environment)
2. Runs `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake` so CMake knows to integrate vcpkg
3. Invokes `cmake --build build` (which internally may call `ninja` or `make` depending on generator chosen automatically by CMake)
4. Executes the produced binary

So while an underlying tool like `make` or `ninja` might still execute, you don't manually call it—CMake abstracts that away.

### Adding a New Dependency

1. Add it to `vcpkg.json` under the `dependencies` array.
2. Re-run `./your_program.sh` (CMake + vcpkg will install & integrate it automatically).
3. Include headers / link usage normally; CMake target usage is typically automatic in manifest mode if you use `find_package` or provided config packages.

### Cleaning the Build

If you need a clean slate:

```
rm -rf build
./your_program.sh
```

This forces a full re-configure.

### Switching Generators

If you want to force Ninja (faster incremental builds):

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

You can still run the binary directly afterward.

Then connect from another terminal:

```
nc localhost 6379
*1\r\n$4\r\nPING\r\n
```

You should receive:

```
+PONG
```

## Extending Commands

To add a command:

1. Update `handle_command.cpp` switch/if chain to recognize the command name (uppercase) from the parsed array.
2. Validate argument count; return a RESP Error (`-ERR ...\r\n`) on mismatch.
3. Implement logic, possibly updating in-memory state (add a global/store singleton or pass a state object).
4. Encode response using RESP conventions (Simple String, Bulk String, Integer, etc.).

## RESP Parsing Notes

RESP forms you'll commonly handle early:

- Simple String: `+OK\r\n`
- Error: `-ERR msg\r\n`
- Integer: `:1000\r\n`
- Bulk String: `$5\r\nhello\r\n`
- Array: `*2\r\n$4\r\nECHO\r\n$5\r\nhello\r\n`

Parser strategy tips:

- Accumulate incoming bytes; only finalize when complete terminators (`\r\n`) or length-specified payloads are fully present.
- Represent partial state if you later move to non-blocking sockets.

## Testing

The platform runs the JS tests in `tests/`. For ad-hoc checks you can use `redis-cli` (if installed) or `nc`:

```
redis-cli -p 6379 PING
```

### Manual Testing with redis-cli

You have two easy options if `redis-cli` isn't already installed:

1. Install Redis locally (provides `redis-cli`):

   - macOS (Homebrew): `brew install redis`
   - Linux (Debian/Ubuntu): `sudo apt-get update && sudo apt-get install -y redis-tools`
   - Then run: `redis-cli -p 6379 PING` or start an interactive session: `redis-cli -p 6379`

2. Use a temporary CLI via npx (no local install needed, requires Node.js):
   - One-off command: `npx redis-cli -p 6379 PING`
   - Interactive: `npx redis-cli -p 6379`

Inside the interactive prompt you can try:

```
PING
SET mykey hello
GET mykey
ECHO hi
```

Expected responses (once those commands are implemented) resemble:

```
PONG
OK
"hello"
"hi"
```

## Common Next Improvements

- Introduce a `KeyValueStore` module for SET/GET
- Add expiration handling (store expiry timestamps, prune on access)
- Switch to non-blocking sockets + `select()` loop
- Implement concurrency-safe data structures if you add threads (optional; single-thread event loop suffices initially)

## Troubleshooting

- Port already in use: change or free it (`lsof -i :6379`).
- Build errors about missing headers: ensure `cmake` picked up the correct compiler and run a clean build (`rm -rf build`).
- Parser returning unexpected types: log raw incoming buffer & parsed token sequence.

---

Feel free to adapt the structure as the challenge advances; keep the README updated so newcomers (and your future self) can navigate quickly.
