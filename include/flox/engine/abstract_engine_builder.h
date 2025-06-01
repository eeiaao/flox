/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/abstract_engine.h"

#include <memory>

namespace flox {

class IEngineBuilder {
public:
  virtual ~IEngineBuilder() = default;

  virtual std::unique_ptr<IEngine> build() = 0;
};

} // namespace flox
