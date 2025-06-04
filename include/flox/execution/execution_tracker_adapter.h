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
  explicit ExecutionTrackerAdapter(IExecutionTracker* tracker) : _tracker(tracker) {}

  void onOrderFilled(const Order& order) override
  {
    if (_tracker)
    {
      _tracker->onOrderFilled(order, std::chrono::steady_clock::now());
    }
  }

  void onOrderRejected(const Order& order, const std::string& reason) override
  {
    if (_tracker)
    {
      _tracker->onOrderRejected(order, reason, std::chrono::steady_clock::now());
    }
  }

 private:
  IExecutionTracker* _tracker = nullptr;
};

}  // namespace flox
