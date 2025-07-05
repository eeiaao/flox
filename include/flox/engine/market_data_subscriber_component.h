#pragma once

#include <concepts>

#include "flox/engine/subscriber_component.h"
#include "flox/util/base/ref.h"
#include "flox/util/meta/meta.h"

namespace flox
{

struct BookUpdateEvent;
struct TradeEvent;
struct CandleEvent;

namespace concepts
{

template <typename T>
concept MarketDataSubscriber =
    Subscriber<T> &&
    requires(T t, const BookUpdateEvent& b, const TradeEvent& tr, const CandleEvent& c) {
      { t.onBookUpdate(b) } -> std::same_as<void>;
      { t.onTrade(tr) } -> std::same_as<void>;
      { t.onCandle(c) } -> std::same_as<void>;
    };

}  // namespace concepts

namespace traits
{

struct MarketDataSubscriberTrait
{
  struct VTable
  {
    const SubscriberTrait::VTable* subscriber;

    void (*onBookUpdate)(void*, const BookUpdateEvent&);
    void (*onTrade)(void*, const TradeEvent&);
    void (*onCandle)(void*, const CandleEvent&);

    template <typename Trait>
    const typename Trait::VTable* as() const
    {
      if constexpr (std::is_same_v<Trait, SubscriberTrait>)
        return subscriber;
      else
        static_assert(sizeof(Trait) == 0, "Trait not supported by this VTable");
    }
  };

  template <typename T>
    requires concepts::MarketDataSubscriber<T>
  static constexpr VTable makeVTable()
  {
    static constexpr auto sub = SubscriberTrait::makeVTable<T>();
    return {
        .subscriber = &sub,
        .onBookUpdate = meta::wrap<&T::onBookUpdate>(),
        .onTrade = meta::wrap<&T::onTrade>(),
        .onCandle = meta::wrap<&T::onCandle>(),
    };
  }
};

}  // namespace traits

class MarketDataSubscriberRef : public RefBase<MarketDataSubscriberRef, traits::MarketDataSubscriberTrait>
{
  using VTable = traits::MarketDataSubscriberTrait::VTable;
  using Base = RefBase<MarketDataSubscriberRef, traits::MarketDataSubscriberTrait>;

 public:
  using Base::Base;

  SubscriberId id() const { return _vtable->subscriber->id(_ptr); }
  SubscriberMode mode() const { return _vtable->subscriber->mode(_ptr); }

  void onTrade(const TradeEvent& ev) const { _vtable->onTrade(_ptr, ev); }
  void onBookUpdate(const BookUpdateEvent& ev) const { _vtable->onBookUpdate(_ptr, ev); }
  void onCandle(const CandleEvent& ev) const { _vtable->onCandle(_ptr, ev); }
};
static_assert(concepts::MarketDataSubscriber<MarketDataSubscriberRef>);

template <>
struct RefFor<traits::MarketDataSubscriberTrait>
{
  using type = MarketDataSubscriberRef;
};

}  // namespace flox
