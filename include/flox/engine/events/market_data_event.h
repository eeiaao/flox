/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/abstract_event_pool.h"
#include "flox/engine/market_data_event_pool.h"
#include "flox/util/ref_countable.h"

namespace flox
{

struct TickBarrier;
class IMarketDataSubscriber;

enum class MarketDataEventType
{
  BOOK,
  TRADE,
  CANDLE
};

struct IMarketDataEvent : public RefCountable
{
 protected:
  IEventPool* _origin = nullptr;

 public:
  virtual ~IMarketDataEvent() = default;
  virtual MarketDataEventType eventType() const noexcept = 0;

  virtual void dispatchTo(IMarketDataSubscriber& sub) const = 0;

  void setPool(IEventPool* pool) { _origin = pool; }

  virtual void releaseToPool()
  {
    if (_origin)
    {
      _origin->release(this);
    }
  }

  virtual EventHandle<IMarketDataEvent> wrap() { return EventHandle<IMarketDataEvent>{this}; }

  virtual void clear() {}
};

}  // namespace flox