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
  std::string resp;

  if (cmd == "SET") {
    if (parts.size() < 3) {
      resp =
          encodeErrorString("ERR wrong number of arguments for 'set' command");
    } else {
      const std::string &key = parts[1];
      const std::string &value = parts[2];

      StoreValue store_value;
      store_value.value = value;

      if (parts.size() > 3) {
        std::string px_arg = parts[3];
        std::transform(px_arg.begin(), px_arg.end(), px_arg.begin(), ::toupper);
        if (px_arg == "PX") {
          try {
            long long ms = std::stoll(parts[4]);
            //  PX expiry test failed (key still exists)
            store_value.has_expiry = true; // fix time issue
            store_value.expiry_time = std::chrono::steady_clock::now() +
                                      std::chrono::milliseconds(ms);
          } catch (const std::exception &e) {
            std::cerr << "Expiry time value out of range: " << e.what()
                      << std::endl;
            store_value.has_expiry = false;
            return;
          }
        }
      }

      // Lock the store before writing
      {
        std::lock_guard<std::mutex> lock(store_mutex);
        store[key] = store_value;
      }

      resp = encodeSimpleString("OK");
    }
    send(client_fd, resp.c_str(), resp.size(), 0);
  } else if (cmd == "GET") {
    if (parts.size() < 2) {
      resp =
          encodeErrorString("ERR wrong number of arguments for 'get' command");
      send(client_fd, resp.c_str(), resp.size(), 0);
    } else {
      const std::string &key = parts[1];

      // Lock the store before writing
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
    }
    send(client_fd, resp.c_str(), resp.size(), 0);
  } else if (cmd == "ECHO") {
    if (parts.size() < 2) {
      resp =
          encodeErrorString("ERR wrong number of arguments for 'echo' command");
    } else {
      resp = encodeBulkString(parts[1]);
    }
    send(client_fd, resp.c_str(), resp.size(), 0);
  } else if (cmd == "PING") {
    // Redis returns PONG as a simple string for PING.
    resp = encodeSimpleString("PONG");
    send(client_fd, resp.c_str(), resp.size(), 0);
  } else {
    resp = "-ERR unknown command '" + parts[0] + "'\r\n";
    send(client_fd, resp.c_str(), resp.size(), 0);
  }
}
