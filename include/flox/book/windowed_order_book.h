/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/abstract_order_book.h"
#include "flox/book/book_side.h"
#include "flox/engine/events/book_update_event.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory_resource>
#include <mutex>
#include <optional>

namespace flox {

class WindowedOrderBook : public IOrderBook {
public:
  WindowedOrderBook(double tickSize, double expectedDeviation,
                    std::pmr::memory_resource *mem)
      : _tickSize(tickSize), _invTickSize(1 / tickSize),
        _windowSize(
            static_cast<size_t>(std::ceil((expectedDeviation * 2) / tickSize))),
        _halfWindowSize(_windowSize / 2), _centerPrice(0.0), _basePrice(0.0),
        _bids(_windowSize, BookSide::Side::Bid, mem),
        _asks(_windowSize, BookSide::Side::Ask, mem) {}

  void applyBookUpdate(const BookUpdateEvent &update) override {
    std::scoped_lock lock(_mutex);

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();

    for (const auto &lvl : update.bids) {
      minPrice = std::min(minPrice, lvl.price);
      maxPrice = std::max(maxPrice, lvl.price);
    }
    for (const auto &lvl : update.asks) {
      minPrice = std::min(minPrice, lvl.price);
      maxPrice = std::max(maxPrice, lvl.price);
    }

    if (_centerPrice == 0.0 || update.type == BookUpdateType::SNAPSHOT) {
      if (minPrice <= maxPrice) {
        shiftWindow(0.5 * (minPrice + maxPrice));
      }
    } else {
      bool needsShift = false;
      for (const auto &lvl : update.bids) {
        if (!isPriceInWindow(lvl.price)) {
          needsShift = true;
          break;
        }
      }
      if (!needsShift) {
        for (const auto &lvl : update.asks) {
          if (!isPriceInWindow(lvl.price)) {
            needsShift = true;
            break;
          }
        }
      }
      if (needsShift && minPrice <= maxPrice) {
        shiftWindow(0.5 * (minPrice + maxPrice));
      }
    }

    if (update.type == BookUpdateType::SNAPSHOT) {
      std::pmr::vector<bool> bidsTouched(_windowSize, false, _bids.allocator());
      for (const auto &lvl : update.bids) {
        double offset = lvl.price - _basePrice;
        if (offset >= 0 && offset < _tickSize * _windowSize) {
          auto idx = static_cast<size_t>(std::round(offset * _invTickSize));
          bidsTouched[idx] = true;
          _bids.setLevel(idx, lvl.quantity);
        }
      }
      for (std::size_t i = 0; i < _windowSize; ++i) {
        if (!bidsTouched[i])
          _bids.setLevel(i, 0.0);
      }

      std::pmr::vector<bool> asksTouched(_windowSize, false, _asks.allocator());
      for (const auto &lvl : update.asks) {
        double offset = lvl.price - _basePrice;
        if (offset >= 0 && offset < _tickSize * _windowSize) {
          auto idx = static_cast<size_t>(std::round(offset * _invTickSize));
          asksTouched[idx] = true;
          _asks.setLevel(idx, lvl.quantity);
        }
      }
      for (std::size_t i = 0; i < _windowSize; ++i) {
        if (!asksTouched[i])
          _asks.setLevel(i, 0.0);
      }

      return;
    }

    // INCREMENTAL UPDATE
    for (const auto &lvl : update.bids) {
      double offset = lvl.price - _basePrice;
      if (offset >= 0 && offset < _tickSize * _windowSize) {
        auto idx = static_cast<size_t>(std::round(offset * _invTickSize));
        _bids.setLevel(idx, lvl.quantity);
      }
    }

    for (const auto &lvl : update.asks) {
      double offset = lvl.price - _basePrice;
      if (offset >= 0 && offset < _tickSize * _windowSize) {
        auto idx = static_cast<size_t>(std::round(offset * _invTickSize));
        _asks.setLevel(idx, lvl.quantity);
      }
    }
  }

  size_t priceToIndex(double price) const {
    return static_cast<size_t>(std::round((price - _basePrice) * _invTickSize));
  }

  double indexToPrice(size_t index) const {
    return _basePrice + index * _tickSize;
  }

  bool isPriceInWindow(double price) const {
    double offset = price - _basePrice;
    return offset >= 0 && offset < _tickSize * _windowSize;
  }

  double bidAtPrice(double price) const override {
    std::scoped_lock lock(_mutex);
    if (!isPriceInWindow(price))
      return 0.0;
    return _bids.getLevel(priceToIndex(price));
  }

  double askAtPrice(double price) const override {
    std::scoped_lock lock(_mutex);
    if (!isPriceInWindow(price))
      return 0.0;
    return _asks.getLevel(priceToIndex(price));
  }

  std::optional<double> bestBid() const override {
    std::scoped_lock lock(_mutex);
    auto idx = _bids.findBest();
    return idx.has_value() ? std::optional<double>{indexToPrice(*idx)}
                           : std::nullopt;
  }

  std::optional<double> bestAsk() const override {
    std::scoped_lock lock(_mutex);
    auto idx = _asks.findBest();
    return idx.has_value() ? std::optional<double>{indexToPrice(*idx)}
                           : std::nullopt;
  }

  double getBidQuantity(double price) const {
    std::scoped_lock lock(_mutex);
    size_t index = priceToIndex(price);
    if (index >= _windowSize)
      return 0.0;
    return _bids.getLevel(index);
  }

  double getAskQuantity(double price) const {
    std::scoped_lock lock(_mutex);
    size_t index = priceToIndex(price);
    if (index >= _windowSize)
      return 0.0;
    return _asks.getLevel(index);
  }

  double centerPrice() const { return _centerPrice; }

  void printBook(std::size_t depth = 10) const {
    std::scoped_lock lock(_mutex);
    std::cout << "=== WindowedOrderBook Snapshot (center=" << _centerPrice
              << ") ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << " Asks (price x qty):\n";
    for (int i = static_cast<int>(_windowSize) - 1; i >= 0; --i) {
      const auto &lvl = _asks.getLevel(i);
      if (lvl > 0.0) {
        std::cout << "  " << indexToPrice(i) << " x " << lvl << "\n";
      }
    }

    std::cout << " Bids (price x qty):\n";
    for (std::size_t i = 0; i < _windowSize; ++i) {
      const auto &lvl = _bids.getLevel(i);
      if (lvl > 0.0) {
        std::cout << "  " << indexToPrice(i) << " x " << lvl << "\n";
      }
    }

    std::cout << "=============================================\n";
  }

private:
  void shiftWindow(double newPrice) {
    double newBase =
        std::round((newPrice - _tickSize * _halfWindowSize) * _invTickSize) *
        _tickSize;

    int shift =
        static_cast<int>(std::round((newBase - _basePrice) * _invTickSize));

    if (_centerPrice == 0.0 ||
        std::abs(shift) >= static_cast<int>(_windowSize)) {
      _bids.clear();
      _asks.clear();
    } else if (shift != 0) {
      _bids.shift(shift);
      _asks.shift(shift);
    }

    _basePrice = newBase;
    _centerPrice = newPrice;
  }

  double _tickSize;
  double _invTickSize;
  std::size_t _windowSize;
  std::size_t _halfWindowSize;

  double _centerPrice;
  double _basePrice;

  BookSide _bids;
  BookSide _asks;

  mutable std::mutex _mutex;
};

} // namespace flox
