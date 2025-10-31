#pragma once

#include <chrono>
#include <string>

/**
 * StoreValue: Represents a value stored in Redis with optional expiry.
 */
struct StoreValue {
  std::string value;
  std::chrono::steady_clock::time_point expiry_time;
  bool has_expiry;

  StoreValue(std::string val) : value(std::move(val)), has_expiry(false) {}

  void set_expiry(long long expiry_ms) {
    has_expiry = true;
    expiry_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(expiry_ms);
  }

  bool is_expired() const {
    return has_expiry && std::chrono::steady_clock::now() > expiry_time;
  }
};
