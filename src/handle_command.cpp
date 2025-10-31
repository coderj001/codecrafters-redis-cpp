#include "include/handle_command.h"

#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <sys/socket.h>

std::unordered_map<std::string, StoreValue> store;
std::mutex store_mutex;

void handleCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.empty())
    return;
  std::string cmd_upper = parts[0];
  std::transform(cmd_upper.begin(), cmd_upper.end(), cmd_upper.begin(),
                 ::toupper);

  if (cmd_upper == "PING") {
    dprintf(client_fd, "+PONG\r\n");
  } else if (cmd_upper == "ECHO") {
    if (parts.size() > 1) {
      std::string echo_msg = parts[1];
      dprintf(client_fd, "$%zu\r\n%s\r\n", echo_msg.size(), echo_msg.c_str());
    }
  } else if (cmd_upper == "SET") {
    handleSetCommand(parts, client_fd);
  } else if (cmd_upper == "GET") {
    handleGetCommand(parts, client_fd);
  } else {
    dprintf(client_fd, "-ERR unknown command '%s'\r\n", parts[0].c_str());
  }
}


void handleSetCommand(const std::vector<std::string> &parts, int fd) {
  if (parts.size() < 3) {
    dprintf(fd, "-ERR wrong number of arguments for 'set' command\r\n");
    return;
  }

  const std::string &key = parts[1];
  const std::string &value = parts[2];
  long long px_expiry_ms = -1;
  bool nx_enabled = false;

  for (size_t i = 3; i < parts.size(); ++i) {
    std::string option = parts[i];
    std::transform(option.begin(), option.end(), option.begin(), ::toupper);

    if (option == "PX" && i + 1 < parts.size()) {
      try {
        px_expiry_ms = std::stoll(parts[++i]);
      } catch (const std::invalid_argument &) {
        dprintf(fd, "-ERR value is not an integer or out of range\r\n");
        return;
      }
    } else if (option == "NX") {
      nx_enabled = true;
    }
  }
  StoreValue store_value(value);
  if (px_expiry_ms > 0) {
    store_value.set_expiry(px_expiry_ms);
  }

  if (nx_enabled) {
    // Only set if the key does not already exist
    std::lock_guard<std::mutex> lock(store_mutex);
    if (store.count(key) > 0) {
      dprintf(fd, "$-1\r\n"); // Return Null Bulk String for NX when key exists
      return;
    }
    store.insert_or_assign(key, std::move(store_value));
  } else {
    std::lock_guard<std::mutex> lock(store_mutex);
    store.insert_or_assign(key, std::move(store_value));
  }

  dprintf(fd, "+OK\r\n");
}

void handleGetCommand(const std::vector<std::string> &parts, int fd) {
  if (parts.size() < 2) {
    dprintf(fd, "-ERR wrong number of arguments for 'get' command\r\n");
    return;
  }

  const std::string &key = parts[1];

  std::lock_guard<std::mutex> lock(store_mutex);
  auto it = store.find(key);

  if (it == store.end()) {
    dprintf(fd, "$-1\r\n"); // Null Bulk String for non-existent key
  } else {
    if (it->second.is_expired()) {
      store.erase(it);
      dprintf(fd, "$-1\r\n"); // Null Bulk String for expired key
    } else {
      dprintf(fd, "$%zu\r\n%s\r\n", it->second.value.size(),
              it->second.value.c_str());
    }
  }
}
