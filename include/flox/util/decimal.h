/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <cassert>
#include <compare>
#include <cstdint>

namespace flox
{

template <typename Tag, int Scale_, int64_t TickSize_ = 1>
class Decimal
{
 public:
  static constexpr int Scale = Scale_;
  static constexpr int64_t TickSize = TickSize_;

  constexpr Decimal() : _raw(0) {}
  explicit constexpr Decimal(int64_t raw) : _raw(raw) {}

  static constexpr Decimal fromDouble(double val)
  {
    return Decimal(static_cast<int64_t>(val * Scale + 0.5));
  }
  static constexpr Decimal fromRaw(int64_t raw) { return Decimal(raw); }

  constexpr double toDouble() const { return static_cast<double>(_raw) / Scale; }

  constexpr int64_t raw() const { return _raw; }

  constexpr Decimal roundToTick() const { return Decimal((_raw / TickSize) * TickSize); }

  constexpr auto operator<=>(const Decimal&) const = default;
  constexpr bool operator==(const Decimal&) const = default;

  constexpr bool operator<(const Decimal& other) const { return _raw < other._raw; }
  constexpr bool operator>(const Decimal& other) const { return _raw > other._raw; }
  constexpr bool operator<=(const Decimal& other) const { return _raw <= other._raw; }
  constexpr bool operator>=(const Decimal& other) const { return _raw >= other._raw; }

  constexpr Decimal operator+(Decimal d) const { return Decimal(_raw + d._raw); }
  constexpr Decimal operator-(Decimal d) const { return Decimal(_raw - d._raw); }

  constexpr Decimal& operator+=(const Decimal& other)
  {
    _raw += other._raw;
    return *this;
  }

  constexpr Decimal& operator-=(const Decimal& other)
  {
    _raw -= other._raw;
    return *this;
  }

  constexpr bool isZero() const { return _raw == 0; }

 private:
  int64_t _raw;
};

}  // namespace flox
