# Tick Barrier

The `TickBarrier` is a synchronization primitive used to coordinate multiple subscribers during synchronous market data processing.

## Purpose

To ensure all subscribers complete their tick before the next tick is published, enabling fully deterministic event handling.

---

## Interface Summary

```cpp
class TickBarrier {
public:
  explicit TickBarrier(size_t total);
  void complete();
  void wait();
};
```

## Responsibilities

- Track the number of subscribers that have completed processing
- Block until all subscribers signal completion of the current tick

## Usage

Used in `SyncMarketDataBus` when `USE_SYNC_MARKET_BUS` is defined:

```cpp
barrier.complete(); // Subscriber signals done
barrier.wait();     // Publisher waits for all to complete
```

## Notes

- Thread-safe via atomic counter
- Uses spin-waiting (`yield`) to avoid context switches
- Should only be used in low-latency, simulation, or testing environments