# TradeEvent

`TradeEvent` carries a single **tick trade** through the FLOX market-data pipeline.

```cpp
struct TradeEvent {
  using Listener = MarketDataSubscriberRef;

  Trade    trade;        // price, qty, side, timestamp
  uint64_t tickSequence; // monotonic id set by EventBus

  void clear() { trade = {}; }  // recycle when pooled
};
```

## Purpose
* Convey raw trades from connectors to strategies, metrics, and loggers with **zero allocations**.

## Fields

| Field          | Meaning                                       |
|----------------|-----------------------------------------------|
| `trade`        | Struct holding price, quantity, side, ts.     |
| `tickSequence` | Sequence number injected by `EventBus`.       |

## Notes
* Trivially copyable; `clear()` prepares the object for reuse in a pool.
* Published on `TradeBus`, consumed by any component implementing `MarketDataSubscriber`.
