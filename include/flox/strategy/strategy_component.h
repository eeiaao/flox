/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/engine/subsystem_component.h"

namespace flox
{

namespace concepts
{

template <typename T>
concept Strategy = MarketDataSubscriber<T> && Subsystem<T>;

}  // namespace concepts

namespace traits
{

struct StrategyTrait
{
  struct VTable
  {
    const SubsystemTrait::VTable* subsystem;
    const MarketDataSubscriberTrait::VTable* mds;

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
    requires concepts::Strategy<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto subsystem = SubsystemTrait::makeVTable<T>();
    static constexpr auto mds = MarketDataSubscriberTrait::makeVTable<T>();
    return {
        .subsystem = &subsystem,
        .mds = &mds,
    };
  }
};

}  // namespace traits

class StrategyRef : public RefBase<StrategyRef, traits::StrategyTrait>
{
  using VTable = traits::StrategyTrait::VTable;
  using Base = RefBase<StrategyRef, traits::StrategyTrait>;

 public:
  using Base::Base;

  StrategyRef(void* ptr, const VTable* vtable)
      : Base(ptr, vtable), _mds(vtable->mds) {}

  void start() const { _vtable->subsystem->start(_ptr); }
  void stop() const { _vtable->subsystem->stop(_ptr); }

  SubscriberId id() const { return _mds->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _mds->subscriber->mode(_ptr); }

  void onTrade(const TradeEvent& ev) const { _mds->onTrade(_ptr, ev); }
  void onBookUpdate(const BookUpdateEvent& ev) const { _mds->onBookUpdate(_ptr, ev); }
  void onCandle(const CandleEvent& ev) const { _mds->onCandle(_ptr, ev); }

 private:
  const traits::MarketDataSubscriberTrait::VTable* _mds = nullptr;
};
static_assert(concepts::Strategy<StrategyRef>);

template <>
struct RefFor<traits::StrategyTrait>
{
  using type = StrategyRef;
};

}  // namespace flox
