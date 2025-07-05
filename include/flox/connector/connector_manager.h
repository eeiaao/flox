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

#include <map>
#include <memory>

namespace flox
{

class ConnectorManager
{
 public:
  void registerConnector(std::shared_ptr<ExchangeConnector> connector)
  {
    const auto& id = connector->exchangeId();
    connectors[id] = connector;
  }

  void startAll(ExchangeConnector::BookUpdateCallback onBookUpdate,
                ExchangeConnector::TradeCallback onTrade)
  {
    for (auto& [symbol, connector] : connectors)
    {
      connector->setCallbacks([onBookUpdate = std::move(onBookUpdate)](const BookUpdateEvent& update) mutable
                              { onBookUpdate(update); },
                              [onTrade = std::move(onTrade)](const TradeEvent& trade) mutable
                              { onTrade(trade); });
      connector->start();
    }
  }

 private:
  std::map<std::string, std::shared_ptr<ExchangeConnector>> connectors;
};

}  // namespace flox