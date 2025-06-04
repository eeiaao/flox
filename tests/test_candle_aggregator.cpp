/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/aggregator/candle_aggregator.h"
#include "flox/common.h"
#include "flox/engine/events/trade_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>

using namespace flox;

namespace
{

constexpr SymbolId SYMBOL = 42;
const std::chrono::seconds INTERVAL = std::chrono::seconds(60);

std::chrono::system_clock::time_point ts(int seconds)
{
  return std::chrono::system_clock::time_point(std::chrono::seconds(seconds));
}

using TradePool = EventPool<TradeEvent, 15>;

EventHandle<TradeEvent> makeTrade(TradePool& pool, SymbolId symbol, double price, double qty,
                                  int sec)
{
  auto handle = pool.acquire();
  handle->symbol = symbol;
  handle->price = Price::fromDouble(price);
  handle->quantity = Quantity::fromDouble(qty);
  handle->isBuy = true;
  handle->timestamp = ts(sec);
  return handle;
}

}  // namespace

TEST(CandleAggregatorTest, AggregatesTradesIntoCandles)
{
  std::vector<Candle> result;
  CandleAggregator aggregator(INTERVAL, [&](SymbolId, const Candle& c)
                              { result.push_back(c); });

  EventPool<TradeEvent, 15> pool;
  aggregator.start();

  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 100, 1, 0));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 105, 2, 10));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 99, 3, 20));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 101, 1, 30));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 102, 2, 65));  // triggers flush

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
  CandleAggregator aggregator(INTERVAL, [&](SymbolId, const Candle& c)
                              { result.push_back(c); });

  EventPool<TradeEvent, 15> pool;
  aggregator.start();

  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 100, 1, 0));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 105, 1, 30));

  aggregator.stop();  // flush remaining

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
  CandleAggregator aggregator(INTERVAL, [&](SymbolId, const Candle& c)
                              { result.push_back(c); });

  TradePool pool;
  aggregator.start();

  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 110, 1, 0));
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 120, 2, 130));  // gap → flush

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].startTime, ts(0));
  EXPECT_EQ(result[0].endTime, ts(60));
  EXPECT_EQ(result[0].close, Price::fromDouble(110.0));
}

TEST(CandleAggregatorTest, SingleTradeCandle)
{
  std::vector<Candle> result;
  CandleAggregator aggregator(INTERVAL, [&](SymbolId, const Candle& c)
                              { result.push_back(c); });

  TradePool pool;
  aggregator.start();
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 123, 1, 5));
  aggregator.stop();  // flush

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
  std::vector<std::pair<SymbolId, Candle>> result;
  CandleAggregator aggregator(
      INTERVAL, [&](SymbolId symbol, const Candle& c)
      { result.emplace_back(symbol, c); });

  TradePool pool;
  aggregator.start();

  aggregator.onMarketData(*makeTrade(pool, 1, 10, 1, 0));
  aggregator.onMarketData(*makeTrade(pool, 2, 20, 2, 10));
  aggregator.onMarketData(*makeTrade(pool, 1, 12, 1, 30));
  aggregator.onMarketData(*makeTrade(pool, 2, 18, 1, 40));

  aggregator.stop();  // flush all

  ASSERT_EQ(result.size(), 2);

  auto it1 = std::find_if(result.begin(), result.end(), [](const auto& p)
                          { return p.first == 1; });
  auto it2 = std::find_if(result.begin(), result.end(), [](const auto& p)
                          { return p.first == 2; });

  ASSERT_TRUE(it1 != result.end());
  ASSERT_TRUE(it2 != result.end());

  EXPECT_EQ(it1->second.volume, Volume::fromDouble(10 * 1 + 12 * 1));
  EXPECT_EQ(it2->second.volume, Volume::fromDouble(20 * 2 + 18 * 1));
}

TEST(CandleAggregatorTest, DoubleStartClearsOldState)
{
  std::vector<Candle> result;
  CandleAggregator aggregator(INTERVAL, [&](SymbolId, const Candle& c)
                              { result.push_back(c); });

  TradePool pool;
  aggregator.start();
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 100, 1, 0));
  aggregator.start();  // clears previous state
  aggregator.onMarketData(*makeTrade(pool, SYMBOL, 105, 2, 65));
  aggregator.stop();

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result[0].open, Price::fromDouble(105.0));
  EXPECT_EQ(result[0].volume, Volume::fromDouble(105.0 * 2));
  EXPECT_EQ(result[0].startTime, ts(60));
}
