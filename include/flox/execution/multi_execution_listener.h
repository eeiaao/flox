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
    if (listener && std::ranges::find(_listeners, listener) == _listeners.end())
    {
      _listeners.push_back(listener);
    }
  }

  void onOrderAccepted(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* l)
                          { l->onOrderAccepted(order); });
  }

  void onOrderPartiallyFilled(const Order& order, Quantity fillQty) override
  {
    std::ranges::for_each(
        _listeners,
        [&](auto* l)
        { l->onOrderPartiallyFilled(order, fillQty); });
  }

  void onOrderFilled(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* l)
                          { l->onOrderFilled(order); });
  }

  void onOrderCanceled(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* l)
                          { l->onOrderCanceled(order); });
  }

  void onOrderExpired(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* l)
                          { l->onOrderExpired(order); });
  }

  void onOrderRejected(const Order& order) override
  {
    std::ranges::for_each(_listeners,
                          [&](auto* l)
                          { l->onOrderRejected(order); });
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder) override
  {
    std::ranges::for_each(
        _listeners,
        [&](auto* l)
        { l->onOrderReplaced(oldOrder, newOrder); });
  }

 private:
  std::vector<IOrderExecutionListener*> _listeners;
};

}  // namespace flox
