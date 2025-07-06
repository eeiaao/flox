/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <concepts>

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/engine/subscriber_component.h"
#include "flox/engine/subsystem_component.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept CandleAggregator =
    requires(T t, const T& ct, const TradeEvent& te, const BookUpdateEvent& be, const CandleEvent& ce) {
      { ct.id() } -> std::same_as<SubscriberId>;
      { ct.mode() } -> std::same_as<SubscriberMode>;
      { t.onTrade(te) };
      { t.onBookUpdate(be) };
      { t.onCandle(ce) };
      { t.start() };
      { t.stop() };
    };

}  // namespace concepts

namespace traits
{

struct CandleAggregatorTrait
{
  struct VTable
  {
    const MarketDataSubscriberTrait::VTable* mds;
    const SubsystemTrait::VTable* subsystem;

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, SubsystemTrait>)
        return subsystem;
      else if constexpr (std::is_same_v<Trait, MarketDataSubscriberTrait>)
        return mds;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::CandleAggregator<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto mds = MarketDataSubscriberTrait::makeVTable<T>();
    static constexpr auto ss = SubsystemTrait::makeVTable<T>();

    return {
        .mds = &mds,
        .subsystem = &ss,
    };
  }
};

}  // namespace traits

class CandleAggregatorRef : public RefBase<CandleAggregatorRef, traits::CandleAggregatorTrait>
{
  using VTable = traits::CandleAggregatorTrait::VTable;
  using Base = RefBase<CandleAggregatorRef, traits::CandleAggregatorTrait>;

 public:
  using Base::Base;

  CandleAggregatorRef(void* ptr, const VTable* vtable)
      : Base(ptr, vtable), _mds(vtable->mds) {}

  SubscriberId id() const { return _mds->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _mds->subscriber->mode(_ptr); }

  void onTrade(const TradeEvent& ev) const { _mds->onTrade(_ptr, ev); }
  void onBookUpdate(const BookUpdateEvent& ev) const { _mds->onBookUpdate(_ptr, ev); }
  void onCandle(const CandleEvent& ev) const { _mds->onCandle(_ptr, ev); }

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

 private:
  const traits::MarketDataSubscriberTrait::VTable* _mds = nullptr;
};
static_assert(concepts::CandleAggregator<CandleAggregatorRef>);

template <>
struct RefFor<traits::CandleAggregatorTrait>
{
  using type = CandleAggregatorRef;
};

}  // namespace flox
