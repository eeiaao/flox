/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/book_side.h"
#include "flox/common.h"

#include <benchmark/benchmark.h>
#include <memory_resource>

using namespace flox;

static void BM_BookSideBestBid(benchmark::State& state)
{
  constexpr std::size_t levels = 100'000;
  std::pmr::monotonic_buffer_resource arena(10'000'000);
  BookSide side(levels, BookSide::Side::Bid, &arena);

  for (std::size_t i = 0; i < levels; ++i)
  {
    side.setLevel(i, Quantity::fromDouble(1.0));
  }

  for (auto _ : state)
  {
    benchmark::DoNotOptimize(side.findBest());
  }
}
BENCHMARK(BM_BookSideBestBid)->Unit(benchmark::kNanosecond);

static void BM_BookSideBestAsk(benchmark::State& state)
{
  constexpr std::size_t levels = 100'000;
  std::pmr::monotonic_buffer_resource arena(10'000'000);
  BookSide side(levels, BookSide::Side::Ask, &arena);

  for (std::size_t i = 0; i < levels; ++i)
  {
    side.setLevel(i, Quantity::fromDouble(1.0));
  }

  for (auto _ : state)
  {
    benchmark::DoNotOptimize(side.findBest());
  }
}
BENCHMARK(BM_BookSideBestAsk)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
