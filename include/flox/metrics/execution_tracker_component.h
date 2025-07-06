/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <chrono>
#include <concepts>
#include <string>

#include "flox/common.h"
#include "flox/engine/subscriber_component.h"
#include "flox/engine/subsystem_component.h"
#include "flox/execution/order.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept ExecutionTracker =
    Subscriber<T> &&
    Subsystem<T> &&
    requires(T t, const Order& o, const Order& o2, Quantity q,
             std::chrono::steady_clock::time_point ts, const std::string& reason) {
      { t.onOrderSubmitted(o, ts) } -> std::same_as<void>;
      { t.onOrderAccepted(o, ts) } -> std::same_as<void>;
      { t.onOrderPartiallyFilled(o, q, ts) } -> std::same_as<void>;
      { t.onOrderFilled(o, ts) } -> std::same_as<void>;
      { t.onOrderCanceled(o, ts) } -> std::same_as<void>;
      { t.onOrderExpired(o, ts) } -> std::same_as<void>;
      { t.onOrderRejected(o, reason, ts) } -> std::same_as<void>;
      { t.onOrderReplaced(o, o2, ts) } -> std::same_as<void>;
    };

}  // namespace concepts

namespace traits
{

struct ExecutionTrackerTrait
{
  struct VTable
  {
    const traits::SubscriberTrait::VTable* subscriber;
    const traits::SubsystemTrait::VTable* subsystem;

    void (*onOrderSubmitted)(void*, const Order&, std::chrono::steady_clock::time_point);
    void (*onOrderAccepted)(void*, const Order&, std::chrono::steady_clock::time_point);
    void (*onOrderPartiallyFilled)(void*, const Order&, Quantity, std::chrono::steady_clock::time_point);
    void (*onOrderFilled)(void*, const Order&, std::chrono::steady_clock::time_point);
    void (*onOrderCanceled)(void*, const Order&, std::chrono::steady_clock::time_point);
    void (*onOrderExpired)(void*, const Order&, std::chrono::steady_clock::time_point);
    void (*onOrderRejected)(void*, const Order&, const std::string&, std::chrono::steady_clock::time_point);
    void (*onOrderReplaced)(void*, const Order&, const Order&, std::chrono::steady_clock::time_point);

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, SubsystemTrait>)
        return subsystem;
      else if constexpr (std::is_same_v<Trait, SubscriberTrait>)
        return subscriber;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::ExecutionTracker<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto sub = SubscriberTrait::makeVTable<T>();
    static constexpr auto sys = SubsystemTrait::makeVTable<T>();
    return {
        .subscriber = &sub,
        .subsystem = &sys,
        .onOrderSubmitted = meta::wrap<&T::onOrderSubmitted>(),
        .onOrderAccepted = meta::wrap<&T::onOrderAccepted>(),
        .onOrderPartiallyFilled = meta::wrap<&T::onOrderPartiallyFilled>(),
        .onOrderFilled = meta::wrap<&T::onOrderFilled>(),
        .onOrderCanceled = meta::wrap<&T::onOrderCanceled>(),
        .onOrderExpired = meta::wrap<&T::onOrderExpired>(),
        .onOrderRejected = meta::wrap<&T::onOrderRejected>(),
        .onOrderReplaced = meta::wrap<&T::onOrderReplaced>(),
    };
  }
};

}  // namespace traits

class ExecutionTrackerRef : public RefBase<ExecutionTrackerRef, traits::ExecutionTrackerTrait>
{
  using VTable = traits::ExecutionTrackerTrait::VTable;
  using Base = RefBase<ExecutionTrackerRef, traits::ExecutionTrackerTrait>;

 public:
  using Base::Base;

  SubscriberId id() const { return _vtable->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _vtable->subscriber->mode(_ptr); }

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

  void onOrderSubmitted(const Order& o, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderSubmitted(_ptr, o, ts);
  }
  void onOrderAccepted(const Order& o, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderAccepted(_ptr, o, ts);
  }
  void onOrderPartiallyFilled(const Order& o, Quantity q, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderPartiallyFilled(_ptr, o, q, ts);
  }
  void onOrderFilled(const Order& o, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderFilled(_ptr, o, ts);
  }
  void onOrderCanceled(const Order& o, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderCanceled(_ptr, o, ts);
  }
  void onOrderExpired(const Order& o, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderExpired(_ptr, o, ts);
  }
  void onOrderRejected(const Order& o, const std::string& reason, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderRejected(_ptr, o, reason, ts);
  }
  void onOrderReplaced(const Order& o1, const Order& o2, std::chrono::steady_clock::time_point ts) const
  {
    _vtable->onOrderReplaced(_ptr, o1, o2, ts);
  }
};
static_assert(concepts::ExecutionTracker<ExecutionTrackerRef>);

template <>
struct RefFor<traits::ExecutionTrackerTrait>
{
  using type = ExecutionTrackerRef;
};

}  // namespace flox
