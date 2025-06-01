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
#include <cstddef>
#include <thread>

namespace flox {

class TickBarrier {
public:
  explicit TickBarrier(size_t total) : _total(total), _completed(0) {}

  void complete() { _completed.fetch_add(1, std::memory_order_acq_rel); }

  void wait() {
    while (_completed.load(std::memory_order_acquire) < _total) {
      std::this_thread::yield();
    }
  }

private:
  const size_t _total;
  std::atomic<size_t> _completed;
};

} // namespace flox
