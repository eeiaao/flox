# Architecture

Flox is a modular framework for building low-latency execution systems. Its design emphasizes **separation of concerns**, **predictable performance**, and **composability**.

---

## Layers of the Architecture

### 1. Abstract Layer

Defines **pure interfaces** with no internal state. These are the contracts your system is built upon:

- `IStrategy`: strategy logic
- `IOrderExecutor`: order submission
- `IOrderExecutionListener`: execution events
- `IRiskManager`, `IOrderValidator`, `IPositionManager`: trade controls and state
- `IOrderBook`, `ExchangeConnector`: market structure
- `ISubsystem`: unified lifecycle interface
- `IMarketDataSubscriber`: receives events via data bus

#### Why it matters

- Enables simulation, replay, and mocking
- Decouples logic from implementation
- Ensures correctness can be validated independently of performance

---

### 2. Implementation Layer

- `Engine` — core coordinator implementing `IEngine`
- `FullOrderBook` — full-depth order storage for all price levels and quantities
- `WindowedOrderBook` — tick-centric view with center/tick size logic
- `BookUpdateEvent`, `TradeEvent` — pooled and reusable market data events
- `EventPool` + `EventHandle` — zero-allocation event lifecycle manager
- `MarketDataBus` — fan-out with lock-free queues (SPSCQueue)
- `EventBus` — generic template used by all bus types
- `SPSCQueue` — bounded ring buffer for 1:1 messaging
- `RefCountable` — atomic reference counting for pooled objects
- `SymbolRegistry` — maps strings to `SymbolId`
- `CandleAggregator` — rolling OHLC aggregator
- `SimpleOrderExecutor` — minimal executor used in the demo
- `PositionManager` — tracks open positions
- `Subsystem<T>` — wraps non-subsystem modules for uniform lifecycle

#### Features

- **Speed**: memory locality, `std::pmr`, lock-free queues
- **Control**: minimal allocations, deterministic fan-out
- **Modularity**: no global state, explicit composition

---

## Strategy Execution: PUSH and PULL Modes

Strategies implement `IMarketDataSubscriber` and can operate in two modes:

### PUSH Mode (default)

The bus actively delivers events to the strategy.

```cpp
class MyPushStrategy : public IStrategy {
public:
  void onBookUpdate(BookUpdateEvent *event) override { /* handle event */ }
};
```

The engine subscribes the strategy via:

```cpp
marketDataBus->subscribe(strategy);
```

### PULL Mode

The strategy explicitly pulls from its queue (used in polling setups or backtests):

```cpp
class MyPullStrategy : public IMarketDataSubscriber {
public:
  SubscriberMode mode() const override { return SubscriberMode::PULL; }

  void readLoop(SPSCQueue<EventHandle<BookUpdateEvent>> &queue) {
    EventHandle<BookUpdateEvent> ev;
    while (queue.pop(ev)) {
      EventDispatcher<EventHandle<BookUpdateEvent>>::dispatch(ev, *this);
    }
  }
};
```

---

## Market Data Fan-Out: MarketDataBus

The `MarketDataBus` delivers `EventHandle<T>` to each subscriber via dedicated lock-free queues.

### Event publishing:

```cpp
bus->publish(std::move(bookUpdate));
```

### Fan-out process

- Each subscriber has a queue (`SPSCQueue`)
 - `EventHandle<T>` wraps and manages event lifecycle
 - `EventDispatcher` delivers each event to the subscriber's callback

### Subscribing:

```cpp
bus->subscribe(myStrategy);
```

In sync mode, subscribers are coordinated via `TickBarrier` and `TickGuard`.

---

## Lifecycle and Subsystems

Each runtime component (strategy, book, sink) is a `ISubsystem` or wrapped in `Subsystem<T>`.  
They expose `start()` and `stop()` for controlled startup/shutdown.

This enables:

- Replay testing
- Safe teardown
- Deterministic benchmarking

---

## Memory and Performance

Flox emphasizes **allocation-free execution paths**:

- `BookUpdateEvent` and `TradeEvent` come from pooled memory
- `EventHandle<T>` ensures safe scoped lifecycle
- `SPSCQueue` supports lock-free delivery
- `std::pmr::vector` is used for zero-allocation order book updates

---

## Symbol-Centric Design

Every operation is indexed by `SymbolId` — a compact `uint32_t` mapped from `exchange:symbol`:

- No string hashing or comparison in hot paths
- Fast per-symbol routing, filtering, fan-out
- Compatible with routing layers (`SymbolIdOrderRouter`, etc.)

---

## Intended Use

Flox is **not a complete trading engine**. It is a **toolbox** for building:

- Real-time trading systems
- Simulators and backtesters
- Distributed signal routers
- Custom HFT infrastructure

Perfect for C++ teams needing:

- Predictable latency
- Fine-grained memory control
- Composable systems

---

## Example Integration

```cpp
auto strategy = std::make_shared<MyStrategy>();
marketDataBus->subscribe(strategy);

marketDataBus->publish(std::move(bookUpdate));
```

In pull-mode:

```cpp
auto *queue = marketDataBus->getQueue(strategy->id());
while (queue->pop(event)) {
  EventDispatcher<EventHandle<BookUpdateEvent>>::dispatch(event, *strategy);
}
```

---

## Summary

Flox is:

- **Modular** — you compose what you need
- **Predictable** — designed for deterministic latency
- **Safe** — pooled events, scoped handles, lifecycle management
- **Flexible** — works in simulation, backtest, or live modes

You define your logic — Flox moves the data.