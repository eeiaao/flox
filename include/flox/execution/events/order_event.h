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
#include "flox/util/base/ref.h"

namespace flox
{

enum class OrderEventType
{
  ACCEPTED,
  PARTIALLY_FILLED,
  FILLED,
  CANCELED,
  EXPIRED,
  REJECTED,
  REPLACED
};

struct OrderEvent
{
  using Listener = OrderExecutionListenerRef;

  OrderEventType type{};
  Order order{};
  Order newOrder{};
  Quantity fillQty{0};
  std::string rejectionReason;

  uint64_t tickSequence = 0;

  template <typename ListenerT>
  void dispatchTo(ListenerT& listener) const
  {
    static_assert(concepts::OrderExecutionListener<ListenerT>);

    switch (type)
    {
      case OrderEventType::ACCEPTED:
        listener.onOrderAccepted(order);
        break;
      case OrderEventType::PARTIALLY_FILLED:
        listener.onOrderPartiallyFilled(order, fillQty);
        break;
      case OrderEventType::FILLED:
        listener.onOrderFilled(order);
        break;
      case OrderEventType::CANCELED:
        listener.onOrderCanceled(order);
        break;
      case OrderEventType::EXPIRED:
        listener.onOrderExpired(order);
        break;
      case OrderEventType::REJECTED:
        listener.onOrderRejected(order, rejectionReason);
        break;
      case OrderEventType::REPLACED:
        listener.onOrderReplaced(order, newOrder);
        break;
    }
  }
};

}  // namespace flox
