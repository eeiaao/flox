/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/abstract_subscriber.h"
#include "flox/engine/events/market_data_event.h"

namespace flox
{

class BookUpdateEvent;
class TradeEvent;
class CandleEvent;

class IMarketDataSubscriber : public ISubscriber
{
 public:
  virtual ~IMarketDataSubscriber() = default;

  virtual void onBookUpdate(const BookUpdateEvent& ev) {}
  virtual void onTrade(const TradeEvent& ev) {}
  virtual void onCandle(const CandleEvent& ev) {}
};

}  // namespace flox