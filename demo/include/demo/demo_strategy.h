#pragma once

#include "demo/simple_components.h"
#include "flox/book/full_order_book_factory.h"
#include "flox/book/windowed_order_book_factory.h"
#include "flox/strategy/abstract_strategy.h"

#include <memory>

namespace demo
{
using namespace flox;

class DemoStrategy : public IStrategy
{
 public:
  DemoStrategy(SymbolId symbol, IOrderBook* book);

  void onStart() override;
  void onStop() override;
  void onTrade(const TradeEvent& trade) override;
  void onBookUpdate(const BookUpdateEvent& book) override;

  IOrderBook* book() const { return _book; }

 private:
  SymbolId _symbol;
  IOrderBook* _book;
  OrderId _nextId{0};
};

}  // namespace demo
