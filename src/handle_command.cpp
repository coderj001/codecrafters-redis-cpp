#include "./include/handle_command.h"
#include "include/resp_parser.h"

#include <algorithm>
#include <sys/socket.h>

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
  } else if (cmd == "ECHO") {
    if (parts.size() < 2) {
      std::string resp =
          "-ERR wrong number of arguments for 'echo' command\r\n";
      send(client_fd, resp.c_str(), resp.size(), 0);
    } else {
      std::string resp = encodeBulkString(parts[1]);
      send(client_fd, resp.c_str(), resp.size(), 0);
    }
  } else if (cmd == "PING") {
    std::string resp = encodeBulkString("PONG");
    send(client_fd, resp.c_str(), resp.size(), 0);
  } else {
    std::string resp = "-ERR unknown command '" + parts[0] + "'\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
  }
}
