#pragma once

#include <memory>
#include <type_traits>

#include "flox/book/bus/book_update_bus.h"
#include "flox/book/bus/trade_bus.h"

namespace flox
{

class MarketDataBus
{
 public:
  using Queue = BookUpdateBus::Queue;

  void subscribe(std::shared_ptr<IMarketDataSubscriber> sub)
  {
    _bookBus.subscribe(sub);
    _tradeBus.subscribe(sub);
  }

  Queue* getQueue(SubscriberId id) { return _bookBus.getQueue(id); }

  template <typename T>
  void publish(EventHandle<T> event)
    requires std::is_same_v<T, BookUpdateEvent> || std::is_same_v<T, TradeEvent>
  {
    if constexpr (std::is_same_v<T, BookUpdateEvent>)
      _bookBus.publish(event);
    else
      _tradeBus.publish(event);
  }

  void start()
  {
    _bookBus.start();
    _tradeBus.start();
  }

  void stop()
  {
    _bookBus.stop();
    _tradeBus.stop();
  }

 private:
  BookUpdateBus _bookBus;
  TradeBus _tradeBus;
};

}  // namespace flox
