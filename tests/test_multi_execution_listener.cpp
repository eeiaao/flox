/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include "flox/execution/multi_execution_listener.h"

using namespace flox;

class MockExecutionListener : public IOrderExecutionListener
{
 public:
  int filledCount = 0;
  int rejectedCount = 0;
  Order lastOrder;
  std::string lastReason;

  void onOrderFilled(const Order& order) override
  {
    ++filledCount;
    lastOrder = order;
  }

  void onOrderRejected(const Order& order, const std::string& reason) override
  {
    ++rejectedCount;
    lastOrder = order;
    lastReason = reason;
  }
};

TEST(MultiExecutionListenerTest, CallsAllListeners)
{
  MultiExecutionListener multi;
  MockExecutionListener l1, l2;

  multi.addListener(&l1);
  multi.addListener(&l2);

  Order order;
  order.symbol = 1;
  order.price = 100.0;
  order.quantity = 1.0;
  multi.onOrderFilled(order);

  EXPECT_EQ(l1.filledCount, 1);
  EXPECT_EQ(l2.filledCount, 1);
  EXPECT_EQ(l1.lastOrder.price, 100);
  EXPECT_EQ(l2.lastOrder.quantity, 1);
}

TEST(MultiExecutionListenerTest, PreventsDuplicateListeners)
{
  MultiExecutionListener multi;
  MockExecutionListener l1;

  multi.addListener(&l1);
  multi.addListener(&l1);  // Duplicate

  Order order;
  order.symbol = 1;
  order.price = 100.0;
  order.quantity = 1.0;
  multi.onOrderFilled(order);

  EXPECT_EQ(l1.filledCount, 1);  // Should only be called once
}
