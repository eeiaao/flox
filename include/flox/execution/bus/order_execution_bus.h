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

/**
 * @brief Create and configure an OrderExecutionBus with optimal isolated core settings
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return Configured OrderExecutionBus instance
 */
inline std::unique_ptr<OrderExecutionBus> createOptimalOrderExecutionBus(bool enablePerformanceOptimizations = false)
{
  auto bus = std::make_unique<OrderExecutionBus>();
  bool success = bus->setupOptimalConfiguration(OrderExecutionBus::ComponentType::EXECUTION,
                                                enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[OrderExecutionBus] Configured with isolated cores for order execution" << std::endl;
  }
  else
  {
    std::cout << "[OrderExecutionBus] Warning: Could not configure isolated cores, using default scheduling" << std::endl;
  }

  return bus;
}

/**
 * @brief Configure an existing OrderExecutionBus for optimal HFT performance
 * @param bus OrderExecutionBus instance to configure
 * @param enablePerformanceOptimizations Enable CPU frequency scaling optimizations
 * @return true if configuration was successful
 */
inline bool configureOrderExecutionBusForHFT(OrderExecutionBus& bus, bool enablePerformanceOptimizations = false)
{
  bool success = bus.setupOptimalConfiguration(OrderExecutionBus::ComponentType::EXECUTION,
                                               enablePerformanceOptimizations);

  if (success)
  {
    std::cout << "[OrderExecutionBus] HFT configuration applied successfully" << std::endl;
    bus.printConfiguration();
  }
  else
  {
    std::cout << "[OrderExecutionBus] Warning: HFT configuration failed" << std::endl;
  }

  return success;
}

}  // namespace flox
