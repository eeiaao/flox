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

namespace flox
{

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
    assert(prev > 0 && "release called on zero refcount");
    return prev == 1;
  }

  void resetRefCount(uint32_t value = 1) noexcept
  {
    _refCount.store(value, std::memory_order_relaxed);
  }

  uint32_t refCount() const noexcept { return _refCount.load(std::memory_order_relaxed); }

 protected:
  std::atomic<uint32_t> _refCount{0};
};

}  // namespace flox
