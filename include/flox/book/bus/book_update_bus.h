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
 * @brief Create a BookUpdateBus with optimal HFT configuration
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return Unique pointer to configured BookUpdateBus
 */
inline std::unique_ptr<BookUpdateBus>
createOptimalBookUpdateBus(bool enablePerformanceOptimizations = false)
{
  auto bus = std::make_unique<BookUpdateBus>();
  bool success = bus->setupOptimalConfiguration(BookUpdateBus::ComponentType::MARKET_DATA,
                                                enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[BookUpdateBus] Configured with isolated cores for market data processing" << std::endl;
  }
  else
  {
    std::cout << "[BookUpdateBus] Warning: Could not configure isolated cores, using default scheduling" << std::endl;
  }

  return bus;
}

/**
 * @brief Configure an existing BookUpdateBus for optimal HFT performance
 * @param bus BookUpdateBus instance to configure
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return true if configuration was successful
 */
inline bool configureBookUpdateBusForHFT(BookUpdateBus& bus, bool enablePerformanceOptimizations = false)
{
  bool success = bus.setupOptimalConfiguration(BookUpdateBus::ComponentType::MARKET_DATA,
                                               enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[BookUpdateBus] HFT configuration applied successfully" << std::endl;
    bus.printConfiguration();
  }
  else
  {
    std::cout << "[BookUpdateBus] Warning: HFT configuration failed" << std::endl;
  }

  return success;
}

}  // namespace flox
