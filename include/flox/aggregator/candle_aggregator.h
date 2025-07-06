/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/aggregator/bus/candle_bus.h"
#include "flox/aggregator/candle_aggregator_component.h"
#include "flox/aggregator/events/candle_event.h"
#include "flox/book/candle.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/market_data_subscriber_component.h"

#include <chrono>

namespace flox
{

class CandleAggregator
{
 public:
  using Trait = traits::CandleAggregatorTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  CandleAggregator(std::chrono::seconds interval, CandleBusRef bus);

  CandleAggregator(CandleAggregator&& other) noexcept
      : _interval(other._interval), _bus(other._bus), _candles(std::move(other._candles))
  {
    assert(_interval.count() > 0 && "CandleAggregator interval expected to be > 0");
  }

  CandleAggregator() = delete;
  CandleAggregator(const CandleAggregator&) = delete;
  CandleAggregator& operator=(const CandleAggregator&) = delete;
  CandleAggregator& operator=(CandleAggregator&&) = delete;

  ~CandleAggregator();

  void start();
  void stop();

  SubscriberId id() const;
  SubscriberMode mode() const;

  void onTrade(const TradeEvent& event);
  void onBookUpdate(const BookUpdateEvent&) {}
  void onCandle(const CandleEvent&) {}

 private:
  struct PartialCandle
  {
    Candle candle{};
    bool initialized = false;
  };

  std::chrono::seconds _interval{1};
  CandleBusRef _bus;
  std::vector<std::optional<PartialCandle>> _candles;

  std::chrono::steady_clock::time_point alignToInterval(std::chrono::steady_clock::time_point tp);
};

}  // namespace flox

static_assert(flox::concepts::CandleAggregator<flox::CandleAggregator>);
