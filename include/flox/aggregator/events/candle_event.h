#pragma once

#include "flox/book/candle.h"
#include "flox/common.h"

namespace flox
{
class IStrategy;  // forward declaration

struct CandleEvent
{
  using Listener = IStrategy;

  SymbolId symbol{};
  Candle candle{};
};

}  // namespace flox
