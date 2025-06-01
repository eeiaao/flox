/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/events/book_update_event.h"

namespace flox {

class IOrderBook {
public:
  virtual ~IOrderBook() = default;

  virtual void applyBookUpdate(const BookUpdateEvent &update) = 0;
  virtual std::optional<double> bestBid() const = 0;
  virtual std::optional<double> bestAsk() const = 0;

  virtual double bidAtPrice(double price) const = 0;
  virtual double askAtPrice(double price) const = 0;
};

} // namespace flox