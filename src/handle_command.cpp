#include "include/handle_command.h"
#include "include/resp_parser.h"

#include <algorithm>
#include <mutex>
#include <sys/socket.h>

std::unordered_map<std::string, StoreValue> store;
std::mutex store_mutex;

void handleCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.empty())
    return;

  std::string cmd = parts[0];
  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

  if (cmd == "SET") {
    handleSetCommand(parts, client_fd);
  } else if (cmd == "GET") {
    handleGetCommand(parts, client_fd);
  } else if (cmd == "ECHO") {
    handleEchoCommand(parts, client_fd);
  } else if (cmd == "PING") {
    handlePingCommand(client_fd);
  } else {
    std::string resp = "-ERR unknown command '" + parts[0] + "'\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
  }
}

void handleSetCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.size() < 3) {
    std::string resp = "-ERR wrong number of arguments for 'set' command\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
    return;
  }

  const std::string &key = parts[1];
  const std::string &value = parts[2];

  StoreValue store_value;
  store_value.value = value;

  if (parts.size() >= 5) {
    std::string px_arg = parts[3];
    std::transform(px_arg.begin(), px_arg.end(), px_arg.begin(), ::toupper);
    if (px_arg == "PX") {
      try {
        long long ms = std::stoll(parts[4]);
        store_value.has_expiry = true;
        store_value.expiry_time =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
      } catch (const std::exception &e) {
        std::cerr << "Expiry time value out of range: " << e.what()
                  << std::endl;
        std::string resp = "-ERR invalid expire time\r\n";
        send(client_fd, resp.c_str(), resp.size(), 0);
        return;
      }
    }
  }

  // Lock the store before writing
  {
    std::lock_guard<std::mutex> lock(store_mutex);
    store[key] = store_value;
  }

  std::string resp = encodeSimpleString("OK");
  send(client_fd, resp.c_str(), resp.size(), 0);
}

void handleGetCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.size() < 2) {
    std::string resp = "-ERR wrong number of arguments for 'get' command\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
    return;
  }

  const std::string &key = parts[1];
  std::string resp;

  // Lock the store before reading
  std::lock_guard<std::mutex> lock(store_mutex);

  auto it = store.find(key);
  if (it != store.end()) {
    // Key exists, now check for expiry
    StoreValue &store_value = it->second;

    if (store_value.is_expired()) {
      store.erase(it);
      resp = encodeNullBulkString();
    } else {
      resp = encodeBulkString(store_value.value);
    }
  } else {
    resp = encodeNullBulkString();
  }

  send(client_fd, resp.c_str(), resp.size(), 0);
}

void handleEchoCommand(const std::vector<std::string> &parts, int client_fd) {
  if (parts.size() < 2) {
    std::string resp = "-ERR wrong number of arguments for 'echo' command\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
    return;
  }

  std::string resp = encodeBulkString(parts[1]);
  send(client_fd, resp.c_str(), resp.size(), 0);
}

void handlePingCommand(int client_fd) {
  std::string resp = encodeSimpleString("PONG");
  send(client_fd, resp.c_str(), resp.size(), 0);
}
