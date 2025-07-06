/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <concepts>
#include <optional>

#include "flox/book/events/book_update_event.h"
#include "flox/common.h"
#include "flox/util/meta/meta.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept OrderBook = requires(T book, const BookUpdateEvent& ev, Price price) {
  { book.applyBookUpdate(ev) } -> std::same_as<void>;
  { book.bestBid() } -> std::same_as<std::optional<Price>>;
  { book.bestAsk() } -> std::same_as<std::optional<Price>>;
  { book.bidAtPrice(price) } -> std::same_as<Quantity>;
  { book.askAtPrice(price) } -> std::same_as<Quantity>;
};

}  // namespace concepts

namespace traits
{

struct OrderBookTrait
{
  struct VTable
  {
    void (*applyBookUpdate)(void*, const BookUpdateEvent&);
    std::optional<Price> (*bestBid)(void*);
    std::optional<Price> (*bestAsk)(void*);
    Quantity (*bidAtPrice)(void*, Price);
    Quantity (*askAtPrice)(void*, Price);
  };

  template <typename T>
    requires concepts::OrderBook<T>
  static constexpr VTable makeVTable()
  {
    return {
        .applyBookUpdate = meta::wrap<&T::applyBookUpdate>(),
        .bestBid = meta::wrap<&T::bestBid>(),
        .bestAsk = meta::wrap<&T::bestAsk>(),
        .bidAtPrice = meta::wrap<&T::bidAtPrice>(),
        .askAtPrice = meta::wrap<&T::askAtPrice>(),
    };
  }
};

}  // namespace traits

class OrderBookRef : public RefBase<OrderBookRef, traits::OrderBookTrait>
{
  using VTable = traits::OrderBookTrait::VTable;
  using Base = RefBase<OrderBookRef, traits::OrderBookTrait>;

 public:
  using Base::Base;

  void applyBookUpdate(const BookUpdateEvent& ev) const
  {
    _vtable->applyBookUpdate(_ptr, ev);
  }

  std::optional<Price> bestBid() const
  {
    return _vtable->bestBid(_ptr);
  }

  std::optional<Price> bestAsk() const
  {
    return _vtable->bestAsk(_ptr);
  }

  Quantity bidAtPrice(Price p) const
  {
    return _vtable->bidAtPrice(_ptr, p);
  }

  Quantity askAtPrice(Price p) const
  {
    return _vtable->askAtPrice(_ptr, p);
  }
};
static_assert(concepts::OrderBook<OrderBookRef>);

template <>
struct RefFor<traits::OrderBookTrait>
{
  using type = OrderBookRef;
};

}  // namespace flox
