/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/connector/exchange_connector.h"
#include "flox/engine/engine_component.h"
#include "flox/engine/engine_config.h"
#include "flox/engine/subsystem_component.h"

#include <memory>
#include <string>
#include <vector>

namespace flox
{

struct ExchangeInstance
{
  std::string exchangeType;
  std::string name;
  std::string symbol;
  std::shared_ptr<ExchangeConnector> connector;
};

class Engine
{
 public:
  Engine(const EngineConfig& config, std::vector<SubsystemRef> buses, std::vector<SubsystemRef> subsystems,
         std::vector<std::shared_ptr<ExchangeConnector>> connectors);

  void start();
  void stop();

 private:
  EngineConfig _config;

  std::vector<SubsystemRef> _buses;
  std::vector<SubsystemRef> _subsystems;
  std::vector<std::shared_ptr<ExchangeConnector>> _connectors;
};

static_assert(concepts::Engine<Engine>);

}  // namespace flox
