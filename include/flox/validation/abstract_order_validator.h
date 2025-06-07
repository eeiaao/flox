/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/execution/order.h"

#include <string>

namespace flox
{

class IOrderValidator
{
 public:
  virtual ~IOrderValidator() = default;

  virtual bool validate(const Order& order, std::string& reason) const = 0;
};

}  // namespace flox