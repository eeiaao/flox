#pragma once

#include "flox/book/events/trade_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<TradeEvent, true, true>;
#else
using TradeBus = EventBus<TradeEvent, false, true>;
#endif

}  // namespace flox
