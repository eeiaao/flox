/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/events/book_update_event.h"
#include "flox/util/eventing/event_bus.h"
#include "flox/util/memory/pool.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, SyncPolicy<pool::Handle<BookUpdateEvent>>>;
#else
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, AsyncPolicy<pool::Handle<BookUpdateEvent>>>;
#endif

}  // namespace flox
