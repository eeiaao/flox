# Running the Flox Engine

This guide explains how to initialize and run the Flox engine using a custom `IEngineBuilder` implementation.

## Structure

To launch the engine:

1. Implement the `IEngineBuilder` interface
2. Register subsystems, connectors, and strategies
3. Return a fully initialized `IEngine` instance (typically `Engine`)

## Example

Here’s a simplified example using `DemoEngineBuilder`:

```cpp
class DemoEngineBuilder : public IEngineBuilder {
public:
  DemoEngineBuilder(const EngineConfig &config) : _config(config) {}

  std::unique_ptr<IEngine> build() override {
    auto registry = std::make_unique<SymbolRegistry>();
    auto mdb = std::make_unique<MarketDataBus>();

    // Register connectors
    ConnectorFactory::instance().registerConnector("bybit",
      [mdb = mdb.get(), registry = registry.get()](const std::string &symbolStr) {
        auto symbolId = registry->getSymbolId("bybit", symbolStr);
        auto conn = std::make_shared<BybitExchangeConnector>(symbolStr, *symbolId);
        conn->setCallbacks(
          [mdb](BookUpdateEvent *b) { mdb->publish(b->wrap()); },
          [mdb](TradeEvent *t) { mdb->publish(t->wrap()); });
        return conn;
      });

    std::vector<std::shared_ptr<ExchangeConnector>> connectors;
    std::vector<std::unique_ptr<ISubsystem>> subsystems;

    // Symbol registration
    for (const auto &ex : _config.exchanges) {
      for (const auto &sym : ex.symbols) {
        registry->registerSymbol(ex.name, sym.symbol);
        auto conn = ConnectorFactory::instance().createConnector(ex.name, sym.symbol);
        if (conn) connectors.push_back(conn);
      }
    }

    // Strategy manager setup
    auto strategyMgr = std::make_unique<StrategyManager>();

    // Strategy wiring
    auto strategies = ImpulseBreakoutStrategyFactory::createStrategiesFromFile(
      "impulse_breakout_config.json", registry.get());

    for (const auto &strat : strategies) {
      auto executor = std::make_unique<SimulatedOrderExecutor>();
      strat->setOrderExecutor(executor.get());
      strategyMgr->addStrategy(strat);
      subsystems.push_back(std::move(executor));
      mdb->subscribe(strat);
    }

    // Subsystem wiring
    subsystems.push_back(std::move(mdb));
    subsystems.push_back(std::move(strategyMgr));
    subsystems.push_back(std::make_unique<Subsystem<SymbolRegistry>>(std::move(registry)));

    return std::make_unique<Engine>(_config, std::move(subsystems), std::move(connectors));
  }

private:
  EngineConfig _config;
};
```

## Notes

- Strategies must implement `IMarketDataSubscriber`
- Use `Subsystem<T>` to integrate non-ISubsystem modules
- `EventHandle` + `wrap()` ensures pooled events are properly dispatched
- To run the engine:

```cpp
auto engine = builder.build();
engine->start();
```

Use this pattern to dynamically configure symbols, connectors, and strategies at runtime.