#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using CandleBus = EventBus<CandleEvent, true, true>;
#else
using CandleBus = EventBus<CandleEvent, false, true>;
#endif

}  // namespace flox
