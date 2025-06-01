# Tick Guard

The `TickGuard` is an RAII utility that automatically signals tick completion when it goes out of scope.

## Purpose

Simplify synchronization logic in `MarketDataBus` (synchronized version) by ensuring each subscriber signals its completion through the `TickBarrier`.

---

## Interface Summary

```cpp
class TickGuard {
public:
  explicit TickGuard(TickBarrier &barrier);
  ~TickGuard();

  TickGuard(const TickGuard &) = delete;
  TickGuard &operator=(const TickGuard &) = delete;
};
```

## Responsibilities

- Call `barrier.complete()` automatically on destruction
- Prevent missing tick completions due to early returns or exceptions

## Usage Example

```cpp
void onMarketData(...) {
  TickGuard guard(barrier);
  // ... process tick ...
} // completion is signaled automatically
```

## Notes

- Used in synchronous market data processing mode
- Should only be constructed with a valid reference to `TickBarrier`