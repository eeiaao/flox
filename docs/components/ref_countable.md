# RefCountable

The `RefCountable` class provides a lightweight reference counting mechanism for pooled objects.

## Purpose

To enable safe and deterministic memory reuse of shared objects (such as events) in a high-performance, low-latency environment.

---

## Interface Summary

```cpp
class RefCountable {
public:
  void retain() noexcept;
  bool release() noexcept;
  void resetRefCount(uint32_t value = 1) noexcept;
  uint32_t refCount() const noexcept;

protected:
  std::atomic<uint32_t> _refCount{0};
};
```

## Responsibilities

- Track ownership count for pooled resources
- Automatically indicate when an object can be recycled
- Prevent premature release or double-free via assertions

## Usage

- Inherited by `IMarketDataEvent`
- Used implicitly by `EventHandle` to manage scoped lifecycle

## Notes

- Not thread-safe for shared access — designed for single-threaded ownership transfers
- Used in conjunction with `EventPool` and `MarketDataBus`