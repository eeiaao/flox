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

#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept Subsystem = requires(T t) {
  { t.start() } -> std::same_as<void>;
  { t.stop() } -> std::same_as<void>;
};

}  // namespace concepts

namespace traits
{

struct SubsystemTrait
{
  struct VTable
  {
    void (*start)(void*);
    void (*stop)(void*);
  };

  template <typename T>
    requires concepts::Subsystem<T>
  static constexpr VTable makeVTable()
  {
    return {
        .start = meta::wrap<&T::start>(),
        .stop = meta::wrap<&T::stop>(),
    };
  }
};

}  // namespace traits

class SubsystemRef : public RefBase<SubsystemRef, traits::SubsystemTrait>
{
  using VTable = traits::SubsystemTrait::VTable;
  using Base = RefBase<SubsystemRef, traits::SubsystemTrait>;

 public:
  using Base::Base;

  void start() const { _vtable->start(_ptr); }
  void stop() const { _vtable->stop(_ptr); }
};
static_assert(concepts::Subsystem<SubsystemRef>);

template <>
struct RefFor<traits::SubsystemTrait>
{
  using type = SubsystemRef;
};

}  // namespace flox
