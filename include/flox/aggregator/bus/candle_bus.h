#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

using CandleBus = EventBus<CandleEvent, false, true>;

}  // namespace flox
