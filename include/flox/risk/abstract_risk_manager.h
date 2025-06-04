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
#include "flox/engine/subsystem.h"

namespace flox
{

class IRiskManager : public ISubsystem
{
 public:
  virtual ~IRiskManager() = default;
  virtual bool allow(const Order& order) const = 0;
};

}  // namespace flox
