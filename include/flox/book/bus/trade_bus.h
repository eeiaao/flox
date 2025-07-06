/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/events/trade_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<TradeEvent, SyncPolicy<TradeEvent>>;
#else
using TradeBus = EventBus<TradeEvent, AsyncPolicy<TradeEvent>>;
#endif

using TradeBusRef = EventBusRef<TradeEvent, TradeBus::Queue>;

}  // namespace flox
