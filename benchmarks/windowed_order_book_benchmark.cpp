/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/book_update.h"
#include "flox/book/book_update_factory.h"
#include "flox/book/windowed_order_book.h"
#include "flox/book/windowed_order_book_factory.h"

#include <benchmark/benchmark.h>
#include <random>

#include <iostream>

using namespace flox;

static void BM_ApplyBookUpdate(benchmark::State &state) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;

  WindowedOrderBookFactory factory;
  auto *book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  std::mt19937 rng(42);
  std::uniform_real_distribution<> priceDist(19900, 20100);
  std::uniform_real_distribution<> qtyDist(1, 5);

  for (auto _ : state) {
    BookUpdateFactory bookUpdateFactory;
    auto snapshot = bookUpdateFactory.create();

    snapshot.type = BookUpdateType::DELTA;
    snapshot.timestamp = std::chrono::system_clock::now();

    for (int i = 0; i < 10000; ++i) {
      double price = priceDist(rng);
      double qty = qtyDist(rng);
      snapshot.bids.push_back({price, qty});
      snapshot.asks.push_back({price + 10, qty});
    }

    book->applyBookUpdate(snapshot);
  }
}
BENCHMARK(BM_ApplyBookUpdate)->Unit(benchmark::kMicrosecond);

static void BM_BestBid(benchmark::State &state) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 5000.0; // 0.1 * 100,000 / 2

  WindowedOrderBookFactory factory;
  auto *book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  BookUpdateFactory bookUpdateFactory;
  auto snapshot = bookUpdateFactory.create();

  snapshot.type = BookUpdateType::DELTA;
  snapshot.timestamp = std::chrono::system_clock::now();

  for (int i = 0; i < 100000; ++i) {
    snapshot.bids.push_back({20000.0 - i * tickSize, 1});
  }
  book->applyBookUpdate(snapshot);

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

  BookUpdateFactory bookUpdateFactory;
  auto snapshot = bookUpdateFactory.create();

  snapshot.type = BookUpdateType::DELTA;
  snapshot.timestamp = std::chrono::system_clock::now();

  for (int i = 0; i < 100000; ++i) {
    snapshot.asks.push_back({20000.0 + i * tickSize, 1});
  }
  book->applyBookUpdate(snapshot);

  for (auto _ : state) {
    benchmark::DoNotOptimize(book->bestAsk());
  }
}
BENCHMARK(BM_BestAsk)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
