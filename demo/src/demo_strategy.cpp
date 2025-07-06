/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "demo/demo_strategy.h"

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/common.h"
#include "flox/execution/order.h"
#include "flox/execution/order_executor_component.h"
#include "flox/killswitch/killswitch_component.h"
#include "flox/risk/risk_manager_component.h"
#include "flox/validation/order_validator_component.h"

#include <iostream>

#include "demo/latency_collector.h"

namespace demo
{

DemoStrategy::DemoStrategy(SubscriberId id, SymbolId symbol,
                           KillSwitchRef killSwitch,
                           OrderValidatorRef orderValidator,
                           RiskManagerRef riskManager,
                           OrderExecutorRef orderExecutor)
    : _id(id), _symbol(symbol), _killSwitch(std::move(killSwitch)), _orderValidator(std::move(orderValidator)), _riskManager(std::move(riskManager)), _orderExecutor(std::move(orderExecutor)), _book(Price::fromDouble(0.1))
{
}

void DemoStrategy::start()
{
  std::cout << "[strategy " << _symbol << "] start" << std::endl;
}

void DemoStrategy::stop()
{
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

    _killSwitch.check(order);

    if (_killSwitch.isTriggered())
    {
      std::cout << "[kill] strategy " << _symbol
                << " blocked by kill switch"
                << ", reason: " << _killSwitch.reason() << "\n";
      return;
    }

    std::string reason;
    if (!_orderValidator.validate(order, reason))
    {
      std::cout << "[strategy " << _symbol << "] order rejected: " << reason << '\n';
      return;
    }

    if (!_riskManager.allow(order))
    {
      std::cout << "[risk] strategy " << _symbol << " rejected order id=" << order.id << '\n';
      return;
    }
  }

  _orderExecutor.submitOrder(order);
}

void DemoStrategy::onBookUpdate(const BookUpdateEvent& ev)
{
  if (ev.update.symbol == _symbol)
  {
    _book.applyBookUpdate(ev);
  }
}

void DemoStrategy::onCandle(const CandleEvent&) {}

}  // namespace demo
