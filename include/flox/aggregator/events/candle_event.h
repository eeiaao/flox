#pragma once

#include "flox/book/candle.h"
#include "flox/common.h"
#include "flox/engine/abstract_market_data_subscriber.h"

namespace flox
{

struct CandleEvent
{
  using Listener = IMarketDataSubscriber;

  SymbolId symbol{};
  Candle candle{};
};

}  // namespace flox
