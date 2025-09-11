#include "store_value.h"

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

extern std::unordered_map<std::string, StoreValue> store;
extern std::mutex store_mutex;

void handleCommand(const std::vector<std::string> &parts, int client_fd);
void handle_set_command(const std::vector<std::string> &parts, int client_fd);
void handle_get_command(const std::vector<std::string> &parts, int client_fd);
void handle_echo_command(const std::vector<std::string> &parts, int client_fd);
void handle_ping_command(int client_fd);
