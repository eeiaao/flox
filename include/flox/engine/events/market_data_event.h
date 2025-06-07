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
#include "flox/util/base/ref_countable.h"

namespace flox
{

struct TickBarrier;
class IMarketDataSubscriber;

struct IMarketDataEvent : public RefCountable
{
 protected:
  IEventPool* _origin = nullptr;

 public:
  virtual ~IMarketDataEvent() = default;

  void setPool(IEventPool* pool)
  {
    _origin = pool;
  }

  virtual void releaseToPool()
  {
    if (_origin)
    {
      _origin->release(this);
    }
  }

  virtual void clear() {}
};

}  // namespace flox