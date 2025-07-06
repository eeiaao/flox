/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/engine.h"
#include "flox/engine/engine_component.h"
#include "flox/engine/engine_config.h"

namespace demo
{
using namespace flox;

class DemoBuilder
{
 public:
  explicit DemoBuilder(const EngineConfig& cfg);
  Engine build();

 private:
  EngineConfig _config;
};

static_assert(flox::concepts::EngineBuilder<DemoBuilder>);

}  // namespace demo
