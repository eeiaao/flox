/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

namespace flox {

class IEngine {
public:
  virtual ~IEngine() = default;

  virtual void start() = 0;
  virtual void stop() = 0;
};

} // namespace flox
