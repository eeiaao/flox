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
#include "flox/execution/order_execution_listener_component.h"

#include <vector>

namespace flox
{

class MultiExecutionListener
{
 public:
  MultiExecutionListener(SubscriberId id) : _id(id) {}

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void addListener(OrderExecutionListenerRef listener)
  {
    for (const auto& l : _listeners)
    {
      if (l.id() == listener.id())
        return;
    }

    _listeners.push_back(std::move(listener));
  }

  void onOrderSubmitted(const Order& o)
  {
    for (auto& l : _listeners)
    {
      l.onOrderSubmitted(o);
    }
  }

  void onOrderAccepted(const Order& o)
  {
    for (auto& l : _listeners)
    {
      l.onOrderAccepted(o);
    }
  }

  void onOrderPartiallyFilled(const Order& o, Quantity q)
  {
    for (auto& l : _listeners)
    {
      l.onOrderPartiallyFilled(o, q);
    }
  }

  void onOrderFilled(const Order& o)
  {
    for (auto& l : _listeners)
    {
      l.onOrderFilled(o);
    }
  }

  void onOrderCanceled(const Order& o)
  {
    for (auto& l : _listeners)
    {
      l.onOrderCanceled(o);
    }
  }

  void onOrderExpired(const Order& o)
  {
    for (auto& l : _listeners)
    {
      l.onOrderExpired(o);
    }
  }

  void onOrderRejected(const Order& o, const std::string& r)
  {
    for (auto& l : _listeners)
    {
      l.onOrderRejected(o, r);
    }
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder)
  {
    for (auto& l : _listeners)
    {
      l.onOrderReplaced(oldOrder, newOrder);
    }
  }

 private:
  SubscriberId _id;
  std::vector<OrderExecutionListenerRef> _listeners;
};

}  // namespace flox

static_assert(flox::concepts::OrderExecutionListener<flox::MultiExecutionListener>);
