/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/util/decimal.h"

#include <cstdint>

namespace flox
{

enum class OrderType
{
  LIMIT,
  MARKET
};
enum class Side
{
  BUY,
  SELL
};

using SymbolId = uint32_t;

struct PriceTag
{
};
struct QuantityTag
{
};
struct VolumeTag
{
};

// tick = 0.000001 for everything
using Price = Decimal<PriceTag, 1'000'000, 1>;
using Quantity = Decimal<QuantityTag, 1'000'000, 1>;
using Volume = Decimal<VolumeTag, 1'000'000, 1>;

}  // namespace flox