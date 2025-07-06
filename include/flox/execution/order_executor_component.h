/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <concepts>

#include "flox/common.h"
#include "flox/execution/order.h"
#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept OrderExecutor = requires(T t, const Order& order, OrderId id) {
  { t.submitOrder(order) } -> std::same_as<void>;
  { t.cancelOrder(id) } -> std::same_as<void>;
  { t.replaceOrder(id, order) } -> std::same_as<void>;
};

}  // namespace concepts

namespace traits
{

struct OrderExecutorTrait
{
  struct VTable
  {
    void (*submitOrder)(void*, const Order&);
    void (*cancelOrder)(void*, OrderId);
    void (*replaceOrder)(void*, OrderId, const Order&);
  };

  template <typename T>
    requires concepts::OrderExecutor<T>
  static constexpr VTable makeVTable()
  {
    return {
        .submitOrder = meta::wrap<&T::submitOrder>(),
        .cancelOrder = meta::wrap<&T::cancelOrder>(),
        .replaceOrder = meta::wrap<&T::replaceOrder>(),
    };
  }
};

}  // namespace traits

class OrderExecutorRef : public RefBase<OrderExecutorRef, traits::OrderExecutorTrait>
{
  using VTable = traits::OrderExecutorTrait::VTable;
  using Base = RefBase<OrderExecutorRef, traits::OrderExecutorTrait>;

 public:
  using Base::Base;

  void submitOrder(const Order& order) const { _vtable->submitOrder(_ptr, order); }
  void cancelOrder(OrderId id) const { _vtable->cancelOrder(_ptr, id); }
  void replaceOrder(OrderId id, const Order& order) const { _vtable->replaceOrder(_ptr, id, order); }
};
static_assert(concepts::OrderExecutor<OrderExecutorRef>);

template <>
struct RefFor<traits::OrderExecutorTrait>
{
  using type = OrderExecutorRef;
};

}  // namespace flox
