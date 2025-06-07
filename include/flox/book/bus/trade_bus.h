#pragma once

#include "flox/book/events/trade_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<EventHandle<TradeEvent>, true, true>;
#else
using TradeBus = EventBus<EventHandle<TradeEvent>, false, true>;
#endif

}  // namespace flox
