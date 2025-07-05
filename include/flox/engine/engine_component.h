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
