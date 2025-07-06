/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "demo/demo_builder.h"

#include <chrono>
#include <memory>
#include <string>

#include "demo/demo_connector.h"
#include "demo/demo_strategy.h"
#include "demo/simple_components.h"

#include "flox/aggregator/bus/candle_bus.h"
#include "flox/aggregator/candle_aggregator.h"
#include "flox/book/bus/book_update_bus.h"
#include "flox/book/bus/trade_bus.h"
#include "flox/engine/engine.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/engine/subsystem_component.h"
#include "flox/execution/bus/order_execution_bus.h"
#include "flox/util/base/ref.h"

namespace demo
{

DemoBuilder::DemoBuilder(const EngineConfig& cfg) : _config(cfg) {}

Engine DemoBuilder::build()
{
  std::vector<SubsystemRef> subsystems;
  std::vector<SubsystemRef> buses;

  auto bookUpdateBus = make<BookUpdateBus>();
  auto tradeBus = make<TradeBus>();
  auto execBus = make<OrderExecutionBus>();
  auto candleBus = make<CandleBus>();

  buses.push_back(bookUpdateBus.as<traits::SubsystemTrait>());
  buses.push_back(tradeBus.as<traits::SubsystemTrait>());
  buses.push_back(execBus.as<traits::SubsystemTrait>());
  buses.push_back(candleBus.as<traits::SubsystemTrait>());

  auto execTrackerAdapterHandle = make<ExecutionTrackerAdapter<ConsoleExecutionTracker>>(
      make<ConsoleExecutionTracker>(89));

  execBus.subscribe(execTrackerAdapterHandle);
  subsystems.push_back(execTrackerAdapterHandle.as<traits::SubsystemTrait>());

  for (SymbolId sym = 0; sym < 8; ++sym)
  {
    auto killSwitch = make<SimpleKillSwitch>();
    auto riskMgr = make<SimpleRiskManager>(killSwitch);
    auto posMgr = make<SimplePositionManager>(100);
    auto validator = make<SimpleOrderValidator>();
    auto pnlTracker = make<SimplePnLTracker>(43);
    auto storage = make<StdoutStorageSink>();

    auto simpleExecutor = make<SimpleOrderExecutor>(
        execBus,
        pnlTracker,
        storage,
        posMgr);

    auto strategy = make<DemoStrategy>(
        sym + 10, sym,
        killSwitch,
        validator,
        riskMgr,
        simpleExecutor);

    bookUpdateBus.subscribe(strategy.as<traits::MarketDataSubscriberTrait>());
    tradeBus.subscribe(strategy.as<traits::MarketDataSubscriberTrait>());

    subsystems.push_back(strategy.as<traits::SubsystemTrait>());
  }

  auto candleAggregator = make<CandleAggregator>(std::chrono::seconds{60}, candleBus);
  tradeBus.subscribe(candleAggregator.as<traits::MarketDataSubscriberTrait>());
  subsystems.push_back(candleAggregator.as<traits::SubsystemTrait>());

  std::vector<std::shared_ptr<ExchangeConnector>> connectors;
  for (SymbolId sym = 0; sym < 3; ++sym)
  {
    connectors.push_back(std::make_shared<DemoConnector>(
        std::string("demo") + char('A' + sym), sym, bookUpdateBus, tradeBus));
  }

  return Engine(_config, std::move(buses), std::move(subsystems), std::move(connectors));
}

}  // namespace demo
