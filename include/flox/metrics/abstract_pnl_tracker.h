/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/order.h"

namespace flox
{

class IPnLTracker
{
 public:
  virtual ~IPnLTracker() = default;
  virtual void onOrderFilled(const Order& order) = 0;
};

}  // namespace flox
