#include "flox/execution/order.h"

#include <gtest/gtest.h>
#include <chrono>

using namespace flox;

TEST(OrderLifecycleTest, Defaults)
{
  Order order{};
  EXPECT_EQ(order.status, OrderStatus::NEW);
  EXPECT_TRUE(order.filledQuantity.isZero());
  EXPECT_EQ(order.createdAt, std::chrono::steady_clock::time_point{});
  EXPECT_FALSE(order.exchangeTimestamp.has_value());
  EXPECT_FALSE(order.lastUpdated.has_value());
  EXPECT_FALSE(order.expiresAfter.has_value());
}
