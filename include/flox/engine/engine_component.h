/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/subsystem_component.h"

namespace flox::concepts
{

template <typename T>
concept Engine = Subsystem<T>;

template <typename T>
concept EngineBuilder = requires(T builder) {
  { builder.build() } -> Engine;
};

}  // namespace flox::concepts
