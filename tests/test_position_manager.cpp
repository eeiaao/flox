/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/execution/order.h"
#include "flox/position/position_manager.h"

#include <gtest/gtest.h>

using namespace flox;

constexpr SymbolId BTC = 1;
constexpr SymbolId ETH = 2;

static Order makeOrder(SymbolId symbol, Side side, double qty)
{
  Order order{};
  order.id = 0;
  order.side = side;
  order.price = Price::fromDouble(0);
  order.quantity = Quantity::fromDouble(qty);
  order.type = OrderType::LIMIT;
  order.symbol = symbol;
  order.createdAt = std::chrono::nanoseconds(0);
  return order;
}

TEST(PositionManager, IncreasesOnBuy)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 1.234567));
  EXPECT_EQ(pm.getPosition(BTC), Quantity::fromDouble(1.234567));
}

TEST(PositionManager, DecreasesOnSell)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 2.0));
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.5));
  EXPECT_EQ(pm.getPosition(BTC), Quantity::fromDouble(1.5));
}

TEST(PositionManager, CanBeNegative)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.25));
  EXPECT_EQ(pm.getPosition(BTC), Quantity::fromDouble(-0.25));
}

TEST(PositionManager, UnknownSymbolIsZero)
{
  PositionManager pm;
  EXPECT_EQ(pm.getPosition(ETH), Quantity::fromRaw(0));
}

TEST(PositionManager, MultipleSymbols)
{
  PositionManager pm;
  pm.onOrderFilled(makeOrder(BTC, Side::BUY, 1.0));
  pm.onOrderFilled(makeOrder(ETH, Side::BUY, 2.0));
  pm.onOrderFilled(makeOrder(BTC, Side::SELL, 0.5));
  EXPECT_EQ(pm.getPosition(BTC), Quantity::fromDouble(0.5));
  EXPECT_EQ(pm.getPosition(ETH), Quantity::fromDouble(2.0));
}
