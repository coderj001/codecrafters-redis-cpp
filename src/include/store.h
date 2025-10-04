#pragma once

#include <chrono>
#include <string>

union Data {
  std::string s_data;
};

/**
 * StoreValue: Represents a value stored in Redis with optional expiry.
 */
struct StoreValue {
  std::string value;
  // Data data;
  std::chrono::steady_clock::time_point expiry_time{};
  bool has_expiry = false;

  /**
   * Checks if the value has expired.
   * @return true if expired, false otherwise.
   */
  bool is_expired() const {
    if (!has_expiry) {
      return false;
    }
    return std::chrono::steady_clock::now() > expiry_time;
  }
};
