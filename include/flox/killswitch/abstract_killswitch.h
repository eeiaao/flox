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

namespace flox
{

class IKillSwitch
{
 public:
  virtual ~IKillSwitch() = default;

  virtual void check(const Order& order) = 0;
  virtual void trigger(const std::string& reason) = 0;
  virtual bool isTriggered() const = 0;
  virtual std::string reason() const = 0;
};

}  // namespace flox
