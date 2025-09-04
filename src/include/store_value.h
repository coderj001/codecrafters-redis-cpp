#include <chrono>
#include <string>

struct StoreValue {
  std::string value;
  std::chrono::steady_clock::time_point expiry_time;
  bool has_expiry = false;
};
