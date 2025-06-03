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
#include "flox/common.h"
#include "flox/engine/events/book_update_event.h"

#include <cmath>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace flox {

class FullOrderBook : public IOrderBook {
public:
  explicit FullOrderBook(Price tickSize)
      : _tickSize(tickSize), _minBidIndex(0), _maxBidIndex(0),
        _minAskIndex(SIZE_MAX), _maxAskIndex(0) {}

  void applyBookUpdate(const BookUpdateEvent &update) override {
    std::scoped_lock lock(_mutex);

    if (update.type == BookUpdateType::SNAPSHOT) {
      _bids.clear();
      _asks.clear();
      _minBidIndex = 0;
      _maxBidIndex = 0;
      _minAskIndex = SIZE_MAX;
      _maxAskIndex = 0;
    }

    for (const auto &[price, qty] : update.bids) {
      auto i = priceToIndex(price);
      if (qty.raw() == 0) {
        _bids.erase(i);
      } else {
        _bids[i] = qty;
        _minBidIndex = std::min(_minBidIndex, i);
        _maxBidIndex = std::max(_maxBidIndex, i);
      }
    }

    for (const auto &[price, qty] : update.asks) {
      auto i = priceToIndex(price);
      if (qty.raw() == 0) {
        _asks.erase(i);
      } else {
        _asks[i] = qty;
        _minAskIndex = std::min(_minAskIndex, i);
        _maxAskIndex = std::max(_maxAskIndex, i);
      }
    }
  }

  std::optional<Price> bestBid() const override {
    std::scoped_lock lock(_mutex);
    for (size_t i = _maxBidIndex + 1; i-- > _minBidIndex;) {
      auto it = _bids.find(i);
      if (it != _bids.end() && it->second.raw() > 0)
        return indexToPrice(i);
    }
    return std::nullopt;
  }

  std::optional<Price> bestAsk() const override {
    std::scoped_lock lock(_mutex);
    for (size_t i = _minAskIndex; i <= _maxAskIndex; ++i) {
      auto it = _asks.find(i);
      if (it != _asks.end() && it->second.raw() > 0)
        return indexToPrice(i);
    }
    return std::nullopt;
  }

  Quantity bidAtPrice(Price price) const override {
    std::scoped_lock lock(_mutex);
    auto it = _bids.find(priceToIndex(price));
    return it != _bids.end() ? it->second : Quantity(0);
  }

  Quantity askAtPrice(Price price) const override {
    std::scoped_lock lock(_mutex);
    auto it = _asks.find(priceToIndex(price));
    return it != _asks.end() ? it->second : Quantity(0);
  }

private:
  size_t priceToIndex(Price price) const {
    return static_cast<size_t>(price.raw() / _tickSize.raw());
  }

  Price indexToPrice(size_t index) const {
    return Price(static_cast<int64_t>(index) * _tickSize.raw());
  }

  Price _tickSize;

  mutable std::mutex _mutex;
  std::unordered_map<size_t, Quantity> _bids;
  std::unordered_map<size_t, Quantity> _asks;

  size_t _minBidIndex;
  size_t _maxBidIndex;
  size_t _minAskIndex;
  size_t _maxAskIndex;
};

} // namespace flox