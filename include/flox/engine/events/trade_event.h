/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <chrono>
#include <string>

#include "flox/common.h"
#include "flox/engine/events/market_data_event.h"

namespace flox {

struct TradeEvent : public IMarketDataEvent {
  SymbolId symbol;
  Price price;
  Quantity quantity;
  bool isBuy;
  std::chrono::system_clock::time_point timestamp;

  TradeEvent(std::pmr::memory_resource *) {}

  MarketDataEventType eventType() const noexcept override;
  void dispatchTo(IMarketDataSubscriber &sub) const override;
};

} // namespace flox
