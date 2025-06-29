#include "demo/demo_strategy.h"

#include <chrono>
#include <iostream>
#include "demo/latency_collector.h"

namespace demo
{

DemoStrategy::DemoStrategy(SymbolId symbol, IOrderBook* book)
    : _symbol(symbol), _book(book)
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

  {
    MEASURE_LATENCY(LatencyCollector::StrategyOnTrade);

    order.id = ++_nextId;
    order.side = (_nextId % 2 == 0 ? Side::BUY : Side::SELL);
    order.price = (_nextId % 2 == 0 ? ev.trade.price - Price::fromDouble(0.01)
                                    : ev.trade.price + Price::fromDouble(0.01));
    order.quantity = Quantity::fromDouble(1.0);
    order.type = OrderType::LIMIT;
    order.symbol = _symbol;
    order.createdAt = std::chrono::steady_clock::now();

    if (auto* ks = GetKillSwitch())
    {
      ks->check(order);

      if (ks->isTriggered())
      {
        std::cout << "[kill] strategy " << _symbol
                  << " blocked by kill switch"
                  << ", reason: " << ks->reason() << "\n";
        return;
      }
    }

    std::string reason;
    if (auto* validator = GetOrderValidator(); validator && !validator->validate(order, reason))
    {
      std::cout << "[strategy " << _symbol << "] order rejected: " << reason << '\n';
      return;
    }

    if (auto* risk = GetRiskManager(); risk && !risk->allow(order))
    {
      std::cout << "[risk] strategy " << _symbol << " rejected order id=" << order.id << '\n';
      return;
    }
  }

  if (auto* exec = GetOrderExecutor())
    exec->submitOrder(order);
}

void DemoStrategy::onBookUpdate(const BookUpdateEvent& ev)
{
  if (ev.update.symbol == _symbol && _book)
  {
    _book->applyBookUpdate(ev);
  }
}

}  // namespace demo
