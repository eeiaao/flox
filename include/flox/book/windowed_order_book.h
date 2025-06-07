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
#include "flox/book/events/book_update_event.h"
#include "flox/common.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory_resource>
#include <mutex>
#include <optional>

namespace flox
{

class WindowedOrderBook : public IOrderBook
{
 public:
  WindowedOrderBook(Price tickSize, Price expectedDeviation, std::pmr::memory_resource* mem)
      : _tickSize(tickSize),
        _invTickSize(1.0 / tickSize.toDouble()),
        _windowSize(static_cast<size_t>(
            std::ceil((expectedDeviation.toDouble() * 2) / tickSize.toDouble()))),
        _halfWindowSize(_windowSize / 2),
        _centerPrice(Price(0)),
        _basePrice(Price(0)),
        _bids(_windowSize, BookSide::Side::Bid, mem),
        _asks(_windowSize, BookSide::Side::Ask, mem)
  {
  }

  void applyBookUpdate(const BookUpdateEvent& event) override
  {
    std::scoped_lock lock(_mutex);

    const auto& update = event.update;

    Price minPrice = Price(std::numeric_limits<int64_t>::max());
    Price maxPrice = Price(std::numeric_limits<int64_t>::lowest());

    for (const auto& lvl : update.bids)
    {
      minPrice = std::min(minPrice, lvl.price);
      maxPrice = std::max(maxPrice, lvl.price);
    }
    for (const auto& lvl : update.asks)
    {
      minPrice = std::min(minPrice, lvl.price);
      maxPrice = std::max(maxPrice, lvl.price);
    }

    if (_centerPrice.raw() == 0 || update.type == BookUpdateType::SNAPSHOT)
    {
      if (minPrice.raw() <= maxPrice.raw())
      {
        shiftWindow(Price((minPrice.raw() + maxPrice.raw()) / 2));
      }
    }
    else
    {
      bool needsShift = false;
      for (const auto& lvl : update.bids)
      {
        if (!isPriceInWindow(lvl.price))
        {
          needsShift = true;
          break;
        }
      }
      if (!needsShift)
      {
        for (const auto& lvl : update.asks)
        {
          if (!isPriceInWindow(lvl.price))
          {
            needsShift = true;
            break;
          }
        }
      }
      if (needsShift && minPrice.raw() <= maxPrice.raw())
      {
        shiftWindow(Price((minPrice.raw() + maxPrice.raw()) / 2));
      }
    }

    if (update.type == BookUpdateType::SNAPSHOT)
    {
      std::pmr::vector<bool> bidsTouched(_windowSize, false, _bids.allocator());
      for (const auto& lvl : update.bids)
      {
        int64_t offset = lvl.price.raw() - _basePrice.raw();
        if (offset >= 0 && offset < static_cast<int64_t>(_tickSize.raw() * _windowSize))
        {
          auto idx = static_cast<size_t>(offset / _tickSize.raw());
          bidsTouched[idx] = true;
          _bids.setLevel(idx, lvl.quantity);
        }
      }
      for (std::size_t i = 0; i < _windowSize; ++i)
      {
        if (!bidsTouched[i])
          _bids.setLevel(i, Quantity(0));
      }

      std::pmr::vector<bool> asksTouched(_windowSize, false, _asks.allocator());
      for (const auto& lvl : update.asks)
      {
        int64_t offset = lvl.price.raw() - _basePrice.raw();
        if (offset >= 0 && offset < static_cast<int64_t>(_tickSize.raw() * _windowSize))
        {
          auto idx = static_cast<size_t>(offset / _tickSize.raw());
          asksTouched[idx] = true;
          _asks.setLevel(idx, lvl.quantity);
        }
      }
      for (std::size_t i = 0; i < _windowSize; ++i)
      {
        if (!asksTouched[i])
          _asks.setLevel(i, Quantity(0));
      }

      return;
    }

    // INCREMENTAL UPDATE
    for (const auto& lvl : update.bids)
    {
      int64_t offset = lvl.price.raw() - _basePrice.raw();
      if (offset >= 0 && offset < static_cast<int64_t>(_tickSize.raw() * _windowSize))
      {
        auto idx = static_cast<size_t>(offset / _tickSize.raw());
        _bids.setLevel(idx, lvl.quantity);
      }
    }

    for (const auto& lvl : update.asks)
    {
      int64_t offset = lvl.price.raw() - _basePrice.raw();
      if (offset >= 0 && offset < static_cast<int64_t>(_tickSize.raw() * _windowSize))
      {
        auto idx = static_cast<size_t>(offset / _tickSize.raw());
        _asks.setLevel(idx, lvl.quantity);
      }
    }
  }

