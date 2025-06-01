# Event Handle & Event Pool

The `EventHandle` and `EventPool` classes together manage the lifecycle of market data events in a high-performance, allocation-free manner.

## Purpose

- `EventPool<T>`: Provides pre-allocated, recyclable instances of event objects (like `BookUpdateEvent`)
- `EventHandle<T>`: Manages scoped ownership of an event, automatically returning it to the pool

---

## EventHandle

### Overview

```cpp
template <typename EventT>
class EventHandle {
  EventT* get() const;
  EventT* operator->() const;
  EventT& operator*() const;
  operator bool() const;
  template <typename U> EventHandle<U> upcast() const;
};
```

### Responsibilities

- Retain ownership of a pooled event
- Return event to the pool automatically via `release()`
- Prevent double-free via RAII
- Enable upcasting to base types like `IMarketDataEvent`

---

## EventPool

### Overview

```cpp
template <typename EventT, size_t Capacity>
class EventPool : public IEventPool {
  EventHandle<EventT> acquire();
  void release(IMarketDataEvent* event) override;
};
```

### Responsibilities

- Pre-allocate a fixed-capacity pool of `EventT` using aligned storage
- Provide fast lock-free reuse with `SPSCQueue`
- Track acquired and released counts

---

## Notes

- Used in `MarketDataBus` and `MarketDataEventPool`
- Ensures zero allocations during event fan-out
- Highly efficient for HFT workloads due to memory locality and lock-freedom