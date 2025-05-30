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
#include "flox/book/full_order_book.h"
#include "flox/book/full_order_book_factory.h"

#include <benchmark/benchmark.h>
#include <iostream>
#include <random>

using namespace flox;

static void BM_ApplyBookUpdate(benchmark::State &state) {
  constexpr double tickSize = 0.1;

  FullOrderBookFactory factory;
  auto *book = factory.create(FullOrderBookConfig{0.1});

  std::mt19937 rng(42);
  std::uniform_real_distribution<> priceDist(19900.0, 20100.0);
  std::uniform_real_distribution<> qtyDist(1.0, 5.0);

  for (auto _ : state) {
    BookUpdateFactory bookUpdateFactory;
    auto update = bookUpdateFactory.create();

    update.type = BookUpdateType::DELTA;
    update.timestamp = std::chrono::system_clock::now();

    for (int i = 0; i < 10000; ++i) {
      double price = priceDist(rng);
      double qty = qtyDist(rng);
      update.bids.push_back({price, qty});
      update.asks.push_back({price + 10.0, qty});
    }

    book->applyBookUpdate(update);
  }
}
BENCHMARK(BM_ApplyBookUpdate)->Unit(benchmark::kMicrosecond);

static void BM_BestBid(benchmark::State &state) {
  constexpr double tickSize = 0.1;

  FullOrderBookFactory factory;
  auto *book = factory.create(FullOrderBookConfig{0.1});

  BookUpdateFactory bookUpdateFactory;
  auto snapshot = bookUpdateFactory.create();

  snapshot.type = BookUpdateType::DELTA;
  snapshot.timestamp = std::chrono::system_clock::now();

  for (int i = 0; i < 100000; ++i) {
    snapshot.bids.push_back({20000.0 - i * tickSize, 1.0});
  }

  book->applyBookUpdate(snapshot);

  for (auto _ : state) {
    benchmark::DoNotOptimize(book->bestBid());
  }
}
BENCHMARK(BM_BestBid)->Unit(benchmark::kNanosecond);

static void BM_BestAsk(benchmark::State &state) {
  constexpr double tickSize = 0.1;

  FullOrderBookFactory factory;
  auto *book = factory.create(FullOrderBookConfig{0.1});

  BookUpdateFactory bookUpdateFactory;
  auto snapshot = bookUpdateFactory.create();

  snapshot.type = BookUpdateType::DELTA;
  snapshot.timestamp = std::chrono::system_clock::now();

  for (int i = 0; i < 100000; ++i) {
    snapshot.asks.push_back({20000.0 + i * tickSize, 1.0});
  }

  book->applyBookUpdate(snapshot);

  for (auto _ : state) {
    benchmark::DoNotOptimize(book->bestAsk());
  }
}
BENCHMARK(BM_BestAsk)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
