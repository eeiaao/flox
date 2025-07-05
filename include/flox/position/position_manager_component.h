#pragma once

#include <concepts>
#include <string>

#include "flox/common.h"
#include "flox/engine/subsystem_component.h"
#include "flox/execution/order_execution_listener_component.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept PositionManager =
    OrderExecutionListener<T> &&
    Subsystem<T> &&
    requires(T t, SymbolId symbol) {
      { t.getPosition(symbol) } -> std::same_as<Quantity>;
    };

}  // namespace concepts

namespace traits
{

struct PositionManagerTrait
{
  struct VTable
  {
    const OrderExecutionListenerTrait::VTable* oel;

    Quantity (*getPosition)(void*, SymbolId);

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, OrderExecutionListenerTrait>)
        return oel;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::PositionManager<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto oel = OrderExecutionListenerTrait::makeVTable<T>();
    return {
        .oel = &oel,
        .getPosition = meta::wrap<&T::getPosition>(),
    };
  }
};

}  // namespace traits

class PositionManagerRef : public RefBase<PositionManagerRef, traits::PositionManagerTrait>
{
  using VTable = traits::PositionManagerTrait::VTable;
  using Base = RefBase<PositionManagerRef, traits::PositionManagerTrait>;

 public:
  using Base::Base;

  PositionManagerRef(void* ptr, const VTable* vtable)
      : Base(ptr, vtable), _oel(vtable->oel) {}

  SubscriberId id() const { return _oel->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _oel->subscriber->mode(_ptr); }

  void start() const { _oel->subsystem->start(_ptr); }
  void stop() const { _oel->subsystem->stop(_ptr); }

  void onOrderSubmitted(const Order& o) const { _oel->onOrderSubmitted(_ptr, o); }
  void onOrderAccepted(const Order& o) const { _oel->onOrderAccepted(_ptr, o); }
  void onOrderPartiallyFilled(const Order& o, Quantity q) const { _oel->onOrderPartiallyFilled(_ptr, o, q); }
  void onOrderFilled(const Order& o) const { _oel->onOrderFilled(_ptr, o); }
  void onOrderCanceled(const Order& o) const { _oel->onOrderCanceled(_ptr, o); }
  void onOrderExpired(const Order& o) const { _oel->onOrderExpired(_ptr, o); }
  void onOrderRejected(const Order& o, const std::string& r) const { _oel->onOrderRejected(_ptr, o, r); }
  void onOrderReplaced(const Order& o, const Order& o2) const { _oel->onOrderReplaced(_ptr, o, o2); }

  Quantity getPosition(SymbolId symbol) const { return _vtable->getPosition(_ptr, symbol); }

 private:
  const traits::OrderExecutionListenerTrait::VTable* _oel = nullptr;
};
static_assert(concepts::PositionManager<PositionManagerRef>);

template <>
struct RefFor<traits::PositionManagerTrait>
{
  using type = PositionManagerRef;
};

}  // namespace flox
