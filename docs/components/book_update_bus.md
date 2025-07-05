# BookUpdateBus

`BookUpdateBus` is a compile-time alias of the generic `EventBus` specialised for order-book updates (`BookUpdateEvent`).  
Mode is chosen via the `USE_SYNC_MARKET_BUS` macro:

```cpp
#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<
    pool::Handle<BookUpdateEvent>,
    SyncPolicy<pool::Handle<BookUpdateEvent>>>;
#else
using BookUpdateBus = EventBus<
    pool::Handle<BookUpdateEvent>,
    AsyncPolicy<pool::Handle<BookUpdateEvent>>>;
#endif

using BookUpdateBusRef =
    EventBusRef<pool::Handle<BookUpdateEvent>, BookUpdateBus::Queue>;
```

## Purpose

Deliver **book-update events** from exchange connectors to multiple consumers (order books, strategies, metrics, loggers) with minimal latency and without extra dynamic allocations inside the bus itself.

## Responsibilities

| Aspect | Details |
|--------|---------|
| **Fan-out** | Writes each `BookUpdateEvent` handle into a dedicated SPSC queue per subscriber. |
| **Policy** | `SyncPolicy` → deterministic, tick-barrier coordination for simulation/tests.<br>`AsyncPolicy` → lock-free delivery for live trading. |
| **Reference type** | `BookUpdateBusRef` lets producers publish without owning the full bus object. |

---

## Internal Behaviour

* **Queues:** One ring buffer per subscriber eliminates contention between consumers.
* **Sync mode:** After publishing a tick, the bus blocks until *every* subscriber signals completion via `TickBarrier`.
* **Async mode:** Publisher writes and returns immediately; subscribers consume at their own pace.
* **Memory:** The bus itself does not allocate after construction.  
  Event objects (**not** the bus) may come from a pool when they carry containers or other heap-backed data; plain POD events are created directly.

## Typical Use Cases

| Producer | Consumers |
|----------|-----------|
| `BybitExchangeConnector` | `NLevelOrderBook`, trading strategies, `ExecutionTrackerComponent` |
| Historical tick-replayer | Back-test harness, benchmarking tools |

## Notes

* Thread-safe under the guarantees of the chosen policy (lock-free vs. barrier-sync).
* Define/undefine `USE_SYNC_MARKET_BUS` **before** including the header to switch modes.
* One bus instance per market-data feed is recommended to avoid cross-core cache traffic.
