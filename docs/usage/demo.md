# Demo Application

Directory **`demo/`** contains a self-contained example that wires FLOX
components into a runnable engine using **simulated** market data.

## Components

| Module                | Role                                                                                           |
|-----------------------|-------------------------------------------------------------------------------------------------|
| `DemoConnector`       | Generates synthetic trades & book updates (mock exchange feed).                                 |
| `DemoStrategy`        | Consumes trades / books, maintains its own order book, submits test orders.                     |
| `SimpleOrderExecutor` | Accepts orders from the strategy, sends mock acknowledgements & fills via **OrderExecutionBus**.|
| `SimplePnLTracker`    | Tracks realised / unrealised PnL using execution events.                                        |
| `SimpleKillSwitch`    | Triggers when max loss / qty / rate limits are breached.                                        |
| `SimpleRiskManager`   | Pre-trade checks; blocks orders that violate configured rules.                                  |
| `DemoBuilder`         | Assembles subsystems: `MarketDataBus`, `TradeBus`, `CandleBus`, `OrderExecutionBus`, etc.       |

## What It Demonstrates
* Full engine startup / shutdown sequence.  
* Event flow from connectors → buses → strategy → executor → trackers.  
* Interaction between risk, kill-switch, and execution pipeline.  
* Candle aggregation and simple logging.

## How to Run

1. **Build** the project with CMake (release or debug):
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   make -j$(nproc)
````

2. **Execute** the demo binary:

   ```bash
   ./demo/flox_demo
   ```

   The program:

   * launches two `DemoConnector` instances,
   * streams market data for **\~30 seconds**,
   * runs `DemoStrategy`,
   * then performs a graceful shutdown.
