/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/trade.h"
#include "flox/engine/events/market_data_event.h"

namespace flox
{

struct TradeEvent
{
  using Listener = IMarketDataSubscriber;

  Trade trade{};
};

}  // namespace flox
