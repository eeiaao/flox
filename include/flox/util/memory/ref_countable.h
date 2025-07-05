/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <atomic>
#include <cassert>
#include <concepts>
#include <cstdlib>

namespace flox
{

namespace concepts
{
template <typename T>
concept RefCountable = requires(T obj) {
  { obj.retain() } -> std::same_as<void>;
  { obj.release() } -> std::same_as<bool>;
  { obj.resetRefCount() } -> std::same_as<void>;
};
}  // namespace concepts

class RefCountable
{
 public:
  void retain() noexcept
  {
    assert(_refCount.load(std::memory_order_relaxed) >= 0);
    _refCount.fetch_add(1, std::memory_order_relaxed);
  }

  bool release() noexcept
  {
    auto prev = _refCount.fetch_sub(1, std::memory_order_acq_rel);
    if (prev == 0) [[unlikely]]
    {
#ifndef NDEBUG
      assert(false && "release called on zero refcount");
#endif
      std::abort();
    }

    return prev == 1;
  }

  void resetRefCount(uint32_t value = 0) noexcept
  {
    _refCount.store(value, std::memory_order_relaxed);
  }

  uint32_t refCount() const noexcept { return _refCount.load(std::memory_order_relaxed); }

 protected:
  std::atomic<uint32_t> _refCount{0};
};

static_assert(concepts::RefCountable<RefCountable>);

}  // namespace flox
