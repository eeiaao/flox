/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/windowed_order_book_factory.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <benchmark/benchmark.h>
#include <random>

using namespace flox;

using BookUpdatePool = EventPool<BookUpdateEvent, 63>;

static void BM_ApplyBookUpdate(benchmark::State &state) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;

  WindowedOrderBookFactory factory;
  auto *book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});
  BookUpdatePool pool;

  std::mt19937 rng(42);
  std::uniform_real_distribution<> priceDist(19900, 20100);
  std::uniform_real_distribution<> qtyDist(1, 5);

  for (auto _ : state) {
    auto update = pool.acquire();
    update->type = BookUpdateType::DELTA;
    update->timestamp = std::chrono::system_clock::now();
    update->bids.clear();
    update->asks.clear();
    update->bids.reserve(10000);
    update->asks.reserve(10000);

    for (int i = 0; i < 10000; ++i) {
      double price = priceDist(rng);
      double qty = qtyDist(rng);
      update->bids.push_back({price, qty});
      update->asks.push_back({price + 10.0, qty});
    }

    book->applyBookUpdate(*update);
  }
}
BENCHMARK(BM_ApplyBookUpdate)->Unit(benchmark::kMicrosecond);

static void BM_BestBid(benchmark::State &state) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 5000.0;

  WindowedOrderBookFactory factory;
  auto *book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});
  BookUpdatePool pool;

  auto update = pool.acquire();
  update->type = BookUpdateType::SNAPSHOT;
  update->timestamp = std::chrono::system_clock::now();
  update->asks.clear();
  update->bids.reserve(100000);

  for (int i = 0; i < 100000; ++i) {
    update->bids.push_back({20000.0 - i * tickSize, 1.0});
  }

  book->applyBookUpdate(*update);

  for (auto _ : state) {
    benchmark::DoNotOptimize(book->bestBid());
  }
}
BENCHMARK(BM_BestBid)->Unit(benchmark::kNanosecond);

static void BM_BestAsk(benchmark::State &state) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 5000.0;

  WindowedOrderBookFactory factory;
  auto *book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});
  BookUpdatePool pool;

  auto update = pool.acquire();
  update->type = BookUpdateType::SNAPSHOT;
  update->timestamp = std::chrono::system_clock::now();
  update->bids.clear();
  update->asks.reserve(100000);

  for (int i = 0; i < 100000; ++i) {
    update->asks.push_back({20000.0 + i * tickSize, 1.0});
  }

  book->applyBookUpdate(*update);

  for (auto _ : state) {
    benchmark::DoNotOptimize(book->bestAsk());
  }
}
BENCHMARK(BM_BestAsk)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
