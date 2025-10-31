#pragma once

#include "./store.h"
#include <shared_mutex>
#include <unordered_map>

class KVStore {
public:
  KVStore();
  ~KVStore();

  void set(const std::string &key, const std::string &value,
           long long expiry_ms = -1);
  std::string get(const std::string &key);
  void cleanup_expired();
  bool remove(const std::string &key);
  size_t size() const;
  void clear();

private:
  std::unordered_map<std::string, StoreValue> store;
  mutable std::shared_mutex mutex_;
};
