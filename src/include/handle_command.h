#include <string>
#include <unordered_map>
#include <vector>

extern std::unordered_map<std::string, std::string> store;

void handleCommand(const std::vector<std::string> &parts, int client_fd);
