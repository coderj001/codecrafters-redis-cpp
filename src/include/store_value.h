#include <chrono>
#include <string>

struct StoreValue {
  std::string value;
  std::chrono::steady_clock::time_point expiry_time;
  bool has_expiry = false;

  bool is_expired() const {
    if (!has_expiry) {
      return false;
    }
    return std::chrono::steady_clock::now() > expiry_time;
  }
};
