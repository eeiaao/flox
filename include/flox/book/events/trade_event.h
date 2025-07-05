#pragma once

#include "flox/book/trade.h"
#include "flox/engine/market_data_subscriber_component.h"

namespace flox
{

struct TradeEvent
{
  using Listener = MarketDataSubscriberRef;

  Trade trade{};
  uint64_t tickSequence = 0;

  void clear()
  {
    trade = {};
  }
};

}  // namespace flox
