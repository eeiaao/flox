/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <memory>

#include "flox/execution/bus/order_execution_bus.h"
#include "flox/execution/events/order_event.h"
#include "flox/execution/order.h"

using namespace flox;

namespace
{

class SyncListener : public IOrderExecutionListener
{
 public:
  explicit SyncListener(std::atomic<int>& c) : _counter(c) {}

  void onOrderAccepted(const Order&) override {}
  void onOrderPartiallyFilled(const Order&, Quantity) override {}
  void onOrderFilled(const Order& order) override
  {
    ++_counter;
    last = order;
  }
  void onOrderCanceled(const Order&) override {}
  void onOrderExpired(const Order&) override {}
  void onOrderRejected(const Order&) override {}
  void onOrderReplaced(const Order&, const Order&) override {}

  Order last{};

 private:
  std::atomic<int>& _counter;
};

}  // namespace

TEST(SyncOrderExecutionBusTest, WaitsForAllSubscribers)
{
  OrderExecutionBus bus;
  std::atomic<int> counter{0};
  auto l1 = std::make_shared<SyncListener>(counter);
  auto l2 = std::make_shared<SyncListener>(counter);
  bus.subscribe(l1);
  bus.subscribe(l2);

  bus.start();

  OrderEvent event{};
  event.type = OrderEventType::FILLED;
  event.order.symbol = 42;
  event.order.side = Side::BUY;
  event.order.quantity = Quantity::fromDouble(1.0);

  bus.publish(event);
  EXPECT_EQ(counter.load(), 2);

  bus.stop();
}
