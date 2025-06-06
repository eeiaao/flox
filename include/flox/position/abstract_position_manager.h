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

namespace flox
{

class IPositionManager
{
 public:
  virtual ~IPositionManager() = default;

  virtual Quantity getPosition(SymbolId symbol) const = 0;
};

}  // namespace flox
