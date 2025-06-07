/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/common.h"
#include "flox/execution/order.h"
#include "flox/position/position_manager.h"

#include <benchmark/benchmark.h>
#include <random>

using namespace flox;

static Order makeOrder(SymbolId symbol, Side side, double qty)
{
  auto now = std::chrono::system_clock::now().time_since_epoch();
  return Order{
      .id = 0,
      .side = side,
      .price = Price{},
      .quantity = Quantity::fromDouble(qty),
      .type = OrderType::LIMIT,
      .symbol = symbol,
      .status = OrderStatus::NEW,
      .createdAt = now};
}

static void BM_PositionManager_OnOrderFilled(benchmark::State& state)
{
  PositionManager pm;
  pm.start();

  std::mt19937 rng(42);
  std::uniform_int_distribution<SymbolId> symbolDist(0, 1000);
  std::uniform_real_distribution<double> qtyDist(0.000001, 10.0);
  std::bernoulli_distribution sideDist(0.5);

  std::vector<Order> orders;
  orders.reserve(state.max_iterations);

  for (int i = 0; i < state.max_iterations; ++i)
  {
    orders.emplace_back(
        makeOrder(symbolDist(rng), sideDist(rng) ? Side::BUY : Side::SELL, qtyDist(rng)));
  }

  for (auto _ : state)
  {
    for (const auto& order : orders)
    {
      pm.onOrderFilled(order);
    }
  }
}

BENCHMARK(BM_PositionManager_OnOrderFilled);
BENCHMARK_MAIN();
