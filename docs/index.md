# Flox

Flox is a modular C++ framework for building low-latency execution systems.  
It provides a clean separation between abstract interfaces and optimized implementations, allowing for:

- Real-time trading infrastructure
- Event-driven processing
- Simulation and live execution
- Deterministic performance tuning with full control over memory and routing

## Highlights

- Pluggable connectors and routing
- Fast in-memory order books
- Strategy engine with lifecycle control
- Unified subscription bus for market data
- Dedicated buses for book, trade, candle, and order events
- Generic `EventBus` powers all event dispatch
