#include "./include/handle_command.h"
#include "./include/resp_parser.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

void handleClient(int client_fd) {
  std::cout << "Client connected on thread " << std::this_thread::get_id()
            << std::endl;

  char buffer[1024];
  std::string input;

  while (true) {
    ssize_t bytes_received = read(client_fd, buffer, sizeof(buffer));
    if (bytes_received <= 0) {
      if (bytes_received < 0) {
        std::cerr << "Failed to read\n";
      }
      break;
    }

    input.append(buffer, bytes_received);

    while (!input.empty()) {
      RESPParser parser(input);
      auto root = parser.parser();
      if (!root) {
        break;
      }

      auto arr = std::dynamic_pointer_cast<Arrays>(root);
      if (!arr) {
        std::cerr << "Expected RESP Array\n";
        break;
      }

      std::vector<std::string> parts;
      for (auto &val : arr->values) {
        auto bulk = std::dynamic_pointer_cast<BulkStrings>(val);
        if (bulk) {
          parts.push_back(bulk->value);
        }
      }

      handleCommand(parts, client_fd);

      input.erase(0, parser.bytesConsumed());
    }
  }

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

    std::thread client_thread(handleClient, client_fd);
    client_thread.detach();
  }

  close(server_fd);
  return EXIT_SUCCESS;
}
