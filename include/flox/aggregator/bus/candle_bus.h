/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

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

}  // namespace flox
