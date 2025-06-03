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

#include <benchmark/benchmark.h>
#include <random>

using namespace flox;

static void BM_CandleAggregator_OnTrade(benchmark::State &state) {
  constexpr SymbolId SYMBOL = 42;
  constexpr std::chrono::seconds INTERVAL(60);

  // Create aggregator with no-op callback
  CandleAggregator aggregator(INTERVAL, [](SymbolId, const Candle &) {});
  aggregator.start();

  std::mt19937 rng(42);
  std::uniform_real_distribution<> priceDist(100.0, 110.0);
  std::uniform_real_distribution<> qtyDist(1.0, 5.0);

  int64_t baseTs = 0;
  EventPool<TradeEvent, 127> tradePool;

  for (auto _ : state) {
    auto trade = tradePool.acquire();

    trade->symbol = SYMBOL;
    trade->price = Price::fromDouble(priceDist(rng));
    trade->quantity = Quantity::fromDouble(qtyDist(rng));
    trade->isBuy = true;
    trade->timestamp =
        std::chrono::system_clock::time_point(std::chrono::seconds(baseTs++));

    aggregator.onMarketData(*trade);
  }

  aggregator.stop();
}

// Registers the benchmark with 1M iterations
BENCHMARK(BM_CandleAggregator_OnTrade)->Iterations(1'000'000);

BENCHMARK_MAIN();