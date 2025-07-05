#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_CANDLE_BUS
using CandleBus = EventBus<CandleEvent, SyncPolicy<CandleEvent>>;
#else
using CandleBus = EventBus<CandleEvent, AsyncPolicy<CandleEvent>>;
#endif

using CandleBusRef = EventBusRef<CandleEvent, CandleBus::Queue>;

}  // namespace flox
