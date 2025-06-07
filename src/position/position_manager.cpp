/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/position/position_manager.h"
#include "flox/common.h"

#include <iostream>

namespace flox
{

void PositionManager::onOrderFilled(const Order& order)
{
  if (order.symbol >= _positions.size())
    return;

  if (order.side == Side::BUY)
  {
    _positions[order.symbol] += order.quantity;
  }
  else
  {
    _positions[order.symbol] -= order.quantity;
  }
}

void PositionManager::onOrderPartiallyFilled(const Order& order, Quantity qty)
{
  if (order.symbol >= _positions.size())
    return;

  Order partial = order;
  partial.quantity = qty;
  onOrderFilled(partial);
}

void PositionManager::onOrderRejected(const Order&)
{
  // no-op
}

Quantity PositionManager::getPosition(SymbolId symbol) const
{
  if (symbol >= _positions.size())
    return Quantity{0};

  return _positions[symbol];
}

void PositionManager::printPositions() const
{
  std::cout << "=== Current Positions ===\n";
  for (SymbolId symbol = 0; symbol < _positions.size(); ++symbol)
  {
    if (!_positions[symbol].isZero())
    {
      std::cout << symbol << ": " << _positions[symbol] << '\n';
    }
  }
  std::cout << "==========================\n";
}

}  // namespace flox
