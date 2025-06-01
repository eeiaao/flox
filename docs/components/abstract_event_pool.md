# Event Pool Interface

The `IEventPool` interface defines a generic contract for releasing market data events back into a memory pool.

## Purpose

To allow efficient reuse of market data event objects (e.g., `BookUpdateEvent`, `TradeEvent`) by avoiding repeated allocations and deallocations.

## Interface Definition

```cpp
class IEventPool {
public:
  virtual ~IEventPool() = default;
  virtual void release(IMarketDataEvent *event) = 0;
};
```

## Responsibilities

- Provide a mechanism to return `IMarketDataEvent` instances to the pool
- Act as a common base for all type-specific event pools

## Integration

- Typically used with `EventHandle<T>` to manage scoped lifetime
- `EventHandle` automatically releases the event back to the `IEventPool` when it goes out of scope

## Notes

- Used by `MarketDataBus` and `MarketDataEventPool`
- Ensures event lifecycle is managed outside of consumers
- Enables zero-allocation data pipelines