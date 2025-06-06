#pragma once

#include <chrono>
#include <memory_resource>
#include <vector>
#include "flox/common.h"

namespace flox
{

enum class BookUpdateType
{
  SNAPSHOT,
  DELTA
};

struct BookLevel
{
  Price price{};
  Quantity quantity{};
  BookLevel() = default;
  BookLevel(Price p, Quantity q) : price(p), quantity(q) {}
};

struct BookUpdate
{
  SymbolId symbol{};
  BookUpdateType type{};
  std::pmr::vector<BookLevel> bids;
  std::pmr::vector<BookLevel> asks;
  std::chrono::system_clock::time_point timestamp{};

  BookUpdate(std::pmr::memory_resource* res) : bids(res), asks(res) {}
};

}  // namespace flox
