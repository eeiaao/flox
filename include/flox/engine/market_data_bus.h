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

#include "flox/common.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/market_data_event.h"

namespace flox
{

class MarketDataBus
{
 public:
  static constexpr size_t QueueSize = 4096;
#ifdef USE_SYNC_MARKET_BUS
  using QueueItem = std::pair<EventHandle<IMarketDataEvent>, TickBarrier*>;
#else
  using QueueItem = EventHandle<IMarketDataEvent>;
#endif

  using Queue = SPSCQueue<QueueItem, QueueSize>;

  MarketDataBus();
  ~MarketDataBus();

  void subscribe(std::shared_ptr<IMarketDataSubscriber> subscriber);

  Queue* getQueue(SubscriberId id);

  void start();
  void stop();

  template <typename T>
  void publish(EventHandle<T> event)
    requires std::is_base_of_v<IMarketDataEvent, T>
  {
    publish(event.template upcast<IMarketDataEvent>());
  }

 private:
  void publish(EventHandle<IMarketDataEvent> event);

 private:
  class Impl;
  std::unique_ptr<Impl> _impl;
};

}  // namespace flox
