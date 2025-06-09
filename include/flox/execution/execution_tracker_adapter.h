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
#include "flox/metrics/abstract_execution_tracker.h"

#include <chrono>

namespace flox
{

class ExecutionTrackerAdapter : public IOrderExecutionListener
{
 public:
  ExecutionTrackerAdapter(SubscriberId id, IExecutionTracker* tracker)
      : IOrderExecutionListener(id), _tracker(tracker)
  {
  }

  void onOrderAccepted(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderAccepted(order, std::chrono::steady_clock::now());
    }
  }

  void onOrderPartiallyFilled(const Order& order, Quantity qty) override
  {
    if (_tracker)
    {
      _tracker->onOrderPartiallyFilled(order, qty,
                                       std::chrono::steady_clock::now());
    }
  }

  void onOrderFilled(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderFilled(order, std::chrono::steady_clock::now());
    }
  }

  void onOrderCanceled(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderCanceled(order, std::chrono::steady_clock::now());
    }
  }

  void onOrderExpired(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderExpired(order, std::chrono::steady_clock::now());
    }
  }

  void onOrderRejected(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderRejected(order, "", std::chrono::steady_clock::now());
    }
  }

  void onOrderReplaced(const Order& oldOrder, const Order& newOrder) override
  {
    if (_tracker)
    {
      _tracker->onOrderReplaced(oldOrder, newOrder,
                                std::chrono::steady_clock::now());
    }
  }

 private:
  IExecutionTracker* _tracker = nullptr;
};

}  // namespace flox
