/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/common.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/market_data_event.h"
#include "flox/engine/symbol_registry.h"
#include "flox/util/ref_countable.h"

#include <chrono>
#include <memory_resource>
#include <string>
#include <vector>

namespace flox
{

enum class BookUpdateType
{
  SNAPSHOT,
  DELTA
};

struct BookLevel
{
  Price price;
  Quantity quantity;

  BookLevel() = default;
  BookLevel(Price p, Quantity q) : price(p), quantity(q) {}
};

struct BookUpdateEvent : public IMarketDataEvent
{
  SymbolId symbol;
  BookUpdateType type;
  std::pmr::vector<BookLevel> bids;
  std::pmr::vector<BookLevel> asks;
  std::chrono::system_clock::time_point timestamp;

  BookUpdateEvent(std::pmr::memory_resource* res) : bids(res), asks(res)
  {
    assert(res != nullptr && "pmr::memory_resource is null!");
  }

  MarketDataEventType eventType() const noexcept override;
  void dispatchTo(IMarketDataSubscriber& sub) const override;
};

}  // namespace flox
