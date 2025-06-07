#pragma once

#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/connector/exchange_connector.h"
#include "flox/engine/bus/market_data_bus.h"
#include "flox/engine/market_data_event_pool.h"

#include <atomic>
#include <random>
#include <string>
#include <thread>

namespace demo
{
using namespace flox;

class DemoConnector : public ExchangeConnector
{
 public:
  DemoConnector(const std::string& id, SymbolId symbol, MarketDataBus& bus);
  void start() override;
  void stop() override;
  std::string exchangeId() const override { return _id; }

 private:
  void run();

  std::string _id;
  SymbolId _symbol;
  MarketDataBus& _bus;
  std::atomic<bool> _running{false};
  std::thread _thread;
  std::mt19937 _rng{std::random_device{}()};
  EventPool<BookUpdateEvent, 7> _bookPool;
};

}  // namespace demo
