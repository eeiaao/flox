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

#include <string>

namespace flox
{

class IOrderExecutionListener
{
 public:
  virtual ~IOrderExecutionListener() = default;
  virtual void onOrderFilled(const Order& order) = 0;
  virtual void onOrderRejected(const Order& order, const std::string& reason) = 0;
};

}  // namespace flox