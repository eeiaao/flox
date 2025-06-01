# Book Update Event

The `BookUpdateEvent` represents a snapshot or delta update to the order book, including both bids and asks.

## Purpose

To deliver order book updates to subscribers like order books, strategies, or analytics modules with minimal latency and no allocations.

---

## Interface Summary

```cpp
struct BookUpdateEvent : public IMarketDataEvent {
  SymbolId symbol;
  BookUpdateType type;
  std::pmr::vector<BookLevel> bids;
  std::pmr::vector<BookLevel> asks;
  std::chrono::system_clock::time_point timestamp;

  BookUpdateEvent(std::pmr::memory_resource *res);
  MarketDataEventType eventType() const noexcept override;
  void dispatchTo(IMarketDataSubscriber &sub) const override;
};
```

## BookLevel Structure

```cpp
struct BookLevel {
  double price;
  double quantity;
};
```

## Responsibilities

- Represent current book state or changes (snapshot vs delta)
- Contain bid/ask levels in memory-resource-backed containers
- Dispatch itself to any subscriber via `dispatchTo(...)`

## Memory Optimization

- Uses `std::pmr::vector` for `bids` and `asks` to reduce allocation overhead
- Allocator is provided during construction from the owning `EventPool`

## Notes

- Produced by market data connectors or simulators
- Consumed by `WindowedOrderBook`, `FullOrderBook`, strategies, etc.
- Supports accurate timestamping via `timestamp` field