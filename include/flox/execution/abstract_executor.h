/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/subsystem.h"
#include "flox/execution/abstract_execution_listener.h"
#include "flox/execution/order.h"
#include "flox/metrics/abstract_execution_tracker.h"

namespace flox
{

class IOrderExecutor : public ISubsystem
{
 public:
  virtual ~IOrderExecutor() = default;

  virtual void submitOrder(const Order& order) = 0;
  virtual void cancelOrder(OrderId orderId) = 0;
  virtual void replaceOrder(OrderId oldOrderId, const Order& newOrder) = 0;

  void setExecutionTracker(IExecutionTracker* tracker) { _tracker = tracker; }
  void setListener(IOrderExecutionListener* listener) { _listener = listener; }

 protected:
  IExecutionTracker* getExecutionTracker() const { return _tracker; }
  IOrderExecutionListener* getListener() const { return _listener; }

 private:
  IExecutionTracker* _tracker = nullptr;
  IOrderExecutionListener* _listener = nullptr;
};

}  // namespace flox
