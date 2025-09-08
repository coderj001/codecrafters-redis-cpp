# Copilot Instructions

These instructions guide GitHub Copilot when generating code, tests, or documentation for this repository.

---

## 📂 Repository Purpose
This repository is a C++ implementation scaffold for the ["Build Your Own Redis" challenge](https://codecrafters.io/challenges/redis).  

The goal is to build a **toy Redis server** capable of handling commands (`PING`, `ECHO`, `SET`, `GET`, …) using the **RESP protocol**.

---

## 🧑‍💻 Coding Conventions
- **Language:** Modern C++ (C++17 or later, currently using C++23).
- **Style:**
  - Use RAII principles and smart pointers (no raw `new/delete`).
  - Prefer `std::unordered_map`, `std::string`, `std::vector`, and standard library facilities.
  - Follow consistent naming:  
    - Functions & variables → `snake_case`  
    - Classes & structs → `PascalCase`  
  - Keep functions small and focused.
- **Error handling:** Return RESP `-ERR ...\r\n` for invalid commands/arguments.
- **Testing:** End-to-end tests live under `tests/` (Node + Jest). Any new command should be tested by extending these where possible.

---

## 📦 Project Structure
- `src/Server.cpp` → Entry point. TCP socket setup, accept loop, request/response cycle.
- `src/handle_command.cpp` / `src/include/handle_command.h` → Dispatch layer: maps RESP arrays → Redis command execution.
- `src/resp_parser.cpp` / `src/include/resp_parser.h` → RESP protocol decoder (Arrays, Bulk Strings, Integers, etc.).
- `src/include/resp_datatypes.h` → RESP type definitions (structs/enums).
- `src/include/store_value.h` → Data storage abstractions for Redis key-value operations.
- `tests/` → JavaScript e2e tests that validate server behavior (`cli.e2e.test.js`, `server.e2e.test.js`, etc.).
- `CMakeLists.txt` + `vcpkg.json` → Build configuration and dependency management.
- `your_program.sh` → Wrapper script: builds with CMake + runs server.

---

## 🔄 Development Workflow
- All code changes go inside `src/` + `src/include/`.
- Run the server locally with:
  ```bash
  ./your_program.sh
  ```
- Build the project with:
  ```bash
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  cmake --build ./build
  ```
- Run tests with:
  ```bash
  cd tests && npm test
  ```

---

## 🤖 Copilot Usage

When generating code:

- Respect the above file structure (don't create new random files).
- Implement one command at a time (e.g. `PING`, `ECHO`, `SET`, `GET`).
- Default to blocking sockets unless explicitly refactoring to event loop.
- Suggest incremental RESP parsing (handle partial buffers).
- Generate RESP-compliant responses (e.g. `+OK\r\n`, `$3\r\nfoo\r\n`).
- Propose minimal, clear unit/e2e test extensions in `tests/`.

---

## ✅ Example Expectations

**Adding GET command:**
- Update `handle_command.cpp` to check if command == "GET".
- Lookup in in-memory map; if not found, return `$-1\r\n` (RESP Null Bulk String).
- Add test to `server.e2e.test.js`: GET key → expected value.

**Extending parser:**
- Add handling for RESP Integers (`:123\r\n`) in `resp_parser.cpp`.
- Update `resp_datatypes.h` with `RespType::Integer`.

---

## 📚 Additional Notes

- Keep `README.md` updated if server behavior changes.
- Prefer clarity over optimization (challenge is educational).
- If unsure of implementation details, generate scaffolding with TODOs.
- Do not introduce new dependencies unless added in `vcpkg.json`.
- Use threading for concurrent client handling as shown in `Server.cpp`.
- Follow existing patterns for RESP data type handling using polymorphic classes.
- Command names should be case-insensitive (convert to uppercase in handlers).
- Use mutex protection for shared data structures like the key-value store.