/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/aggregator/events/candle_event.h"
#include "flox/book/nlevel_order_book.h"
#include "flox/common.h"
#include "flox/execution/order_executor_component.h"
#include "flox/killswitch/killswitch_component.h"
#include "flox/risk/risk_manager_component.h"
#include "flox/strategy/strategy_component.h"
#include "flox/validation/order_validator_component.h"

namespace demo
{
using namespace flox;

class DemoStrategy
{
 public:
  using Trait = traits::StrategyTrait;
  using Allocator = PoolAllocator<Trait, 8>;

  explicit DemoStrategy(
      SubscriberId id, SymbolId symbol,
      KillSwitchRef killSwitch,
      OrderValidatorRef orderValidator,
      RiskManagerRef riskManager,
      OrderExecutorRef orderExecutor);

  SubscriberId id() const { return _id; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  void start();
  void stop();

  void onTrade(const TradeEvent& trade);
  void onBookUpdate(const BookUpdateEvent& book);
  void onCandle(const CandleEvent&);

 private:
  SubscriberId _id;
  SymbolId _symbol;
  NLevelOrderBook<> _book;
  OrderId _nextId{0};

  KillSwitchRef _killSwitch;
  OrderValidatorRef _orderValidator;
  RiskManagerRef _riskManager;
  OrderExecutorRef _orderExecutor;
};

static_assert(flox::concepts::Strategy<DemoStrategy>);

}  // namespace demo
