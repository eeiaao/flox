/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/order.h"
#include "flox/position/position_manager.h"

#include <gtest/gtest.h>

using namespace flox;

constexpr SymbolId BTC = 1;
constexpr SymbolId ETH = 2;

static Order makeOrder(SymbolId symbol, Side side, double qty)
{
  return Order{.id = 0,
               .side = side,
               .price = 0,
               .quantity = qty,
               .type = OrderType::LIMIT,
               .symbol = symbol,
               .timestamp = std::chrono::system_clock::now()};
}

TEST(PositionManager, IncreasesOnBuy)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 1.234567));
  EXPECT_NEAR(pm.getPosition(BTC), 1.234567, 1e-6);
}

TEST(PositionManager, DecreasesOnSell)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 2.0));
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.5));
  EXPECT_NEAR(pm.getPosition(BTC), 1.5, 1e-6);
}

TEST(PositionManager, CanBeNegative)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.25));
  EXPECT_NEAR(pm.getPosition(BTC), -0.25, 1e-6);
}

TEST(PositionManager, UnknownSymbolIsZero)
{
  PositionManager pm;
  EXPECT_NEAR(pm.getPosition(ETH), 0.0, 1e-6);
}

TEST(PositionManager, MultipleSymbols)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 1.0));
  pm.onOrderFilled(makeOrder(ETH, Side::BUY, 2.0));
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.5));
  EXPECT_NEAR(pm.getPosition(BTC), 0.5, 1e-6);
  EXPECT_NEAR(pm.getPosition(ETH), 2.0, 1e-6);
}
