/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/common.h"
#include "flox/engine/subsystem.h"
#include "flox/execution/abstract_execution_listener.h"
#include "flox/position/abstract_position_manager.h"

#include <cstdint>
#include <iostream>
#include <vector>

namespace flox
{

class PositionManager : public IPositionManager, public IOrderExecutionListener, public ISubsystem
{
 public:
  void start() override {}
  void stop() override {}

  void onOrderFilled(const Order& order) override;
  void onOrderRejected(const Order& order, const std::string& reason) override;

  double getPosition(SymbolId symbol) const override;
  void printPositions() const;

 private:
  static constexpr int64_t QTY_PRECISION = 1'000'000;
  static constexpr size_t MAX_SYMBOLS = 65'536;

  std::vector<int64_t> _positions = std::vector<int64_t>(MAX_SYMBOLS, 0);

  static int64_t toInternal(double qty)
  {
    return static_cast<int64_t>(qty * QTY_PRECISION + 0.5);
  }

  static double toDisplay(int64_t qty) { return static_cast<double>(qty) / QTY_PRECISION; }
};

}  // namespace flox
