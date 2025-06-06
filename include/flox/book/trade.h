#pragma once

#include <chrono>
#include "flox/common.h"

namespace flox
{

struct Trade
{
  SymbolId symbol{};
  Price price{};
  Quantity quantity{};
  bool isBuy{false};
  std::chrono::system_clock::time_point timestamp{};
};

}  // namespace flox
