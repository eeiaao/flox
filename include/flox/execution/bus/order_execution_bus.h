#pragma once

#include "flox/execution/events/order_event.h"
#include "flox/util/eventing/event_bus.h"
#include "flox/util/eventing/event_bus_component.h"

namespace flox
{

#ifdef USE_SYNC_ORDER_BUS
using OrderExecutionBus = EventBus<OrderEvent, SyncPolicy<OrderEvent> >;
#else
using OrderExecutionBus = EventBus<OrderEvent, AsyncPolicy<OrderEvent> >;
#endif

using OrderExecutionBusRef = EventBusRef<OrderEvent, OrderExecutionBus::Queue>;

}  // namespace flox
