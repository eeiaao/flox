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
#include "flox/engine/events/market_data_event.h"

namespace flox {

using SubscriberId = uint64_t;
enum class SubscriberMode { PUSH, PULL };

class IMarketDataSubscriber {
public:
  virtual ~IMarketDataSubscriber() = default;
  virtual void onMarketData(const IMarketDataEvent &event) = 0;

  virtual SubscriberId id() const = 0;
  virtual SubscriberMode mode() const { return SubscriberMode::PUSH; }
};

} // namespace flox