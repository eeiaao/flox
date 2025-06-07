#pragma once

#include "flox/book/events/book_update_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<EventHandle<BookUpdateEvent>, true, true>;
#else
using BookUpdateBus = EventBus<EventHandle<BookUpdateEvent>, false, true>;
#endif

}  // namespace flox
