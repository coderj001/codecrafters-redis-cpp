#include "include/handle_command.h"
#include "include/resp_parser.h"

#include <algorithm>
#include <mutex>
#include <sys/socket.h>

std::unordered_map<std::string, StoreValue> store;
std::mutex store_mutex;

void handleCommand(const std::vector<std::string> &command_parts, int client_fd) {
  if (command_parts.empty())
    return;

  std::string command_name = command_parts[0];
  std::transform(command_name.begin(), command_name.end(), command_name.begin(), ::toupper);

  if (command_name == "SET") {
    handleSetCommand(command_parts, client_fd);
  } else if (command_name == "GET") {
    handleGetCommand(command_parts, client_fd);
  } else if (command_name == "RPUSH") {
    handlePpushCommand(command_parts, client_fd);
  } else if (command_name == "ECHO") {
    handleEchoCommand(command_parts, client_fd);
  } else if (command_name == "PING") {
    handlePingCommand(client_fd);
  } else {
    std::string response_string = "-ERR unknown command '" + command_parts[0] + "'\r\n";
    send(client_fd, response_string.c_str(), response_string.size(), 0);
  }
}

void handleSetCommand(const std::vector<std::string> &command_parts, int client_fd) {
  if (command_parts.size() < 3) {
    std::string response_string = "-ERR wrong number of arguments for 'set' command\r\n";
    send(client_fd, response_string.c_str(), response_string.size(), 0);
    return;
  }

  const std::string &key = command_parts[1];
  const std::string &value = command_parts[2];

  StoreValue store_value;
  store_value.value = value;

  if (command_parts.size() >= 5) {
    std::string expiry_option = command_parts[3];
    std::transform(expiry_option.begin(), expiry_option.end(), expiry_option.begin(), ::toupper);
    if (expiry_option == "PX") {
      try {
        long long expiry_milliseconds = std::stoll(command_parts[4]);
        store_value.has_expiry = true;
        store_value.expiry_time =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(expiry_milliseconds);
      } catch (const std::exception &e) {
        std::cerr << "Expiry time value out of range: " << e.what()
                  << std::endl;
        std::string response_string = "-ERR invalid expire time\r\n";
        send(client_fd, response_string.c_str(), response_string.size(), 0);
        return;
      }
    }
  }

  // Lock the store before writing
  {
    std::lock_guard<std::mutex> lock(store_mutex);
    store[key] = store_value;
  }

  std::string response_string = encodeSimpleString("OK");
  send(client_fd, response_string.c_str(), response_string.size(), 0);
}

void handleGetCommand(const std::vector<std::string> &command_parts, int client_fd) {
  if (command_parts.size() < 2) {
    std::string response_string = "-ERR wrong number of arguments for 'get' command\r\n";
    send(client_fd, response_string.c_str(), response_string.size(), 0);
    return;
  }

  const std::string &key = command_parts[1];
  std::string response_string;

  // Lock the store before reading
  std::lock_guard<std::mutex> lock(store_mutex);

  auto store_iterator = store.find(key);
  if (store_iterator != store.end()) {
    // Key exists, now check for expiry
    StoreValue &store_value = store_iterator->second;

    if (store_value.is_expired()) {
      store.erase(store_iterator);
      response_string = encodeNullBulkString();
    } else {
      response_string = encodeBulkString(store_value.value);
    }
  } else {
    response_string = encodeNullBulkString();
  }

  send(client_fd, response_string.c_str(), response_string.size(), 0);
}

void handleEchoCommand(const std::vector<std::string> &command_parts, int client_fd) {
  if (command_parts.size() < 2) {
    std::string response_string = "-ERR wrong number of arguments for 'echo' command\r\n";
    send(client_fd, response_string.c_str(), response_string.size(), 0);
    return;
  }

  std::string response_string = encodeBulkString(command_parts[1]);
  send(client_fd, response_string.c_str(), response_string.size(), 0);
}

void handlePingCommand(int client_fd) {
  std::string response_string = encodeSimpleString("PONG");
  send(client_fd, response_string.c_str(), response_string.size(), 0);
}


void handlePpushCommand(const std::vector<std::string> &command_parts, int client_fd){
}
