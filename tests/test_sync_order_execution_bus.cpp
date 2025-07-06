/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <functional>

#include "flox/execution/bus/order_execution_bus.h"
#include "flox/execution/events/order_event.h"
#include "flox/execution/order.h"
#include "flox/execution/order_execution_listener_component.h"
#include "flox/util/base/ref.h"

#ifndef USE_SYNC_ORDER_BUS
#error "Test requires USE_SYNC_ORDER_BUS to be defined"
#endif

using namespace flox;

namespace
{

class SyncListener
{
 public:
  using Trait = traits::OrderExecutionListenerTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit SyncListener(SubscriberId id, std::atomic<int>& c)
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
    ++_counter.get();
    last = order;
  }
  void onOrderCanceled(const Order&) {}
  void onOrderExpired(const Order&) {}
  void onOrderRejected(const Order&, const std::string&) {}
  void onOrderReplaced(const Order&, const Order&) {}

  Order last{};

 private:
  SubscriberId _id;
  std::reference_wrapper<std::atomic<int>> _counter;
};
static_assert(concepts::OrderExecutionListener<SyncListener>);

}  // namespace

TEST(SyncOrderExecutionBusTest, WaitsForAllSubscribers)
{
  OrderExecutionBus bus;
  std::atomic<int> counter{0};

  bus.subscribe(make<SyncListener>(1, counter));
  bus.subscribe(make<SyncListener>(2, counter));

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