  size_t priceToIndex(Price price) const
  {
    return static_cast<size_t>((price.raw() - _basePrice.raw()) / _tickSize.raw());
  }

  Price indexToPrice(size_t index) const
  {
    return Price(_basePrice.raw() + index * _tickSize.raw());
  }

  bool isPriceInWindow(Price price) const
  {
    int64_t offset = price.raw() - _basePrice.raw();
    return offset >= 0 && offset < static_cast<int64_t>(_tickSize.raw() * _windowSize);
  }

  Quantity bidAtPrice(Price price) const override
  {
    std::scoped_lock lock(_mutex);
    if (!isPriceInWindow(price))
      return Quantity(0);
    return _bids.getLevel(priceToIndex(price));
  }

  Quantity askAtPrice(Price price) const override
  {
    std::scoped_lock lock(_mutex);
    if (!isPriceInWindow(price))
      return Quantity(0);
    return _asks.getLevel(priceToIndex(price));
  }

  std::optional<Price> bestBid() const override
  {
    std::scoped_lock lock(_mutex);
    auto idx = _bids.findBest();
    return idx.has_value() ? std::optional<Price>{indexToPrice(*idx)} : std::nullopt;
  }

  std::optional<Price> bestAsk() const override
  {
    std::scoped_lock lock(_mutex);
    auto idx = _asks.findBest();
    return idx.has_value() ? std::optional<Price>{indexToPrice(*idx)} : std::nullopt;
  }

  Quantity getBidQuantity(Price price) const
  {
    std::scoped_lock lock(_mutex);
    size_t index = priceToIndex(price);
    if (index >= _windowSize)
      return Quantity(0);
    return _bids.getLevel(index);
  }

  Quantity getAskQuantity(Price price) const
  {
    std::scoped_lock lock(_mutex);
    size_t index = priceToIndex(price);
    if (index >= _windowSize)
      return Quantity(0);
    return _asks.getLevel(index);
  }

  Price centerPrice() const { return _centerPrice; }

  void printBook(std::size_t depth = 10) const
  {
    std::scoped_lock lock(_mutex);
    std::cout << "=== WindowedOrderBook Snapshot (center=" << _centerPrice.toDouble()
              << ") ===\n";
    std::cout << std::fixed << std::setprecision(6);

    std::cout << " Asks (price x qty):\n";
    for (int i = static_cast<int>(_windowSize) - 1; i >= 0; --i)
    {
      const auto& lvl = _asks.getLevel(i);
      if (lvl.raw() > 0)
      {
        std::cout << "  " << indexToPrice(i).toDouble() << " x " << lvl.toDouble() << "\n";
      }
    }

    std::cout << " Bids (price x qty):\n";
    for (std::size_t i = 0; i < _windowSize; ++i)
    {
      const auto& lvl = _bids.getLevel(i);
      if (lvl.raw() > 0)
      {
        std::cout << "  " << indexToPrice(i).toDouble() << " x " << lvl.toDouble() << "\n";
      }
    }

    std::cout << "=============================================\n";
  }

 private:
  void shiftWindow(Price newPrice)
  {
    int64_t newBaseRaw =
        static_cast<int64_t>(std::round(
            (newPrice.toDouble() - _tickSize.toDouble() * _halfWindowSize) * _invTickSize)) *
        _tickSize.raw();

    int shift = static_cast<int>(std::round((newBaseRaw - _basePrice.raw()) / _tickSize.raw()));

    if (_centerPrice.raw() == 0 || std::abs(shift) >= static_cast<int>(_windowSize))
    {
      _bids.clear();
      _asks.clear();
    }
    else if (shift != 0)
    {
      _bids.shift(shift);
      _asks.shift(shift);
    }

    _basePrice = Price(newBaseRaw);
    _centerPrice = newPrice;
  }

  Price _tickSize;
  double _invTickSize;
  std::size_t _windowSize;
  std::size_t _halfWindowSize;

  Price _centerPrice;
  Price _basePrice;

  BookSide _bids;
  BookSide _asks;

  mutable std::mutex _mutex;
};

}  // namespace flox