/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <chrono>

namespace flox {

struct Candle {
  double open = 0.0;
  double high = 0.0;
  double low = 0.0;
  double close = 0.0;
  double volume = 0.0;
  std::chrono::system_clock::time_point startTime;
  std::chrono::system_clock::time_point endTime;

  Candle() = default;

  Candle(std::chrono::system_clock::time_point ts, double price, double qty)
      : open(price), high(price), low(price), close(price), volume(qty),
        startTime(ts), endTime(ts) {}
};

} // namespace flox