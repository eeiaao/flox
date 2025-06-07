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

#include <vector>

namespace flox
{

class PositionManager : public IPositionManager, public IOrderExecutionListener, public ISubsystem
{
 public:
  void start() override {}
  void stop() override {}

  void onOrderAccepted(const Order&) override {}
  void onOrderPartiallyFilled(const Order& order, Quantity qty) override;
  void onOrderFilled(const Order& order) override;
  void onOrderCanceled(const Order&) override {}
  void onOrderExpired(const Order&) override {}
  void onOrderRejected(const Order& order) override;
  void onOrderReplaced(const Order&, const Order&) override {}

  Quantity getPosition(SymbolId symbol) const override;
  void printPositions() const;

 private:
  static constexpr size_t MAX_SYMBOLS = 65'536;

  std::vector<Quantity> _positions = std::vector<Quantity>(MAX_SYMBOLS, Quantity{0});
};

}  // namespace flox
