/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/bus/book_update_bus.h"
#include "flox/book/bus/trade_bus.h"
#include "flox/book/events/book_update_event.h"
#include "flox/connector/exchange_connector.h"
#include "flox/util/memory/pool.h"

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
  DemoConnector(const std::string& id, SymbolId symbol,
                BookUpdateBusRef bookUpdateBus,
                TradeBusRef tradeBus);
  void start() override;
  void stop() override;
  std::string exchangeId() const override { return _id; }

 private:
  void run();

  std::string _id;
  SymbolId _symbol;
  BookUpdateBusRef _bookUpdateBus;
  TradeBusRef _tradeBus;
  std::atomic<bool> _running{false};
  std::thread _thread;
  std::mt19937 _rng{std::random_device{}()};
  pool::Pool<BookUpdateEvent, 7> _bookPool;
};

}  // namespace demo
