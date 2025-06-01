# Trade Event

The `TradeEvent` represents an executed trade in the market, including direction, price, and quantity.

## Purpose

To provide subscribers (e.g., strategies, volume analyzers) with real-time trade information in a zero-allocation, poolable format.

---

## Interface Summary

```cpp
struct TradeEvent : public IMarketDataEvent {
  SymbolId symbol;
  double price;
  double quantity;
  bool isBuy;
  std::chrono::system_clock::time_point timestamp;

  TradeEvent(std::pmr::memory_resource *);
  MarketDataEventType eventType() const noexcept override;
  void dispatchTo(IMarketDataSubscriber &sub) const override;
};
```

## Responsibilities

- Represent a single trade tick
- Indicate direction with `isBuy`
- Dispatch itself to subscribers like strategies or metrics collectors

## Memory Optimization

- Accepts a polymorphic memory resource for consistency with pooled event design
- Recycled via `EventPool<TradeEvent>`

## Notes

- Used by latency-sensitive strategies for trade confirmation
- Supports precise event timestamps for time-based logic