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

#include "flox/engine/subsystem_component.h"
#include "flox/execution/order.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept StorageSink =
    Subsystem<T> &&
    requires(T& t, const Order& order) {
      { t.store(order) } -> std::same_as<void>;
    };

}  // namespace concepts

namespace traits
{

struct StorageSinkTrait
{
  struct VTable
  {
    const SubsystemTrait::VTable* subsystem;
    void (*store)(void*, const Order&);

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
    requires concepts::StorageSink<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto subsystem = SubsystemTrait::makeVTable<T>();
    return {
        .subsystem = &subsystem,
        .store = meta::wrap<&T::store>(),
    };
  }
};

}  // namespace traits

class StorageSinkRef : public RefBase<StorageSinkRef, traits::StorageSinkTrait>
{
  using VTable = traits::StorageSinkTrait::VTable;
  using Base = RefBase<StorageSinkRef, traits::StorageSinkTrait>;

 public:
  using Base::Base;

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

  void store(const Order& order) const { _vtable->store(_ptr, order); }
};
static_assert(concepts::StorageSink<StorageSinkRef>);

template <>
struct RefFor<traits::StorageSinkTrait>
{
  using type = StorageSinkRef;
};

}  // namespace flox
