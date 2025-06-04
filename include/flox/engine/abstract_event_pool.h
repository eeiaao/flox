/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

namespace flox
{

class IMarketDataEvent;

class IEventPool
{
 public:
  virtual ~IEventPool() = default;
  virtual void release(IMarketDataEvent* event) = 0;
};

}  // namespace flox
