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

namespace flox
{

Engine::Engine(const EngineConfig& config, std::vector<std::unique_ptr<ISubsystem>> subsystems,
               std::vector<std::shared_ptr<ExchangeConnector>> connectors)
    : _config(config), _subsystems(std::move(subsystems)), _connectors(std::move(connectors))
{
}

void Engine::start()
{
  for (auto& connector : _connectors)
  {
    connector->start();
  }

  for (auto& subsystem : _subsystems)
  {
    subsystem->start();
  }
}

void Engine::stop()
{
  std::reverse(_subsystems.begin(), _subsystems.end());
  for (auto& subsystem : _subsystems)
  {
    subsystem->stop();
  }

  for (auto& connector : _connectors)
  {
    connector->stop();
  }
}

}  // namespace flox