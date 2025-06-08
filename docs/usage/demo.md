# Demo Application

The `demo` folder contains a minimal example showing how to wire Flox components together.
It assembles a working engine with simulated market data and a very simple trading strategy.

## Features

- `DemoConnector` generates synthetic trades and book updates
- `DemoStrategy` reacts to trades and updates its own order book
- `SimpleOrderExecutor` submits mock orders and reports fills via `OrderExecutionBus`
- `SimplePnLTracker`, `SimpleKillSwitch`, and `SimpleRiskManager` illustrate supporting modules
- `DemoBuilder` creates `MarketDataBus`, `CandleBus`, and other subsystems

## Running

After building the project with CMake you can run the demo binary:

```bash
./demo/flox_demo
```

It will start two demo connectors, publish market data, and run the strategy for
around five seconds before stopping.
