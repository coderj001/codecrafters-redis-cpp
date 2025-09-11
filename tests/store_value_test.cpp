#include <gtest/gtest.h>
#include "../../src/include/store_value.h"
#include <chrono>
#include <thread>

TEST(StoreValueTest, NotExpiredWhenNoExpirySet) {
    StoreValue sv;
    sv.value = "test";
    ASSERT_FALSE(sv.is_expired());
}

TEST(StoreValueTest, NotExpiredWhenExpiryIsInTheFuture) {
    StoreValue sv;
    sv.value = "test";
    sv.has_expiry = true;
    sv.expiry_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    ASSERT_FALSE(sv.is_expired());
}

TEST(StoreValueTest, ExpiredWhenExpiryIsInThePast) {
    StoreValue sv;
    sv.value = "test";
    sv.has_expiry = true;
    sv.expiry_time = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Ensure clock ticks
    ASSERT_TRUE(sv.is_expired());
}
