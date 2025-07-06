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
#include <cstdint>

#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

namespace flox
{

using SubscriberId = uint64_t;

enum class SubscriberMode
{
  PUSH,
  PULL
};

namespace concepts
{

template <typename T>
concept SubscriberDot = requires(T t) {
  { t.id() } -> std::same_as<SubscriberId>;
  { t.mode() } -> std::same_as<SubscriberMode>;
};

template <typename T>
concept SubscriberArrow = requires(T t) {
  { t->id() } -> std::same_as<SubscriberId>;
  { t->mode() } -> std::same_as<SubscriberMode>;
};

template <typename T>
concept Subscriber = SubscriberDot<T> || SubscriberArrow<T>;

}  // namespace concepts

namespace traits
{

struct SubscriberVTable
{
  SubscriberId (*id)(const void*);
  SubscriberMode (*mode)(const void*);
};

struct SubscriberTrait
{
  struct VTable
  {
    SubscriberId (*id)(const void*);
    SubscriberMode (*mode)(const void*);
  };

  template <typename T>
    requires concepts::Subscriber<T>
  static constexpr VTable makeVTable()
  {
    return {
        .id = meta::wrap<&T::id>(),
        .mode = meta::wrap<&T::mode>(),
    };
  }
};

}  // namespace traits

class SubscriberRef : public RefBase<SubscriberRef, traits::SubscriberTrait>
{
  using VTable = traits::SubscriberTrait::VTable;
  using Base = RefBase<SubscriberRef, traits::SubscriberTrait>;

 public:
  using Base::Base;

  SubscriberId id() const { return _vtable->id(_ptr); }
  SubscriberMode mode() const { return _vtable->mode(_ptr); }
};
static_assert(concepts::Subscriber<SubscriberRef>);

template <>
struct RefFor<traits::SubscriberTrait>
{
  using type = SubscriberRef;
};

}  // namespace flox
