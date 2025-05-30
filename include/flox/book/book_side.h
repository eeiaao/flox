/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <vector>

namespace flox {

class BookSide {
public:
  enum class Side { Bid, Ask };

  BookSide(std::size_t windowSize, Side side, std::pmr::memory_resource *mem)
      : _qty(mem), _offset(0), _windowSize(windowSize), _side(side) {
    _qty.resize(windowSize);
  }

  void setLevel(std::size_t index, double qty) {
    _qty[ring(index)] = qty;

    if (qty > 0.0) {
      if (!_bestIndex.has_value()) {
        _bestIndex = index;
      } else if (_side == Side::Bid) {
        if (index > *_bestIndex)
          _bestIndex = index;
      } else {
        if (index < *_bestIndex)
          _bestIndex = index;
      }
    } else if (_bestIndex.has_value() && index == *_bestIndex) {
      _bestIndex.reset(); // invalidate cache
    }
  }

  double getLevel(std::size_t index) const { return _qty[ring(index)]; }

  void shift(int levels) {
    if (std::abs(levels) >= static_cast<int>(_windowSize)) {
      clear();
      _offset = 0;
      return;
    }
    _offset = (_offset + levels + _windowSize) % _windowSize;
    _bestIndex.reset();
  }

  void clear() {
    std::fill(_qty.begin(), _qty.end(), 0.0);
    _bestIndex.reset();
  }

  std::optional<std::size_t> findBest() const {
    if (_bestIndex.has_value())
      return _bestIndex;

    if (_side == Side::Bid) {
      for (int i = static_cast<int>(_windowSize) - 1; i >= 0; --i) {
        std::size_t physical = (i + _offset) % _windowSize;
        if (_qty[physical] > 0.0)
          return i;
      }
    } else {
      for (std::size_t i = 0; i < _windowSize; ++i) {
        std::size_t physical = (i + _offset) % _windowSize;
        if (_qty[physical] > 0.0)
          return i;
      }
    }

    return std::nullopt;
  }

  std::pmr::memory_resource *allocator() const {
    return _qty.get_allocator().resource();
  }

protected:
  std::size_t ring(std::size_t index) const {
    return (index + _offset) % _windowSize;
  }

  std::pmr::vector<double> _qty;
  std::size_t _offset;
  std::size_t _windowSize;
  Side _side;

  mutable std::optional<std::size_t> _bestIndex;
};

} // namespace flox
