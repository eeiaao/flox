/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/candle.h"
#include "flox/book/events/trade_event.h"
#include "flox/common.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/market_data_event.h"
#include "flox/engine/subsystem.h"

#include <chrono>
#include <functional>
#include <unordered_map>

namespace flox
{

class CandleAggregator : public ISubsystem, public IMarketDataSubscriber
{
 public:
  using CandleCallback = std::move_only_function<void(SymbolId, const Candle&)>;

  CandleAggregator(std::chrono::seconds interval, CandleCallback callback);

  void start() override;
  void stop() override;

  SubscriberId id() const override { return reinterpret_cast<SubscriberId>(this); }
  SubscriberMode mode() const override { return SubscriberMode::PUSH; }

  void onTrade(const TradeEvent& trade) override;

 private:
  struct PartialCandle
  {
    Candle candle;
    bool initialized = false;
  };

  std::chrono::seconds _interval;
  CandleCallback _callback;
  std::unordered_map<SymbolId, PartialCandle> _candles;

  std::chrono::system_clock::time_point alignToInterval(std::chrono::system_clock::time_point tp);
};

}  // namespace flox
