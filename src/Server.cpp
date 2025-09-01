#include "./include/resp_parser.h"
#include <algorithm>
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

std::unordered_map<std::string, std::string> store;

void handleCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.empty())
    return;

  std::string cmd = parts[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

  if (cmd == "SET") {
    if (parts.size() < 3) {
      std::string resp = "-ERR wrong number of arguments for 'set' command\r\n";
      send(client_fd, resp.c_str(), resp.size(), 0);
    } else {
      const std::string &key = parts[1];
      const std::string &value = parts[2];
      store[key] = value;
      std::string resp = encodeSimpleString("OK");
      send(client_fd, resp.c_str(), resp.size(), 0);
    }
  } else if (cmd == "GET") {
    if (parts.size() < 2) {
      std::string resp = "-ERR wrong number of arguments for 'get' command\r\n";
      send(client_fd, resp.c_str(), resp.size(), 0);
    } else {
      const std::string &key = parts[1];
      auto it = store.find(key);
      if (it != store.end()) {
        std::string resp = encodeBulkString(it->second);
        send(client_fd, resp.c_str(), resp.size(), 0);
      } else {
        std::string resp = encodeNullBulkString();
        send(client_fd, resp.c_str(), resp.size(), 0);
      }
    }
  } else {
    std::string resp = "-ERR unknown command '" + parts[0] + "'\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
  }
}

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
  return 0;
}
