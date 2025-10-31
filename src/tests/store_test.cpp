#include "../include/store.h"
#include <gtest/gtest.h>

TEST(StoreValueTest, NoExpiry) {
  StoreValue sv("test");
  EXPECT_FALSE(sv.is_expired());
}

TEST(StoreValueTest, Expired) {
  StoreValue sv("test");
  sv.set_expiry(-1000); // Set expiry to 1 second ago
  EXPECT_TRUE(sv.is_expired());
}

TEST(StoreValueTest, NotExpired) {
  StoreValue sv("test");
  sv.set_expiry(1000); // Set expiry to 1 second from now
  EXPECT_FALSE(sv.is_expired());
}
