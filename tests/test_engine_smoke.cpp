/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/common.h"
#include "flox/engine/abstract_engine_builder.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/events/trade_event.h"
#include "flox/engine/market_data_bus.h"
#include "flox/engine/market_data_event_pool.h"
#include "flox/strategy/abstract_strategy.h"

#include <gtest/gtest.h>

using namespace flox;

// Smoke test: demonstrates how to create a strategy, wire it into the engine,
// publish trades and book updates via a mock connector, and receive results.

namespace
{

class TestStrategy : public IStrategy
{
 public:
  explicit TestStrategy(SymbolId symbol) : _symbol(symbol) {}

  void onTrade(TradeEvent* trade) override
  {
    if (trade->symbol == _symbol)
    {
      ++_seenTrades;
      _lastTradePrice = trade->price;
    }
  }

  void onBookUpdate(BookUpdateEvent* update) override
  {
    if (update->symbol == _symbol && !update->bids.empty())
    {
      ++_seenBooks;
      _lastBid = update->bids[0].price;
    }
  }

  int seenTrades() const { return _seenTrades; }
  int seenBooks() const { return _seenBooks; }
  Price lastTradePrice() const { return _lastTradePrice; }
  Price lastBid() const { return _lastBid; }

 private:
  SymbolId _symbol;
  int _seenTrades = 0;
  int _seenBooks = 0;
  Price _lastTradePrice = Price::fromDouble(0.0);
  Price _lastBid = Price::fromDouble(0.0);
};

using TradePool = EventPool<TradeEvent, 7>;
using BookUpdatePool = EventPool<BookUpdateEvent, 7>;

class MockConnector
{
 public:
  MockConnector(MarketDataBus& bus, TradePool& tradePool, BookUpdatePool& bookPool,
                SymbolId symbol)
      : _bus(bus), _tradePool(tradePool), _bookPool(bookPool), _symbol(symbol)
  {
  }

  void publishTrade(Price price, Quantity qty)
  {
    auto event = _tradePool.acquire();
    event->symbol = _symbol;
    event->price = price;
    event->quantity = qty;
    event->isBuy = true;
    event->timestamp = std::chrono::system_clock::now();
    _bus.publish(std::move(event));
  }

  void publishBook(Price bidPrice, Quantity bidQty)
  {
    auto event = _bookPool.acquire();
    event->symbol = _symbol;
    event->type = BookUpdateType::SNAPSHOT;
    event->bids.push_back({bidPrice, bidQty});
    _bus.publish(std::move(event));
  }

 private:
  MarketDataBus& _bus;
  TradePool& _tradePool;
  BookUpdatePool& _bookPool;
  SymbolId _symbol;
};

class SmokeEngineBuilder : public IEngineBuilder
{
 public:
  SmokeEngineBuilder(SymbolId symbol, std::shared_ptr<TestStrategy> strategy)
      : _symbol(symbol), _strategy(std::move(strategy))
  {
  }

  std::unique_ptr<IEngine> build() override
  {
    _bus = std::make_unique<MarketDataBus>();
    _tradePool = std::make_unique<TradePool>();
    _bookPool = std::make_unique<BookUpdatePool>();
    _connector = std::make_unique<MockConnector>(*_bus, *_tradePool, *_bookPool, _symbol);

    _bus->subscribe(_strategy);
    return std::make_unique<EngineImpl>(*_bus, *_connector, _strategy);
  }

  class EngineImpl : public IEngine
  {
   public:
    EngineImpl(MarketDataBus& bus, MockConnector& connector,
               std::shared_ptr<TestStrategy> strategy)
        : _bus(bus), _connector(connector), _strategy(std::move(strategy))
    {
    }

    void start() override {}
    void stop() override {}

    void runTrade(Price price, Quantity qty) { _connector.publishTrade(price, qty); }
    void runBook(Price price, Quantity qty) { _connector.publishBook(price, qty); }

    std::shared_ptr<TestStrategy> strategy() const { return _strategy; }

   private:
    MarketDataBus& _bus;
    MockConnector& _connector;
    std::shared_ptr<TestStrategy> _strategy;
  };

  SymbolId _symbol;
  std::shared_ptr<TestStrategy> _strategy;
  std::unique_ptr<MarketDataBus> _bus;
  std::unique_ptr<TradePool> _tradePool;
  std::unique_ptr<BookUpdatePool> _bookPool;
  std::unique_ptr<MockConnector> _connector;
};

}  // namespace

TEST(SmokeEngineTest, StrategyReceivesBothEvents)
{
  constexpr SymbolId SYMBOL = 777;
  auto strategy = std::make_shared<TestStrategy>(SYMBOL);
  SmokeEngineBuilder builder(SYMBOL, strategy);
  auto engineBase = builder.build();
  auto engine = static_cast<SmokeEngineBuilder::EngineImpl*>(engineBase.get());

  engine->start();
  engine->runTrade(Price::fromDouble(101.25), Quantity::fromDouble(10.0));
  engine->runBook(Price::fromDouble(101.10), Quantity::fromDouble(5.0));
  engine->stop();

  EXPECT_EQ(strategy->seenTrades(), 1);
  EXPECT_EQ(strategy->seenBooks(), 1);
  EXPECT_EQ(strategy->lastTradePrice(), Price::fromDouble(101.25));
  EXPECT_EQ(strategy->lastBid(), Price::fromDouble(101.10));
}
