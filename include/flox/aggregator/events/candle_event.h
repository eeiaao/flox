/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/candle.h"
#include "flox/common.h"
#include "flox/engine/market_data_subscriber_component.h"

namespace flox
{

struct CandleEvent
{
  using Listener = MarketDataSubscriberRef;

  SymbolId symbol{};
  Candle candle{};

  uint64_t tickSequence = 0;
};

}  // namespace flox
