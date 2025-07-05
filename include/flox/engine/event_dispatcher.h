#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/execution/events/order_event.h"
#include "flox/util/memory/pool.h"

namespace flox
{

template <typename T>
struct EventDispatcher;

template <typename T>
struct EventDispatcher<pool::Handle<T>>
{
  static void dispatch(const pool::Handle<T>& ev, auto& sub)
  {
    EventDispatcher<T>::dispatch(*ev, sub);
  }
};

template <>
struct EventDispatcher<BookUpdateEvent>
{
  static void dispatch(const BookUpdateEvent& ev, auto& sub)
  {
    sub.onBookUpdate(ev);
  }
};

template <>
struct EventDispatcher<TradeEvent>
{
  static void dispatch(const TradeEvent& ev, auto& sub)
  {
    sub.onTrade(ev);
  }
};

template <>
struct EventDispatcher<CandleEvent>
{
  static void dispatch(const CandleEvent& ev, auto& sub)
  {
    sub.onCandle(ev);
  }
};

template <>
struct EventDispatcher<OrderEvent>
{
  static void dispatch(const OrderEvent& ev, auto& listener)
  {
    ev.dispatchTo(listener);
  }
};

}  // namespace flox
