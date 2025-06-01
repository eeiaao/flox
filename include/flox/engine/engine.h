/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/connector/exchange_connector.h"
#include "flox/engine/abstract_engine.h"
#include "flox/engine/engine_config.h"
#include "flox/engine/subsystem.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace flox {

struct ExchangeInstance {
  std::string exchangeType;
  std::string name;
  std::string symbol;
  std::shared_ptr<ExchangeConnector> connector;
};

class Engine : public IEngine {
public:
  Engine(const EngineConfig &config,
         std::vector<std::unique_ptr<ISubsystem>> subsystems,
         std::vector<std::shared_ptr<ExchangeConnector>> connectors);

  void start() override;
  void stop() override;

private:
  EngineConfig _config;

  std::vector<std::unique_ptr<ISubsystem>> _subsystems;
  std::vector<std::shared_ptr<ExchangeConnector>> _connectors;
};

} // namespace flox
