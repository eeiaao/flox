/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/position/position_manager.h"

namespace flox
{

void PositionManager::onOrderFilled(const Order& order)
{
  if (order.symbol >= _positions.size())
    return;

  const auto scaledQty = toInternal(order.quantity);

  if (order.side == Side::BUY)
  {
    _positions[order.symbol] += scaledQty;
  }
  else
  {
    _positions[order.symbol] -= scaledQty;
  }
}

void PositionManager::onOrderRejected(const Order&, const std::string&)
{
  // no-op
}

double PositionManager::getPosition(SymbolId symbol) const
{
  if (symbol >= _positions.size())
    return 0.0;

  return toDisplay(_positions[symbol]);
}

void PositionManager::printPositions() const
{
  std::cout << "=== Current Positions ===\n";
  for (SymbolId symbol = 0; symbol < _positions.size(); ++symbol)
  {
    if (_positions[symbol] != 0)
    {
      std::cout << symbol << ": " << toDisplay(_positions[symbol]) << '\n';
    }
  }
  std::cout << "==========================\n";
}

}  // namespace flox
