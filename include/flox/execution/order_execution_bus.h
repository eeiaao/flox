/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <memory>

#include "flox/book/order.h"
#include "flox/execution/abstract_execution_listener.h"
#include "flox/execution/order_event.h"
#include "flox/util/spsc_queue.h"

#ifdef USE_SYNC_ORDER_BUS
#include "flox/engine/tick_barrier.h"
#endif

namespace flox
{

class OrderExecutionBus
{
 public:
  static constexpr size_t QueueSize = 4096;
#ifdef USE_SYNC_ORDER_BUS
  using QueueItem = std::pair<OrderEvent, TickBarrier*>;
#else
  using QueueItem = OrderEvent;
#endif

  using Queue = SPSCQueue<QueueItem, QueueSize>;

  OrderExecutionBus();
  ~OrderExecutionBus();

  void subscribe(std::shared_ptr<IOrderExecutionListener> listener);
  void publish(const OrderEvent& event);
  void start();
  void stop();

 private:
  class Impl;
  std::unique_ptr<Impl> _impl;
};

}  // namespace flox
