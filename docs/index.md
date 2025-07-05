# Flox

**Flox** is a modular C++20 toolbox for building **low-latency execution
systems**.  The framework cleanly separates *abstract interfaces* from
*high-performance implementations*, so the same trading logic can run in live,
sim, or back-test modes with identical APIs.

## Key Capabilities

* **Pluggable connectors** – inject exchange adapters through `ConnectorFactory`.
* **Unified EventBus** – lock-free fan-out for book, trade, candle, and order
  events; swap between deterministic `SyncPolicy` and ultra-low-latency
  `AsyncPolicy`.
* **Lifecycle-controlled strategies** – every strategy is a `Subsystem`;
  the engine starts and stops them deterministically.
* **Optimised order books** – full-depth (`NLevelOrderBook`) and windowed
  variants, allocation-free.
* **Memory discipline** – pooled events (`pool::Pool`), fixed-precision
  `Decimal`, `std::pmr` arenas.
* **Modular risk path** – validators, risk managers, kill switches are all
  hot-swappable via type-erased refs.

## Typical Use-Cases

| Scenario               | Flox Role                                                |
|------------------------|----------------------------------------------------------|
| **Live trading**       | Deterministic latency, per-symbol routing, risk hooks.   |
| **Back-testing**       | Same code, offline replay, tick-accurate execution sim.  |
| **Analytics pipeline** | Event-driven processing of large tick datasets.          |
| **Custom HFT stack**   | Compose only needed blocks; zero external dependencies.  |

## Getting Started

1. Build the library (see **Getting Started** doc).  
2. Implement or reuse an `EngineBuilder` that wires buses, connectors, strategies.  
3. Launch:

````cpp
auto engine = builder.build();
engine->start(); // blocks until stop()
````

## Learn More

* **Architecture** – deep dive into layers, PUSH/PULL modes, memory model.
* **Writing Strategies** – implement `Strategy` concept, inject risk & exec refs.
* **Demo Application** – run a full mock exchange + strategy in \~30 seconds.
