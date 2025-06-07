#include "demo/demo_strategy.h"

#include <iostream>

namespace demo
{

DemoStrategy::DemoStrategy(SymbolId symbol, std::unique_ptr<IOrderBook> book)
    : _symbol(symbol), _book(std::move(book))
{
}

void DemoStrategy::onStart()
{
  std::cout << "[strategy " << _symbol << "] start" << std::endl;
}

void DemoStrategy::onStop()
{
  std::cout << "[strategy " << _symbol << "] stop" << std::endl;
}

void DemoStrategy::onTrade(const TradeEvent& ev)
{
  if (ev.trade.symbol != _symbol)
    return;

  Order order{};
  order.id = ++_nextId;
  order.side = Side::BUY;
  order.price = ev.trade.price;
  order.quantity = Quantity::fromDouble(1.0);
  order.type = OrderType::LIMIT;
  order.symbol = _symbol;
  order.createdAt = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now().time_since_epoch());

  std::string reason;
  if (GetOrderValidator() && !GetOrderValidator()->validate(order, reason))
  {
    std::cout << "[strategy " << _symbol << "] order rejected: " << reason << '\n';
    return;
  }
  if (GetRiskManager() && !GetRiskManager()->allow(order))
    return;

  if (GetOrderExecutor())
    GetOrderExecutor()->submitOrder(order);
}

void DemoStrategy::onBookUpdate(const BookUpdateEvent& ev)
{
  if (ev.update.symbol == _symbol && _book)
    _book->applyBookUpdate(ev);
}

}  // namespace demo
