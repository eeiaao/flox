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
#include <ranges>

namespace flox
{

CandleAggregator::CandleAggregator(std::chrono::seconds interval, CandleBus* bus)
    : _interval(interval), _bus(bus)
{
}

void CandleAggregator::start()
{
  _candles.clear();
}
void CandleAggregator::stop()
{
  for (auto&& [symbol, pc] :
       _candles | std::views::filter([](auto&& kv)
                                     { return kv.second.initialized; }))
  {
    pc.candle.endTime = pc.candle.startTime + _interval;
    if (_bus)
    {
      CandleEvent ev{symbol, pc.candle};
      _bus->publish(ev);
    }
  }
  _candles.clear();
}

void CandleAggregator::onTrade(const TradeEvent& event)
{
  auto ts = alignToInterval(event.trade.timestamp);
  auto& partial = _candles[event.trade.symbol];

  if (!partial.initialized || partial.candle.startTime != ts)
  {
    if (partial.initialized)
    {
      partial.candle.endTime = partial.candle.startTime + _interval;
      if (_bus)
      {
        CandleEvent ev{event.trade.symbol, partial.candle};
        _bus->publish(ev);
      }
    }
    partial.candle =
        Candle(ts, event.trade.price,
               Volume::fromDouble(event.trade.price.toDouble() * event.trade.quantity.toDouble()));
    partial.candle.endTime = ts + _interval;
    partial.initialized = true;
    return;
  }

  auto& c = partial.candle;
  c.high = std::max(c.high, event.trade.price);
  c.low = std::min(c.low, event.trade.price);
  c.close = event.trade.price;
  c.volume += Volume::fromDouble(event.trade.price.toDouble() * event.trade.quantity.toDouble());
  c.endTime = partial.candle.startTime + _interval;
}

std::chrono::system_clock::time_point CandleAggregator::alignToInterval(
    std::chrono::system_clock::time_point tp)
{
  auto epoch = tp.time_since_epoch();
  auto secs = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  auto snapped = (secs.count() / _interval.count()) * _interval.count();
  return std::chrono::system_clock::time_point(std::chrono::seconds(snapped));
}

}  // namespace flox