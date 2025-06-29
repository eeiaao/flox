#include "demo/demo_builder.h"

#include <chrono>
#include <memory>
#include <string>

#include "flox/aggregator/bus/candle_bus.h"
#include "flox/aggregator/candle_aggregator.h"
#include "flox/engine/bus/market_data_bus.h"
#include "flox/execution/bus/order_execution_bus.h"
#include "flox/execution/execution_tracker_adapter.h"

namespace demo
{

DemoBuilder::DemoBuilder(const EngineConfig& cfg) : _config(cfg) {}

std::unique_ptr<IEngine> DemoBuilder::build()
{
  auto mdb = std::make_unique<MarketDataBus>();
  auto execBus = std::make_unique<OrderExecutionBus>();

  auto execTracker = std::make_unique<ConsoleExecutionTracker>();
  auto trackerAdapter = std::make_shared<ExecutionTrackerAdapter>(1, execTracker.get());
  execBus->subscribe(trackerAdapter);

  auto obFactory = std::make_unique<WindowedOrderBookFactory>();
  WindowedOrderBookConfig bookCfg(Price::fromDouble(0.01), Price::fromDouble(10));

  std::vector<std::shared_ptr<IStrategy>> strategies;
  std::vector<std::unique_ptr<ISubsystem>> subsystems;

  for (SymbolId sym = 0; sym < 8; ++sym)
  {
    auto killSwitch = std::make_unique<SimpleKillSwitch>();
    auto riskMgr = std::make_unique<SimpleRiskManager>(killSwitch.get());
    auto posMgr = std::make_unique<SimplePositionManager>(100);
    auto validator = std::make_unique<SimpleOrderValidator>();
    auto pnlTracker = std::make_unique<SimplePnLTracker>();
    auto storage = std::make_unique<StdoutStorageSink>();

    auto executor = std::make_unique<SimpleOrderExecutor>(*execBus);
    executor->setPnLTracker(pnlTracker.get());
    executor->setStorageSink(storage.get());
    executor->setPositionManager(posMgr.get());
    executor->setListener(posMgr.get());

    auto* book = obFactory->create(bookCfg);
    auto strat = std::make_shared<DemoStrategy>(sym, book);
    strat->setOrderExecutor(executor.get());
    strat->setOrderValidator(validator.get());
    strat->setRiskManager(riskMgr.get());
    strat->setPositionManager(posMgr.get());
    strat->setKillSwitch(killSwitch.get());

    strategies.push_back(strat);

    subsystems.push_back(std::move(executor));
    subsystems.push_back(std::move(riskMgr));
    subsystems.push_back(std::move(posMgr));
    subsystems.push_back(std::make_unique<Subsystem<SimpleOrderValidator>>(std::move(validator)));
    subsystems.push_back(std::make_unique<Subsystem<SimplePnLTracker>>(std::move(pnlTracker)));
    subsystems.push_back(std::move(storage));
    subsystems.push_back(std::make_unique<Subsystem<SimpleKillSwitch>>(std::move(killSwitch)));
  }

  class StrategySubsystem : public ISubsystem
  {
   public:
    explicit StrategySubsystem(std::shared_ptr<IStrategy> s) : _s(std::move(s)) {}
    void start() override { _s->onStart(); }
    void stop() override { _s->onStop(); }

   private:
    std::shared_ptr<IStrategy> _s;
  };
  std::vector<std::unique_ptr<ISubsystem>> strategySubs;
  for (auto& strat : strategies)
    strategySubs.push_back(std::make_unique<StrategySubsystem>(strat));

  auto candleBus = std::make_unique<CandleBus>();

  for (auto& strat : strategies)
  {
    mdb->subscribe(strat);
    candleBus->subscribe(strat);
  }

  mdb->subscribe(std::make_shared<CandleAggregator>(std::chrono::seconds{60}, candleBus.get()));

  std::vector<std::shared_ptr<ExchangeConnector>> connectors;
  for (SymbolId sym = 0; sym < 3; ++sym)
  {
    auto conn = std::make_shared<DemoConnector>(std::string("demo") + char('A' + sym), sym, *mdb);
    connectors.push_back(conn);
  }

  for (auto& sub : strategySubs)
  {
    subsystems.push_back(std::move(sub));
  }

  subsystems.push_back(std::make_unique<Subsystem<MarketDataBus>>(std::move(mdb)));
  subsystems.push_back(std::make_unique<Subsystem<CandleBus>>(std::move(candleBus)));
  subsystems.push_back(std::make_unique<Subsystem<OrderExecutionBus>>(std::move(execBus)));
  subsystems.push_back(std::make_unique<Subsystem<WindowedOrderBookFactory>>(std::move(obFactory)));
  subsystems.push_back(std::make_unique<Subsystem<IExecutionTracker>>(std::move(execTracker)));

  return std::make_unique<Engine>(_config, std::move(subsystems), std::move(connectors));
}

}  // namespace demo
