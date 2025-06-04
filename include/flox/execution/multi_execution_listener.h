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
#include <vector>

namespace flox
{

class MultiExecutionListener : public IOrderExecutionListener
{
 public:
  void addListener(IOrderExecutionListener* listener)
  {
    if (listener && std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end())
    {
      _listeners.push_back(listener);
    }
  }

  void onOrderFilled(const Order& order) override
  {
    for (auto* listener : _listeners)
    {
      listener->onOrderFilled(order);
    }
  }

  void onOrderRejected(const Order& order, const std::string& reason) override
  {
    for (auto* listener : _listeners)
    {
      listener->onOrderRejected(order, reason);
    }
  }

 private:
  std::vector<IOrderExecutionListener*> _listeners;
};

}  // namespace flox
