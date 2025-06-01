/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/aggregator/candle_aggregator.h"

#include <cassert>

namespace flox {

CandleAggregator::CandleAggregator(std::chrono::seconds interval,
                                   CandleCallback callback)
    : _interval(interval), _callback(std::move(callback)) {}

void CandleAggregator::start() { _candles.clear(); }
void CandleAggregator::stop() {
  for (auto &[symbol, pc] : _candles) {
    if (pc.initialized) {
      pc.candle.endTime = pc.candle.startTime + _interval;
      _callback(symbol, pc.candle);
    }
  }
  _candles.clear();
}

void CandleAggregator::onMarketData(const IMarketDataEvent &event) {
  if (event.eventType() == MarketDataEventType::TRADE) {
    onTrade(static_cast<TradeEvent *>(const_cast<IMarketDataEvent *>(&event)));
  }
}

void CandleAggregator::onTrade(TradeEvent *trade) {
  auto ts = alignToInterval(trade->timestamp);
  auto &partial = _candles[trade->symbol];

  if (!partial.initialized || partial.candle.startTime != ts) {
    if (partial.initialized) {
      partial.candle.endTime = partial.candle.startTime + _interval;
      _callback(trade->symbol, partial.candle);
    }
    partial.candle = Candle(ts, trade->price, trade->quantity);
    partial.candle.endTime = ts + _interval;
    partial.initialized = true;
    return;
  }

  auto &c = partial.candle;
  c.high = std::max(c.high, trade->price);
  c.low = std::min(c.low, trade->price);
  c.close = trade->price;
  c.volume += trade->quantity;
  c.endTime = partial.candle.startTime + _interval;
}

std::chrono::system_clock::time_point
CandleAggregator::alignToInterval(std::chrono::system_clock::time_point tp) {
  auto epoch = tp.time_since_epoch();
  auto secs = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  auto snapped = (secs.count() / _interval.count()) * _interval.count();
  return std::chrono::system_clock::time_point(std::chrono::seconds(snapped));
}

} // namespace flox