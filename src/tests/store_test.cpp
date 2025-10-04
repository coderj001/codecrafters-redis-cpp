#include "../include/store.h"
#include <chrono>
#include <gtest/gtest.h>

TEST(StoreValueTest, NoExpiry) {
  StoreValue sv;
  sv.value = "test";
  sv.has_expiry = false;
  EXPECT_FALSE(sv.is_expired());
}

TEST(StoreValueTest, Expired) {
  StoreValue sv;
  sv.value = "test";
  sv.has_expiry = true;
  sv.expiry_time = std::chrono::steady_clock::now() - std::chrono::seconds(1);
  EXPECT_TRUE(sv.is_expired());
}

TEST(StoreValueTest, NotExpired) {
  StoreValue sv;
  sv.value = "test";
  sv.has_expiry = true;
  sv.expiry_time = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  EXPECT_FALSE(sv.is_expired());
}
