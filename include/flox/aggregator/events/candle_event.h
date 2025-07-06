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
