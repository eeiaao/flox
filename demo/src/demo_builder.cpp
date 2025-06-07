#include "demo/demo_builder.h"

#include <chrono>
#include <iostream>
#include <string>

namespace demo
{

DemoBuilder::DemoBuilder(const EngineConfig& cfg) : _config(cfg) {}

std::unique_ptr<IEngine> DemoBuilder::build()
{
  auto mdb = std::make_unique<Subsystem<MarketDataBus>>(std::make_unique<MarketDataBus>());
  auto execBus = std::make_unique<Subsystem<OrderExecutionBus>>(std::make_unique<OrderExecutionBus>());

  auto execTracker = std::make_unique<ConsoleExecutionTracker>();
  auto pnlTracker = std::make_unique<SimplePnLTracker>();
  auto posMgr = std::make_unique<SimplePositionManager>();
  auto storage = std::make_unique<StdoutStorageSink>();
  auto killSwitch = std::make_unique<SimpleKillSwitch>();
  auto riskMgr = std::make_unique<SimpleRiskManager>(killSwitch.get());
  auto validator = std::make_unique<SimpleOrderValidator>();
  auto executor = std::make_unique<SimpleOrderExecutor>(*execBus->get());
  executor->setExecutionTracker(execTracker.get());
  executor->setPnLTracker(pnlTracker.get());
  executor->setStorageSink(storage.get());
  executor->setPositionManager(posMgr.get());
  executor->setListener(posMgr.get());

  auto obFactory = std::make_unique<WindowedOrderBookFactory>();
  WindowedOrderBookConfig bookCfg(Price::fromDouble(0.01), Price::fromDouble(10));

  std::vector<std::shared_ptr<IStrategy>> strategies;
  for (SymbolId sym = 0; sym < 3; ++sym)
  {
    auto* book = obFactory->create(bookCfg);
    auto strat = std::make_shared<DemoStrategy>(sym, book);
    strat->setOrderExecutor(executor.get());
    strat->setOrderValidator(validator.get());
    strat->setRiskManager(riskMgr.get());
    strat->setPositionManager(posMgr.get());
    strat->setKillSwitch(killSwitch.get());
    strategies.push_back(std::move(strat));
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

  auto candleAgg = std::make_unique<CandleAggregator>(
      std::chrono::seconds{60},
      [](SymbolId, const Candle& c)
      { std::cout << "[candle] close=" << c.close.toDouble() << '\n'; });

  for (auto& strat : strategies)
  {
    mdb->get()->subscribe(strat);
  }

  mdb->get()->subscribe(std::shared_ptr<IMarketDataSubscriber>(candleAgg.get(), [](auto*) {}));

  std::vector<std::shared_ptr<ExchangeConnector>> connectors;
  for (SymbolId sym = 0; sym < 2; ++sym)
  {
    auto conn = std::make_shared<DemoConnector>(std::string("demo") + char('A' + sym), sym, *mdb->get());
    connectors.push_back(conn);
  }

  std::vector<std::unique_ptr<ISubsystem>> subsystems;
  subsystems.push_back(std::move(mdb));
  subsystems.push_back(std::move(execBus));
  for (auto& sub : strategySubs)
    subsystems.push_back(std::move(sub));
  subsystems.push_back(std::move(executor));
  subsystems.push_back(std::move(riskMgr));
  subsystems.push_back(std::move(posMgr));
  subsystems.push_back(std::move(storage));
  subsystems.push_back(std::move(candleAgg));

  subsystems.push_back(std::make_unique<Subsystem<WindowedOrderBookFactory>>(std::move(obFactory)));
  subsystems.push_back(std::make_unique<Subsystem<IOrderValidator>>(std::move(validator)));
  subsystems.push_back(std::make_unique<Subsystem<IExecutionTracker>>(std::move(execTracker)));
  subsystems.push_back(std::make_unique<Subsystem<IPnLTracker>>(std::move(pnlTracker)));
  subsystems.push_back(std::make_unique<Subsystem<IKillSwitch>>(std::move(killSwitch)));

  return std::make_unique<Engine>(_config, std::move(subsystems), std::move(connectors));
}

}  // namespace demo
