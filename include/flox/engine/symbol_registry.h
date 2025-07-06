/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "flox/common.h"

namespace flox
{

class SymbolRegistry
{
 public:
  SymbolId registerSymbol(const std::string& exchange, const std::string& symbol);

  std::optional<SymbolId> getSymbolId(const std::string& exchange,
                                      const std::string& symbol) const;

  std::pair<std::string, std::string> getSymbolName(SymbolId id) const;

 private:
  mutable std::mutex _mutex;
  std::unordered_map<std::string, SymbolId> _map;
  std::vector<std::pair<std::string, std::string>> _reverse;
};

}  // namespace flox
