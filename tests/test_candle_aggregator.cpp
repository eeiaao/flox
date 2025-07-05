/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/aggregator/bus/candle_bus.h"
#include "flox/aggregator/candle_aggregator.h"
#include "flox/aggregator/events/candle_event.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/common.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/strategy/strategy_component.h"
#include "flox/util/base/ref.h"

#include <gtest/gtest.h>

using namespace flox;

namespace
{

constexpr SymbolId SYMBOL = 42;
const std::chrono::seconds INTERVAL = std::chrono::seconds(60);

std::chrono::steady_clock::time_point ts(int seconds)
{
  return std::chrono::steady_clock::time_point(std::chrono::seconds(seconds));
}

TradeEvent makeTrade(SymbolId symbol, double price, double qty,
                     int sec)
{
  TradeEvent event;

  event.trade.symbol = symbol;
  event.trade.price = Price::fromDouble(price);
  event.trade.quantity = Quantity::fromDouble(qty);
  event.trade.isBuy = true;
  event.trade.timestamp = ts(sec);

  return event;
}

class TestStrategy
{
 public:
  using Trait = traits::StrategyTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit TestStrategy(SubscriberId id, std::vector<Candle>& out, std::vector<SymbolId>* symOut = nullptr)
      : _id(id), _out(out), _symOut(symOut)
  {
  }

  void start() {}
  void stop() {}

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void onBookUpdate(const BookUpdateEvent&) {}
  void onTrade(const TradeEvent&) {}
  void onCandle(const CandleEvent& event)
  {
    _out.push_back(event.candle);
    if (_symOut)
      _symOut->push_back(event.symbol);
  }

 private:
  SubscriberId _id;
  std::vector<Candle>& _out;
  std::vector<SymbolId>* _symOut;
};

static_assert(concepts::Strategy<TestStrategy>);

}  // namespace

TEST(CandleAggregatorTest, AggregatesTradesIntoCandles)
{
  std::vector<Candle> result;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, result);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(SYMBOL, 100, 1, 0));
  aggregator.onTrade(makeTrade(SYMBOL, 105, 2, 10));
  aggregator.onTrade(makeTrade(SYMBOL, 99, 3, 20));
  aggregator.onTrade(makeTrade(SYMBOL, 101, 1, 30));
  aggregator.onTrade(makeTrade(SYMBOL, 102, 2, 65));  // triggers flush

  bus.stop();

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].open, Price::fromDouble(100.0));
  EXPECT_EQ(result[0].high, Price::fromDouble(105.0));
  EXPECT_EQ(result[0].low, Price::fromDouble(99.0));
  EXPECT_EQ(result[0].close, Price::fromDouble(101.0));
  EXPECT_EQ(result[0].volume, Volume::fromDouble(100 * 1 + 105 * 2 + 99 * 3 + 101 * 1));
  EXPECT_EQ(result[0].startTime, ts(0));
  EXPECT_EQ(result[0].endTime, ts(60));
}

TEST(CandleAggregatorTest, FlushesFinalCandleOnStop)
{
  std::vector<Candle> result;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, result);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(SYMBOL, 100, 1, 0));
  aggregator.onTrade(makeTrade(SYMBOL, 105, 1, 30));

  aggregator.stop();  // flush remaining
  bus.stop();

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].open, Price::fromDouble(100.0));
  EXPECT_EQ(result[0].high, Price::fromDouble(105.0));
  EXPECT_EQ(result[0].low, Price::fromDouble(100.0));
  EXPECT_EQ(result[0].close, Price::fromDouble(105.0));
  EXPECT_EQ(result[0].volume, Volume::fromDouble(100 * 1 + 105 * 1));
}

TEST(CandleAggregatorTest, StartsNewCandleAfterGap)
{
  std::vector<Candle> result;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, result);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(SYMBOL, 110, 1, 0));
  aggregator.onTrade(makeTrade(SYMBOL, 120, 2, 130));  // gap → flush

  bus.stop();
  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].startTime, ts(0));
  EXPECT_EQ(result[0].endTime, ts(60));
  EXPECT_EQ(result[0].close, Price::fromDouble(110.0));
}

TEST(CandleAggregatorTest, SingleTradeCandle)
{
  std::vector<Candle> result;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, result);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(SYMBOL, 123, 1, 5));
  aggregator.stop();  // flush
  bus.stop();

  ASSERT_EQ(result.size(), 1);
  const auto& candle = result[0];
  EXPECT_EQ(candle.open, Price::fromDouble(123.0));
  EXPECT_EQ(candle.high, Price::fromDouble(123.0));
  EXPECT_EQ(candle.low, Price::fromDouble(123.0));
  EXPECT_EQ(candle.close, Price::fromDouble(123.0));
  EXPECT_EQ(candle.volume, Volume::fromDouble(123.0 * 1));
}

TEST(CandleAggregatorTest, MultipleSymbolsAreAggregatedSeparately)
{
  std::vector<Candle> candles;
  std::vector<SymbolId> symbols;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, candles, &symbols);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(1, 10, 1, 0));
  aggregator.onTrade(makeTrade(2, 20, 2, 10));
  aggregator.onTrade(makeTrade(1, 12, 1, 30));
  aggregator.onTrade(makeTrade(2, 18, 1, 40));

  aggregator.stop();  // flush all
  bus.stop();

  ASSERT_EQ(candles.size(), 2);

  auto it1 = std::find_if(symbols.begin(), symbols.end(), [](SymbolId s)
                          { return s == 1; });
  auto it2 = std::find_if(symbols.begin(), symbols.end(), [](SymbolId s)
                          { return s == 2; });

  ASSERT_TRUE(it1 != symbols.end());
  ASSERT_TRUE(it2 != symbols.end());

  EXPECT_EQ(candles[0].volume + candles[1].volume,
            Volume::fromDouble(10 * 1 + 12 * 1 + 20 * 2 + 18 * 1));
}

TEST(CandleAggregatorTest, DoubleStartClearsOldState)
{
  std::vector<Candle> result;
  auto bus = make<CandleBus>();
  bus.enableDrainOnStop();
  auto aggregator = make<CandleAggregator>(INTERVAL, bus);
  auto strat = make<TestStrategy>(1, result);
  bus.subscribe(strat.as<traits::MarketDataSubscriberTrait>());

  bus.start();
  aggregator.start();

  aggregator.onTrade(makeTrade(SYMBOL, 100, 1, 0));
  aggregator.start();  // clears previous state
  aggregator.onTrade(makeTrade(SYMBOL, 105, 2, 65));
  aggregator.stop();
  bus.stop();

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].open, Price::fromDouble(105.0));
  EXPECT_EQ(result[0].volume, Volume::fromDouble(105.0 * 2));
  EXPECT_EQ(result[0].startTime, ts(60));
}
