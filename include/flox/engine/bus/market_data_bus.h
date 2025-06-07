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

  void publish(EventHandle<BookUpdateEvent> ev)
  {
    _bookBus.publish(std::move(ev));
  }

  void publish(const TradeEvent& ev)
  {
    _tradeBus.publish(ev);
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
