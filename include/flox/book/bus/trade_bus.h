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
#include "flox/book/events/trade_event.h"
#include "flox/util/eventing/event_bus.h"

namespace flox
{

#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<TradeEvent, SyncPolicy<TradeEvent>>;
#else
using TradeBus = EventBus<TradeEvent, AsyncPolicy<TradeEvent>>;
#endif

using TradeBusRef = EventBusRef<TradeEvent, TradeBus::Queue>;

/**
 * @brief Create and configure a TradeBus with optimal isolated core settings
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return Configured TradeBus instance
 */
inline std::unique_ptr<TradeBus> createOptimalTradeBus(bool enablePerformanceOptimizations = false)
{
  auto bus = std::make_unique<TradeBus>();
  bool success = bus->setupOptimalConfiguration(TradeBus::ComponentType::MARKET_DATA,
                                                enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[TradeBus] Configured with isolated cores for market data processing" << std::endl;
  }
  else
  {
    std::cout << "[TradeBus] Warning: Could not configure isolated cores, using default scheduling" << std::endl;
  }

  return bus;
}

/**
 * @brief Configure an existing TradeBus for optimal HFT performance
 * @param bus TradeBus instance to configure
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return true if configuration was successful
 */
inline bool configureTradeBusForHFT(TradeBus& bus, bool enablePerformanceOptimizations = false)
{
  bool success = bus.setupOptimalConfiguration(TradeBus::ComponentType::MARKET_DATA,
                                               enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[TradeBus] HFT configuration applied successfully" << std::endl;
    bus.printConfiguration();
  }
  else
  {
    std::cout << "[TradeBus] Warning: HFT configuration failed" << std::endl;
  }

  return success;
}

}  // namespace flox
