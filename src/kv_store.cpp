#include "include/kv_store.h"
#include "include/store.h"
#include <mutex>
#include <shared_mutex>
#include <utility>

KVStore::KVStore() = default;
KVStore::~KVStore() = default;

void KVStore::set(const std::string &key, const std::string &value,
                  long long expiry_ms) {
  std::unique_lock<std::shared_mutex> lock(mutex_); // exclusive write

  StoreValue store_value{value};

  if (expiry_ms > 0) {
    store_value.set_expiry(expiry_ms);
  }

  store.insert_or_assign(key, std::move(store_value));
}

std::string KVStore::get(const std::string &key) {
  { // Scoped shared lock for reading
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = store.find(key);
    if (it == store.end()) {
      return ""; // not found
    }

    const StoreValue &store_value = it->second;
    if (!store_value.is_expired()) {
      return store_value.value;
    }
  } // Shared lock is released here

  // If we reach here, the item was found and is expired.
  // Now acquire an exclusive lock to remove it.
  std::unique_lock<std::shared_mutex> lock(mutex_);
  auto it = store.find(key); // Re-find with exclusive lock
  if (it != store.end() && it->second.is_expired()) {
    store.erase(it);
  }
  return ""; // Expired returns empty
}

void KVStore::cleanup_expired() {
  std::unique_lock<std::shared_mutex> lock(
      mutex_); // Exclusive lock for writing

  auto it = store.begin();
  while (it != store.end()) {
    if (it->second.is_expired()) {
      it = store.erase(it);
    } else {
      ++it;
    }
  }
}

bool KVStore::remove(const std::string &key) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  auto it = store.find(key);
  if (it != store.end()) {
    store.erase(it);
    return true;
  }
  return false;
}

size_t KVStore::size() const {
  std::shared_lock<std::shared_mutex> lock(mutex_);
  return store.size();
}

void KVStore::clear() {
  std::unique_lock<std::shared_mutex> lock(mutex_);
  store.clear();
}
