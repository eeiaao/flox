#pragma once

#include "flox/execution/events/order_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_ORDER_BUS
using OrderExecutionBus = EventBus<OrderEvent, true>;
#else
using OrderExecutionBus = EventBus<OrderEvent, false>;
#endif

}  // namespace flox
