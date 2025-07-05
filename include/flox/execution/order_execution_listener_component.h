#pragma once

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
concept OrderExecutionListener =
    Subsystem<T> &&
    Subscriber<T> &&
    requires(T t, const Order& o, const Order& o2, const std::string& r, Quantity q) {
      { t.onOrderSubmitted(o) } -> std::same_as<void>;
      { t.onOrderAccepted(o) } -> std::same_as<void>;
      { t.onOrderPartiallyFilled(o, q) } -> std::same_as<void>;
      { t.onOrderFilled(o) } -> std::same_as<void>;
      { t.onOrderCanceled(o) } -> std::same_as<void>;
      { t.onOrderExpired(o) } -> std::same_as<void>;
      { t.onOrderRejected(o, r) } -> std::same_as<void>;
      { t.onOrderReplaced(o, o2) } -> std::same_as<void>;
    };

}  // namespace concepts

namespace traits
{

struct OrderExecutionListenerTrait
{
  struct VTable
  {
    const SubscriberTrait::VTable* subscriber;
    const SubsystemTrait::VTable* subsystem;

    void (*onOrderSubmitted)(void*, const Order&);
    void (*onOrderAccepted)(void*, const Order&);
    void (*onOrderPartiallyFilled)(void*, const Order&, Quantity);
    void (*onOrderFilled)(void*, const Order&);
    void (*onOrderCanceled)(void*, const Order&);
    void (*onOrderExpired)(void*, const Order&);
    void (*onOrderRejected)(void*, const Order&, const std::string&);
    void (*onOrderReplaced)(void*, const Order&, const Order&);

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
    requires concepts::OrderExecutionListener<T>
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

class OrderExecutionListenerRef : public RefBase<OrderExecutionListenerRef, traits::OrderExecutionListenerTrait>
{
  using VTable = traits::OrderExecutionListenerTrait::VTable;
  using Base = RefBase<OrderExecutionListenerRef, traits::OrderExecutionListenerTrait>;

 public:
  using Base::Base;

  SubscriberId id() const { return _vtable->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _vtable->subscriber->mode(_ptr); }

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

  void onOrderSubmitted(const Order& o) const { _vtable->onOrderSubmitted(_ptr, o); }
  void onOrderAccepted(const Order& o) const { _vtable->onOrderAccepted(_ptr, o); }
  void onOrderPartiallyFilled(const Order& o, Quantity q) const { _vtable->onOrderPartiallyFilled(_ptr, o, q); }
  void onOrderFilled(const Order& o) const { _vtable->onOrderFilled(_ptr, o); }
  void onOrderCanceled(const Order& o) const { _vtable->onOrderCanceled(_ptr, o); }
  void onOrderExpired(const Order& o) const { _vtable->onOrderExpired(_ptr, o); }
  void onOrderRejected(const Order& o, const std::string& r) const { _vtable->onOrderRejected(_ptr, o, r); }
  void onOrderReplaced(const Order& o1, const Order& o2) const { _vtable->onOrderReplaced(_ptr, o1, o2); }
};
static_assert(concepts::OrderExecutionListener<OrderExecutionListenerRef>);

template <>
struct RefFor<traits::OrderExecutionListenerTrait>
{
  using type = OrderExecutionListenerRef;
};

}  // namespace flox
