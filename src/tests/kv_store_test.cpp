#include "../include/kv_store.h"
#include <gtest/gtest.h>
#include <thread>

TEST(KVStoreTest, SetGetWithExpiry) {
  KVStore kv_store;
  std::string key = "mykey";
  std::string value = "myvalue";
  long long expiry_ms = 100; // 100 milliseconds

  kv_store.set(key, value, expiry_ms);

  // Get immediately, should not be expired
  EXPECT_EQ(kv_store.get(key), value);

  // Wait for expiry
  std::this_thread::sleep_for(std::chrono::milliseconds(expiry_ms + 50)); // Wait a bit longer than expiry_ms

  // Get after expiry, should be empty
  EXPECT_EQ(kv_store.get(key), "");
}

TEST(KVStoreTest, CleanupExpired) {
  KVStore kv_store;
  kv_store.set("key1", "value1", 10); // Expire soon
  kv_store.set("key2", "value2", 100000); // Not expire
  kv_store.set("key3", "value3", 20); // Expire soon

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  kv_store.cleanup_expired();
  EXPECT_EQ(kv_store.get("key1"), "");
  EXPECT_EQ(kv_store.get("key3"), "");
  EXPECT_EQ(kv_store.get("key2"), "value2");
  EXPECT_EQ(kv_store.size(), 1);
}
