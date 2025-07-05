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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace flox
{

class ConnectorFactory
{
 public:
  using CreatorFunc =
      std::move_only_function<std::shared_ptr<ExchangeConnector>(const std::string&)>;

  static ConnectorFactory& instance()
  {
    static ConnectorFactory factory;
    return factory;
  }

  void registerConnector(const std::string& type, CreatorFunc creator)
  {
    _creators[type] = std::move(creator);
  }

  std::optional<std::shared_ptr<ExchangeConnector>> createConnector(const std::string& type,
                                                                    const std::string& symbol)
  {
    auto it = _creators.find(type);
    if (it != _creators.end())
    {
      return it->second(symbol);
    }

    return std::nullopt;
  }

 private:
  ConnectorFactory() = default;
  std::unordered_map<std::string, CreatorFunc> _creators;
};

}  // namespace flox
