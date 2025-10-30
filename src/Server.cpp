#include "./include/handle_command.h"
#include "./include/resp_parser.h"

// Provides definitions for internet operations, like converting between host and
// network byte order (e.g., htons).
#include <arpa/inet.h>
// C standard library. Provides general utilities like program termination
// (e.g., EXIT_SUCCESS).
#include <cstdlib>
// C++ standard library for input/output streams (e.g., std::cout, std::cerr).
#include <iostream>
// Part of the C++ I/O library, provides ostream and related functionality like
// std::unitbuf.
#include <ostream>
// Core Berkeley sockets API. Defines functions like socket(), bind(), listen(),
// and accept() for network communication.
#include <sys/socket.h>
// Defines various data types used in system calls, often required by other
// system headers like <sys/socket.h>.
#include <sys/types.h>
// C++ standard library for creating and managing threads (e.g., std::thread).
#include <thread>
// POSIX standard header. Provides access to the OS API, including functions
// like read(), write(), and close().
#include <unistd.h>

void handle_client_connection(int client_fd) {
  // This function is executed by a new thread for each client connection.
  // The `main` function creates a `std::thread` and runs this function on it.
  // This allows the server to handle multiple clients concurrently.
  // `std::this_thread::get_id()` returns the unique ID of the current thread.
  std::cout << "Client connected on thread " << std::this_thread::get_id()
            << std::endl;

  // A mutex (mutual exclusion) is a lock used to protect shared data.
  // When multiple threads need to access the same data, a mutex ensures
  // that only one thread can access it at a time, preventing "race conditions"
  // where threads might corrupt the data by modifying it simultaneously.
  //
  // IMPORTANT NOTE: In this specific code, `input_mutex` and the `accumulated_input_buffer` string
  // are local variables. Each thread gets its own separate copy. Therefore,
  // this mutex is not actually protecting any data shared between threads.
  // It's effectively redundant here, but serves as an example of how you
  // *would* declare a mutex. A real use case would involve a mutex that is
  // shared (e.g., global or passed by reference) among multiple threads.
  std::mutex input_mutex;
  std::string accumulated_input_buffer;
  char receive_buffer[1024];

  while (true) {
    ssize_t received_byte_count = read(client_fd, receive_buffer, sizeof(receive_buffer));
    if (received_byte_count <= 0) {
      if (received_byte_count < 0) {
        std::cerr << "Failed to read\n";
      }
      break; // Client disconnected or error occurred.
    }

    // `std::lock_guard` is a helper class that automatically manages a mutex.
    // When `lock` is created, it calls `input_mutex.lock()`, acquiring the lock.
    // This means any other thread trying to lock the same mutex will have to wait.
    // The block of code that follows is the "critical section".
    {
      std::lock_guard<std::mutex> lock(input_mutex);
      accumulated_input_buffer.append(receive_buffer, received_byte_count);
    } // The lock is automatically released here when `lock` goes out of scope.
      // This RAII (Resource Acquisition Is Initialization) pattern is the
      // recommended way to handle mutexes, as it guarantees the lock is
      // released even if an exception occurs.

    // This inner loop processes the accumulated data from the `accumulated_input_buffer` buffer.
    while (!accumulated_input_buffer.empty()) {
      try {
        RESPParser parser(accumulated_input_buffer);
        auto parsed_resp_data = parser.parse();
        if (!parsed_resp_data) {
          // Not enough data to parse a full command, wait for more.
          break;
        }

        auto resp_array = std::dynamic_pointer_cast<Arrays>(parsed_resp_data);
        if (!resp_array) {
          std::cerr << "Expected RESP Array\n";
          break;
        }

        std::vector<std::string> command_parts;
        for (auto &array_element : resp_array->values) {
          auto bulk_string_element = std::dynamic_pointer_cast<BulkStrings>(array_element);
          if (bulk_string_element) {
            command_parts.push_back(bulk_string_element->value);
          }
        }

        // This function likely processes the command. If it accesses a shared
        // resource (like a global key-value store), it must use its own
        // mutexes internally to ensure thread safety.
        handleCommand(command_parts, client_fd);

        // Remove the processed command from the input buffer.
        accumulated_input_buffer.erase(0, parser.bytesConsumed());
      } catch (const std::exception &e) {
        std::cerr << "Parsing error: " << e.what() << "\n";
        break; // Stop processing on error.
      }
    }
  }

  // Clean up by closing the client's socket connection.
  close(client_fd);
}

int main(int argc, char **argv) {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
      0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "Listen failed\n";
    return 1;
  }

  while (true) {
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                           (socklen_t *)&client_addr_len);

    if (client_fd < 0) {
      std::cerr << "Failed to accept client connection\n";
    }

    std::thread client_thread(handle_client_connection, client_fd);
    client_thread.detach();
  }

  close(server_fd);
  return EXIT_SUCCESS;
}
