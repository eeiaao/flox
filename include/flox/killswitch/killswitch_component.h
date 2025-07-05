#pragma once

#include <concepts>
#include <string>

#include "flox/execution/order.h"
#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept KillSwitch = requires(T t, const Order& order, const std::string& reason) {
  { t.check(order) } -> std::same_as<void>;
  { t.trigger(reason) } -> std::same_as<void>;
  { t.isTriggered() } -> std::same_as<bool>;
  { t.reason() } -> std::same_as<std::string>;
};

}  // namespace concepts

namespace traits
{

struct KillSwitchTrait
{
  struct VTable
  {
    void (*check)(void*, const Order&);
    void (*trigger)(void*, const std::string&);
    bool (*isTriggered)(void*);
    std::string (*reason)(void*);
  };

  template <typename T>
    requires concepts::KillSwitch<T>
  static constexpr VTable makeVTable()
  {
    return {
        .check = meta::wrap<&T::check>(),
        .trigger = meta::wrap<&T::trigger>(),
        .isTriggered = meta::wrap<&T::isTriggered>(),
        .reason = meta::wrap<&T::reason>(),
    };
  }
};

}  // namespace traits

class KillSwitchRef : public RefBase<KillSwitchRef, traits::KillSwitchTrait>
{
  using VTable = traits::KillSwitchTrait::VTable;
  using Base = RefBase<KillSwitchRef, traits::KillSwitchTrait>;

 public:
  using Base::Base;

  void check(const Order& order) const { _vtable->check(_ptr, order); }
  void trigger(const std::string& r) const { _vtable->trigger(_ptr, r); }
  bool isTriggered() const { return _vtable->isTriggered(_ptr); }
  std::string reason() const { return _vtable->reason(_ptr); }
};
static_assert(concepts::KillSwitch<KillSwitchRef>);

template <>
struct RefFor<traits::KillSwitchTrait>
{
  using type = KillSwitchRef;
};

}  // namespace flox
