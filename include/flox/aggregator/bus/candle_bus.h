/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <memory>
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

/**
 * @brief Create and configure a CandleBus with optimal isolated core settings
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return Configured CandleBus instance
 */
inline std::unique_ptr<CandleBus> createOptimalCandleBus(bool enablePerformanceOptimizations = false)
{
  auto bus = std::make_unique<CandleBus>();
  bool success = bus->setupOptimalConfiguration(CandleBus::ComponentType::STRATEGY,
                                                enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[CandleBus] Configured with isolated cores for candle processing" << std::endl;
  }
  else
  {
    std::cout << "[CandleBus] Warning: Could not configure isolated cores, using default scheduling" << std::endl;
  }

  return bus;
}

/**
 * @brief Configure an existing CandleBus for optimal HFT performance
 * @param bus CandleBus instance to configure
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return true if configuration was successful
 */
inline bool configureCandleBusForHFT(CandleBus& bus, bool enablePerformanceOptimizations = false)
{
  bool success = bus.setupOptimalConfiguration(CandleBus::ComponentType::STRATEGY,
                                               enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[CandleBus] HFT configuration applied successfully" << std::endl;
    bus.printConfiguration();
  }
  else
  {
    std::cout << "[CandleBus] Warning: HFT configuration failed" << std::endl;
  }

  return success;
}

}  // namespace flox
