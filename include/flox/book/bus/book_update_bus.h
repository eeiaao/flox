/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/events/book_update_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, SyncPolicy<pool::Handle<BookUpdateEvent>>>;
#else
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, AsyncPolicy<pool::Handle<BookUpdateEvent>>>;
#endif

using BookUpdateBusRef = EventBusRef<pool::Handle<BookUpdateEvent>, BookUpdateBus::Queue>;

}  // namespace flox
