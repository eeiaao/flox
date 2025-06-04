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

#include <chrono>
#include <cstdint>
#include <string>

namespace flox
{

struct Order
{
  uint64_t id;
  Side side;
  double price;
  double quantity;
  OrderType type;
  SymbolId symbol;
  std::chrono::system_clock::time_point timestamp;
};

}  // namespace flox