/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <concepts>

#include "flox/engine/subsystem_component.h"
#include "flox/execution/order.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept RiskManager =
    Subsystem<T> &&
    requires(T t, const Order& order) {
      { t.allow(order) } -> std::same_as<bool>;
    };

}  // namespace concepts

namespace traits
{

struct RiskManagerTrait
{
  struct VTable
  {
    const SubsystemTrait::VTable* subsystem;
    bool (*allow)(void*, const Order&);

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, SubsystemTrait>)
        return subsystem;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::RiskManager<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto subsystem = SubsystemTrait::makeVTable<T>();
    return {
        .subsystem = &subsystem,
        .allow = meta::wrap<&T::allow>(),
    };
  }
};

}  // namespace traits

class RiskManagerRef : public RefBase<RiskManagerRef, traits::RiskManagerTrait>
{
  using VTable = traits::RiskManagerTrait::VTable;
  using Base = RefBase<RiskManagerRef, traits::RiskManagerTrait>;

 public:
  using Base::Base;

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

  bool allow(const Order& order) const { return _vtable->allow(_ptr, order); }
};
static_assert(concepts::RiskManager<RiskManagerRef>);

template <>
struct RefFor<traits::RiskManagerTrait>
{
  using type = RiskManagerRef;
};

}  // namespace flox
