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

#include "flox/execution/bus/order_execution_bus.h"
#include "flox/execution/events/order_event.h"
#include "flox/execution/order.h"
#include "flox/execution/order_execution_listener_component.h"
#include "flox/util/base/ref.h"

using namespace flox;

namespace
{

class CountingListener
{
 public:
  using Trait = traits::OrderExecutionListenerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit CountingListener(SubscriberId id, std::atomic<int>& c)
      : _id(id), _counter(c) {}

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onOrderSubmitted(const Order&) {}
  void onOrderAccepted(const Order&) {}
  void onOrderPartiallyFilled(const Order&, Quantity) {}
  void onOrderFilled(const Order& order)
  {
    ++_counter;
    last = order;
  }
  void onOrderCanceled(const Order&) {}
  void onOrderExpired(const Order&) {}
  void onOrderRejected(const Order&, const std::string&) {}
  void onOrderReplaced(const Order&, const Order&) {}

  Order last{};

 private:
  SubscriberId _id;
  std::atomic<int>& _counter;
};
static_assert(concepts::OrderExecutionListener<CountingListener>);

}  // namespace

TEST(OrderExecutionBusTest, SubscribersReceiveFill)
{
  OrderExecutionBus bus;
  bus.enableDrainOnStop();
  std::atomic<int> c1{0}, c2{0};
  auto l1 = make<CountingListener>(1, c1);
  auto l2 = make<CountingListener>(2, c2);
  bus.subscribe(l1);
  bus.subscribe(l2);

  bus.start();

  OrderEvent event{};
  event.type = OrderEventType::FILLED;
  event.order.symbol = 1;
  event.order.side = Side::BUY;
  event.order.quantity = Quantity::fromDouble(1.0);

  bus.publish(event);
  bus.stop();

  EXPECT_EQ(c1.load(), 1);
  EXPECT_EQ(c2.load(), 1);
}
