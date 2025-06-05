/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/execution/abstract_execution_listener.h"

#include <algorithm>
#include <ranges>
#include <vector>

namespace flox
{

class MultiExecutionListener : public IOrderExecutionListener
{
 public:
  void addListener(IOrderExecutionListener* listener)
  {
    if (listener && std::ranges::find(_listeners, listener) == _listeners.end())
    {
      _listeners.push_back(listener);
    }
  }

  void onOrderFilled(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* listener)
                          { listener->onOrderFilled(order); });
  }

  void onOrderRejected(const Order& order, const std::string& reason) override
  {
    std::ranges::for_each(
        _listeners,
        [&](auto* listener)
        { listener->onOrderRejected(order, reason); });
  }

 private:
  std::vector<IOrderExecutionListener*> _listeners;
};

}  // namespace flox
