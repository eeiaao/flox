/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/engine/engine.h"

#include <algorithm>
#include <utility>

#include "flox/engine/subsystem_component.h"

namespace flox
{

Engine::Engine(const EngineConfig& config, std::vector<SubsystemRef> buses,
               std::vector<SubsystemRef> subsystems,
               std::vector<std::shared_ptr<ExchangeConnector>> connectors)
    : _config(config), _buses(std::move(buses)), _subsystems(std::move(subsystems)), _connectors(std::move(connectors))
{
}

void Engine::start()
{
  for (auto& bus : _buses)
  {
    bus.start();
  }

  for (auto& subsystem : _subsystems)
  {
    subsystem.start();
  }

  for (auto& connector : _connectors)
  {
    connector->start();
  }
}

void Engine::stop()
{
  for (auto& connector : _connectors)
  {
    connector->stop();
  }

  for (auto& bus : _buses)
  {
    bus.stop();
  }

  std::reverse(_subsystems.begin(), _subsystems.end());
  for (auto& subsystem : _subsystems)
  {
    subsystem.stop();
  }
}

}  // namespace flox