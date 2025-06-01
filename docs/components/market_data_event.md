# Market Data Event Interface

The `IMarketDataEvent` interface defines the base class for all market data events in Flox, such as book updates, trades, and candles.

## Purpose

To provide a common, pool-aware, and type-safe interface for delivering and recycling events across the system.

---

## Interface Summary

```cpp
class IMarketDataEvent : public RefCountable {
public:
  virtual ~IMarketDataEvent() = default;

  virtual MarketDataEventType eventType() const noexcept = 0;
  virtual void dispatchTo(IMarketDataSubscriber &sub) const = 0;

  void setPool(IEventPool *pool);
  virtual void releaseToPool();
  virtual EventHandle<IMarketDataEvent> wrap();
  virtual void clear();
};
```

## Responsibilities

- Identify the event type (book, trade, candle)
- Dispatch itself to any `IMarketDataSubscriber`
- Participate in pooled memory management via `IEventPool`

## Pooling & Lifecycle

- Inherits from `RefCountable` to track references
- Implements `releaseToPool()` to return itself to the originating pool
- `wrap()` is used to safely create an `EventHandle`

## Notes

- All concrete event types like `BookUpdateEvent` or `TradeEvent` inherit from this
- Core to Flox's zero-allocation, high-throughput architecture