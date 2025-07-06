/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include "flox/common.h"
#include "flox/execution/multi_execution_listener.h"
#include "flox/execution/order_execution_listener_component.h"
#include "flox/util/base/ref.h"

using namespace flox;

class MockExecutionListener
{
  SubscriberId _id{};

 public:
  using Trait = traits::OrderExecutionListenerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  int acceptedCount = 0;
  int partialCount = 0;
  int filledCount = 0;
  int canceledCount = 0;
  int expiredCount = 0;
  int rejectedCount = 0;
  int replacedCount = 0;
  Order lastOrder;
  Order replacedOld;
  Order replacedNew;

  MockExecutionListener(SubscriberId id) : _id(id) {}

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onOrderSubmitted(const Order& order) {}

  void onOrderAccepted(const Order& order)
  {
    ++acceptedCount;
    lastOrder = order;
  }

  void onOrderPartiallyFilled(const Order& order, Quantity)
  {
    ++partialCount;
    lastOrder = order;
  }

  void onOrderFilled(const Order& order)
  {
    ++filledCount;
    lastOrder = order;
  }

  void onOrderCanceled(const Order& order)
  {
    ++canceledCount;
    lastOrder = order;
  }

  void onOrderExpired(const Order& order)
  {
    ++expiredCount;
    lastOrder = order;
  }

  void onOrderRejected(const Order& order, const std::string&)
  {
    ++rejectedCount;
    lastOrder = order;
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder)
  {
    ++replacedCount;
    replacedOld = oldOrder;
    replacedNew = newOrder;
  }
};
static_assert(concepts::OrderExecutionListener<MockExecutionListener>);

TEST(MultiExecutionListenerTest, CallsAllListeners)
{
  MultiExecutionListener multi(1);

  auto h1 = make<MockExecutionListener>(10);
  auto h2 = make<MockExecutionListener>(20);

  multi.addListener(h1);
  multi.addListener(h2);

  Order order{};
  order.symbol = 1;
  order.price = Price::fromDouble(100.0);
  order.quantity = Quantity::fromDouble(1.0);

  multi.onOrderFilled(order);

  EXPECT_EQ(h1.get<MockExecutionListener>().filledCount, 1);
  EXPECT_EQ(h2.get<MockExecutionListener>().filledCount, 1);
  EXPECT_EQ(h1.get<MockExecutionListener>().lastOrder.price, Price::fromDouble(100.0));
  EXPECT_EQ(h2.get<MockExecutionListener>().lastOrder.quantity, Quantity::fromDouble(1.0));
}

TEST(MultiExecutionListenerTest, ForwardsLifecycleCallbacks)
{
  MultiExecutionListener multi(1);

  auto handle = make<MockExecutionListener>(2);
  multi.addListener(handle);

  Order order{};
  order.symbol = 1;
  order.price = Price::fromDouble(10.0);
  order.quantity = Quantity::fromDouble(1.0);

  auto& ref = handle.get<MockExecutionListener>();

  multi.onOrderAccepted(order);
  EXPECT_EQ(ref.acceptedCount, 1);

  multi.onOrderPartiallyFilled(order, Quantity::fromDouble(0.5));
  EXPECT_EQ(ref.partialCount, 1);

  multi.onOrderCanceled(order);
  EXPECT_EQ(ref.canceledCount, 1);

  multi.onOrderExpired(order);
  EXPECT_EQ(ref.expiredCount, 1);

  multi.onOrderRejected(order, "test");
  EXPECT_EQ(ref.rejectedCount, 1);

  Order newOrder{};
  newOrder.symbol = 2;
  newOrder.price = Price::fromDouble(11.0);
  newOrder.quantity = Quantity::fromDouble(1.0);

  multi.onOrderReplaced(order, newOrder);
  EXPECT_EQ(ref.replacedCount, 1);
  EXPECT_EQ(ref.replacedOld.symbol, 1u);
  EXPECT_EQ(ref.replacedNew.symbol, 2u);
}

TEST(MultiExecutionListenerTest, PreventsDuplicateListeners)
{
  MultiExecutionListener multi(100);

  auto handle1 = make<MockExecutionListener>(101);
  auto& ref = handle1.get<MockExecutionListener>();

  multi.addListener(handle1);
  auto duplicate = make<MockExecutionListener>(101);
  multi.addListener(duplicate);

  Order order{};
  order.symbol = 1;
  order.price = Price::fromDouble(100.0);
  order.quantity = Quantity::fromDouble(1.0);

  multi.onOrderFilled(order);

  EXPECT_EQ(ref.filledCount, 1);
}
