# Architecture

Flox is a **modular** framework for building low-latency
execution systems.  Design goals:

* **Separation of concerns** – logic, transport, persistence, and risk live in
  different modules.
* **Predictable performance** – bounded queues, fixed-precision math, pooled
  events.
* **Composability** – every component is assembled at runtime through
  dependency injection; no global state.

## Layer 1 – Contracts

Pure, stateless **concepts** that any implementation must satisfy.

| Concept                          | Purpose                                |
|----------------------------------|----------------------------------------|
| `Strategy`                       | Trading logic (subscribes to market data, implements `Subsystem`). |
| `OrderExecutor` / `OrderValidator` / `RiskManager` | Pre-trade and execution pipeline. |
| `OrderExecutionListener`         | Receives fills / cancels / rejects.    |
| `PositionManager`, `KillSwitch`  | Post-trade state / circuit breaker.    |
| `OrderBook`, `ExchangeConnector` | Market-structure and data ingestion.   |
| `Subsystem`                      | Unified `start()` / `stop()` lifecycle.|
| `MarketDataSubscriber`           | Receives `BookUpdateEvent`, `TradeEvent`, `CandleEvent`. |

*All compile-time verified with `static_assert(concepts::…)>`.*

## Layer 2 – Type Erasure

Each concept has a **Trait** (static v-table) and a **Ref** (two-pointer
handle) generated with `meta::wrap`:

```
OrderBookTrait  ──► OrderBookRef
RiskManagerTrait ─► RiskManagerRef
...
```

* One pointer indirection, zero virtual inheritance.
* Fast cross-language FFI if needed (C ABI).

## Layer 3 – Infrastructure

| Component                | Highlights                                            |
|--------------------------|--------------------------------------------------------|
| `EventBus<Event,Policy>` | Lock-free fan-out (`SPSCQueue`) with `SyncPolicy` or `AsyncPolicy`. |
| `SPSCQueue<T,N>`         | Bounded, power-of-two ring buffer, wait-free.          |
| `Pool<T,N>` + `Handle<T>`| Intrusive ref-counted pool; no heap after startup.     |
| `TickBarrier` / `TickGuard` | Deterministic synchronisation for back-tests.       |
| `SymbolRegistry`         | `exchange:symbol` ⇆ `SymbolId` bijection.             |
| `CandleAggregator`       | Rolling OHLCV builder feeding `CandleBus`.            |

## Strategy Execution – PUSH vs PULL

| Mode   | Description                           | When to use          |
|--------|---------------------------------------|----------------------|
| **PUSH** (default) | Bus thread delivers events; strategy callback must be non-blocking. | Live trading, simple replay |
| **PULL**           | Strategy owns queue pointer and polls.         | ML models, synchronous research loops |

Switch by returning `SubscriberMode::PULL` from `mode()`.

## Market-Data Fan-Out

````
ExchangeConnector → {Book/Trade}Bus
↘ TickBarrier (sync mode only)
Strategy1  ← queue1
Strategy2  ← queue2
Logger     ← queue3
````

`EventDispatcher` resolves the correct `onBookUpdate` / `onTrade` / `onCandle`
at **compile time**.

## Lifecycle

Every runtime piece is a `Subsystem` (or wrapped by `Subsystem<T>`).  
`Engine` starts components in the order:

1. **Buses**  
2. **Strategies / Trackers / Sinks**  
3. **Connectors / Executors**

Shutdown reverses the sequence, guaranteeing clean teardown and drained queues.

## Memory Discipline

* All hot-path objects come from `pool::Pool`.
* `Decimal` fixed-point numbers avoid FP rounding.
* `std::pmr` arenas back grow-only containers for book levels & candles.

## Symbol-Centric Flow

* Every market event carries a `SymbolId` (32-bit).
* No string hashing on the hot path.
* Strategy can filter by simple integer comparison.

## Putting It Together

````cpp
auto strat   = make<MyStrategy>(execRef, riskRef, valRef);
marketBus.subscribe(strat);   // PUSH mode

Engine engine(cfg, {marketBus}, {strat}, {connector});
engine.start();
````

Flox moves the data at micro-second scale – you focus on the alpha.
