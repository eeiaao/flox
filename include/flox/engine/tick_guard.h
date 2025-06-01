/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/tick_barrier.h"

namespace flox {

class TickGuard {
public:
  explicit TickGuard(TickBarrier &barrier) : _barrier(&barrier) {}

  ~TickGuard() {
    if (_barrier)
      _barrier->complete();
  }

  TickGuard(const TickGuard &) = delete;
  TickGuard &operator=(const TickGuard &) = delete;

private:
  TickBarrier *_barrier;
};

} // namespace flox
