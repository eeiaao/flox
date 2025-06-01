# Roadmap

This roadmap outlines upcoming improvements and tooling for the Flox HFT Engine.

## Planned Features

### 0. Developer Experience
- Formatting tools

### 1. Binary Format for Market Data
- Efficient serialization format for storing market data snapshots and trades.
- Designed to be faster and more compact than JSON or CSV.
- Intended for use in both live systems and backtesting environments.

### 2. Backtesting Tooling
- Infrastructure for offline strategy evaluation.
- Will support:
  - Market data replay (from binary format).
  - Order simulation and execution environment.
  - Strategy performance statistics and visualization.

### 3. Integrated JS engine
- To allow writing strategies using JavaScript

## Long-Term Goals

- Keep the engine modular and extensible.
- Ensure compatibility between live trading and backtest environments.

