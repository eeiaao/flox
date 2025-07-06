/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/bus/book_update_bus.h"
#include "flox/book/bus/trade_bus.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/common.h"
#include "flox/engine/engine_component.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/strategy/strategy_component.h"
#include "flox/util/base/ref.h"
#include "flox/util/memory/pool.h"

#include <gtest/gtest.h>
#include <memory>

using namespace flox;

// Smoke test: demonstrates how to create a strategy, wire it into the engine,
// publish trades and book updates via a mock connector, and receive results.

namespace
{

class TestStrategy
{
 public:
  using Trait = traits::StrategyTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit TestStrategy(SubscriberId id, SymbolId symbol) : _id(id), _symbol(symbol) {}

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onTrade(const TradeEvent& event)
  {
    if (event.trade.symbol == _symbol)
    {
      ++_seenTrades;
      _lastTradePrice = event.trade.price;
    }
  }

  void onBookUpdate(const BookUpdateEvent& event)
  {
    if (event.update.symbol == _symbol && !event.update.bids.empty())
    {
      ++_seenBooks;
      _lastBid = event.update.bids[0].price;
    }
  }

  void onCandle(const CandleEvent&) {}

  int seenTrades() const { return _seenTrades; }
  int seenBooks() const { return _seenBooks; }
  Price lastTradePrice() const { return _lastTradePrice; }
  Price lastBid() const { return _lastBid; }

 private:
  SubscriberId _id;
  SymbolId _symbol;
  int _seenTrades = 0;
  int _seenBooks = 0;
  Price _lastTradePrice = Price::fromDouble(0.0);
  Price _lastBid = Price::fromDouble(0.0);
};
static_assert(concepts::Strategy<TestStrategy>);

using BookUpdatePool = pool::Pool<BookUpdateEvent, 7>;

class MockConnector
{
 public:
  MockConnector(BookUpdateBusRef bookUpdateBus, TradeBusRef tradeBus, BookUpdatePool& bookPool,
                SymbolId symbol)
      : _bookUpdateBus(bookUpdateBus), _tradeBus(tradeBus), _bookPool(bookPool), _symbol(symbol)
  {
  }

  void publishTrade(Price price, Quantity qty)
  {
    TradeEvent event;

    event.trade.symbol = _symbol;
    event.trade.price = price;
    event.trade.quantity = qty;
    event.trade.isBuy = true;
    event.trade.timestamp = std::chrono::steady_clock::now();

    _tradeBus.publish(event);
  }

  void publishBook(Price bidPrice, Quantity bidQty)
  {
    auto eventOpt = _bookPool.acquire();
    assert(eventOpt);
    auto& event = *eventOpt;
    event->update.symbol = _symbol;
    event->update.type = BookUpdateType::SNAPSHOT;
    event->update.bids.push_back({bidPrice, bidQty});
    _bookUpdateBus.publish(std::move(event));
  }

 private:
  BookUpdateBusRef _bookUpdateBus;
  TradeBusRef _tradeBus;

  BookUpdatePool& _bookPool;
  SymbolId _symbol;
};

class SmokeEngineBuilder
{
 public:
  SmokeEngineBuilder(SymbolId symbol, StrategyRef strategy)
      : _symbol(symbol), _strategy(strategy)
  {
  }

  auto build()
  {
    auto bookUpdateBus = make<BookUpdateBus>();
    auto tradeBus = make<TradeBus>();

    bookUpdateBus.enableDrainOnStop();
    tradeBus.enableDrainOnStop();

    auto pool = std::make_unique<BookUpdatePool>();
    auto connector = std::make_unique<MockConnector>(bookUpdateBus, tradeBus, *pool, _symbol);

    bookUpdateBus.subscribe(_strategy.as<traits::MarketDataSubscriberTrait>());
    tradeBus.subscribe(_strategy.as<traits::MarketDataSubscriberTrait>());

    return EngineImpl{
        bookUpdateBus,
        tradeBus,
        std::move(pool),
        std::move(connector),
        _strategy};
  }

 private:
  SymbolId _symbol;
  StrategyRef _strategy;

  struct EngineImpl
  {
    EngineImpl(BookUpdateBusRef bookUpdateBus,
               TradeBusRef tradeBus,
               std::unique_ptr<BookUpdatePool> pool,
               std::unique_ptr<MockConnector> connector,
               StrategyRef strategy)
        : _bookUpdateBus(bookUpdateBus),
          _tradeBus(tradeBus),
          _pool(std::move(pool)),
          _connector(std::move(connector)),
          _strategy(strategy)
    {
    }

    void start()
    {
      _bookUpdateBus.start();
      _tradeBus.start();
    }
    void stop()
    {
      _bookUpdateBus.stop();
      _tradeBus.stop();
    }

    void runTrade(Price price, Quantity qty)
    {
      _connector->publishTrade(price, qty);
    }

    void runBook(Price price, Quantity qty)
    {
      _connector->publishBook(price, qty);
    }

    auto strategy() const { return _strategy; }

   private:
    BookUpdateBusRef _bookUpdateBus;
    TradeBusRef _tradeBus;

    std::unique_ptr<BookUpdatePool> _pool;
    std::unique_ptr<MockConnector> _connector;
    StrategyRef _strategy;
  };

  static_assert(concepts::Engine<EngineImpl>);
};

static_assert(concepts::EngineBuilder<SmokeEngineBuilder>);

}  // namespace

TEST(SmokeEngineTest, StrategyReceivesBothEvents)
{
  constexpr SymbolId SYMBOL = 777;
  auto strategy = make<TestStrategy>(1, SYMBOL);
  SmokeEngineBuilder builder(SYMBOL, strategy);
  auto engine = builder.build();

  engine.start();
  engine.runTrade(Price::fromDouble(101.25), Quantity::fromDouble(10.0));
  engine.runBook(Price::fromDouble(101.10), Quantity::fromDouble(5.0));
  engine.stop();

  EXPECT_EQ(strategy.get<TestStrategy>().seenTrades(), 1);
  EXPECT_EQ(strategy.get<TestStrategy>().seenBooks(), 1);
  EXPECT_EQ(strategy.get<TestStrategy>().lastTradePrice(), Price::fromDouble(101.25));
  EXPECT_EQ(strategy.get<TestStrategy>().lastBid(), Price::fromDouble(101.10));
}
