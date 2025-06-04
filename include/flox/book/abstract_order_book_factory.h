/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/abstract_order_book.h"

namespace flox
{

struct IOrderBookConfig
{
  virtual ~IOrderBookConfig() = default;
};

class IOrderBookFactory
{
 public:
  virtual ~IOrderBookFactory() = default;
  virtual IOrderBook* create(const IOrderBookConfig& config) = 0;
};

}  // namespace flox