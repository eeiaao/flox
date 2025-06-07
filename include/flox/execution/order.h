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
#include <optional>

namespace flox
{

enum class OrderStatus
{
  NEW,
  PENDING,
  PARTIALLY_FILLED,
  FILLED,
  CANCELED,
  EXPIRED,
  REPLACED,
  REJECTED
};

struct Order
{
  OrderId id{};
  Side side{};
  Price price{};
  Quantity quantity{};
  OrderType type{};
  SymbolId symbol{};

  OrderStatus status = OrderStatus::NEW;
  Quantity filledQuantity{0};

  std::chrono::nanoseconds createdAt{};
  std::optional<std::chrono::nanoseconds> exchangeTimestamp;
  std::optional<std::chrono::nanoseconds> lastUpdated;
  std::optional<std::chrono::nanoseconds> expiresAfter;
};

}  // namespace flox