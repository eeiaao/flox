/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/book_update.h"
#include "flox/engine/events/market_data_event.h"

#include <memory_resource>

namespace flox
{

struct BookUpdateEvent : public IMarketDataEvent
{
  using Listener = IMarketDataSubscriber;

  BookUpdate update;

  BookUpdateEvent(std::pmr::memory_resource* res) : update(res)
  {
    assert(res != nullptr && "pmr::memory_resource is null!");
  }

  void clear() override
  {
    update.bids.clear();
    update.asks.clear();
  }
};

}  // namespace flox
