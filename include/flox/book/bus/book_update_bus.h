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

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, SyncPolicy<pool::Handle<BookUpdateEvent>>>;
#else
using BookUpdateBus = EventBus<pool::Handle<BookUpdateEvent>, AsyncPolicy<pool::Handle<BookUpdateEvent>>>;
#endif

using BookUpdateBusRef = EventBusRef<pool::Handle<BookUpdateEvent>, BookUpdateBus::Queue>;

/**
 * @brief Create a BookUpdateBus with optimal performance configuration
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return Unique pointer to configured BookUpdateBus
 */
inline std::unique_ptr<BookUpdateBus>
createOptimalBookUpdateBus(bool enablePerformanceOptimizations = false)
{
  auto bus = std::make_unique<BookUpdateBus>();
#if FLOX_CPU_AFFINITY_ENABLED
  bool success = bus->setupOptimalConfiguration(BookUpdateBus::ComponentType::MARKET_DATA,
                                                enablePerformanceOptimizations);
#endif
  return bus;
}

/**
 * @brief Configure an existing BookUpdateBus for optimal performance
 * @param bus BookUpdateBus instance to configure
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return true if configuration was successful
 */
inline bool configureBookUpdateBusForPerformance(BookUpdateBus& bus, bool enablePerformanceOptimizations = false)
{
#if FLOX_CPU_AFFINITY_ENABLED
  return bus.setupOptimalConfiguration(BookUpdateBus::ComponentType::MARKET_DATA,
                                       enablePerformanceOptimizations);
#else
  return true;
#endif
}

}  // namespace flox
